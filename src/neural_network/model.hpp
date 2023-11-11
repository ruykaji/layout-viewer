#ifndef __MODEL_H__
#define __MODEL_H__

#include "data.hpp"
#include "encoders/efficientnetb3.hpp"

// UNet model
// ======================================================================================

struct AttentionBlockImpl : torch::nn::Module {
    torch::nn::Sequential W_g { nullptr };
    torch::nn::Sequential W_x { nullptr };
    torch::nn::Sequential psi { nullptr };
    torch::nn::ReLU6 relu { nullptr };

    AttentionBlockImpl(int64_t F_g, int64_t F_l, int64_t F_int)
    {
        W_g = torch::nn::Sequential(
            torch::nn::Conv2d(torch::nn::Conv2dOptions(F_g, F_int, 1).stride(1).padding(0).bias(true)),
            torch::nn::BatchNorm2d(F_int));

        W_x = torch::nn::Sequential(
            torch::nn::Conv2d(torch::nn::Conv2dOptions(F_l, F_int, 1).stride(1).padding(0).bias(true)),
            torch::nn::BatchNorm2d(F_int));

        psi = torch::nn::Sequential(torch::nn::Conv2d(torch::nn::Conv2dOptions(F_int, 1, 1).stride(1).padding(0).bias(true)),
            torch::nn::BatchNorm2d(1),
            torch::nn::Sigmoid());

        relu = torch::nn::ReLU6(torch::nn::ReLU6Options(true));

        register_module("W_g", W_g);
        register_module("W_x", W_x);
        register_module("psi", psi);
    };

    torch::Tensor forward(torch::Tensor g, torch::Tensor x)
    {
        auto g1 = W_g->forward(g);
        auto x1 = W_x->forward(x);

        return x * psi->forward(relu->forward(g1 + x1));
    }
};

TORCH_MODULE(AttentionBlock);

struct DecoderBlockImpl : torch::nn::Module {
    AttentionBlock attention { nullptr };
    torch::nn::Sequential conv1 { nullptr };
    torch::nn::Sequential conv2 { nullptr };

    DecoderBlockImpl(int64_t in_channels, int64_t skip_channels, int64_t out_channels)
    {
        if (skip_channels > 0) {
            attention = AttentionBlock(in_channels, skip_channels, out_channels);
            register_module("attention", attention);
        }

        conv1 = torch::nn::Sequential(
            torch::nn::Conv2d(torch::nn::Conv2dOptions(in_channels + skip_channels, out_channels, 3).padding(1).bias(false)),
            torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true)),
            torch::nn::BatchNorm2d(out_channels));

        conv2 = torch::nn::Sequential(
            torch::nn::Conv2d(torch::nn::Conv2dOptions(out_channels, out_channels, 3).padding(1).bias(false)),
            torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true)),
            torch::nn::BatchNorm2d(out_channels));

        register_module("conv1", conv1);
        register_module("conv2", conv2);
    }

    torch::Tensor forward(torch::Tensor x, torch::Tensor skip = torch::Tensor())
    {
        x = torch::nn::functional::interpolate(x, torch::nn::functional::InterpolateFuncOptions().scale_factor(std::vector<double>({ 2.0, 2.0 })).mode(torch::kBilinear).align_corners(true));

        if (skip.defined()) {
            skip = attention->forward(x, skip);
            x = torch::cat({ x, skip }, 1);
        }

        x = conv1->forward(x);
        x = conv2->forward(x);

        return x;
    }
};

TORCH_MODULE(DecoderBlock);

struct SegmentationHeadImpl : torch::nn::Module {
    torch::nn::Conv2d conv1 { nullptr };

    SegmentationHeadImpl() = default;

    SegmentationHeadImpl(int64_t in_channels, int64_t out_channels, int64_t kernel_size = 3)
    {
        conv1 = torch::nn::Conv2d(torch::nn::Conv2dOptions(in_channels, out_channels, kernel_size).padding(kernel_size / 2));

        register_module("conv1", conv1);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        return conv1->forward(x);
    }
};

TORCH_MODULE(SegmentationHead);

struct UNetImpl : torch::nn::Module {
    EfficientNetB3 encoder { nullptr };
    SegmentationHead segmentationHead { nullptr };

    std::vector<DecoderBlock> decoderBlocks {};

    UNetImpl()
    {
        encoder = EfficientNetB3();

        std::vector<int32_t> encoderChannels = { 40, 32, 48, 136, 384 };
        std::vector<int32_t> decoderChannels = { 256, 128, 64, 32, 16 };

        std::reverse(encoderChannels.begin(), encoderChannels.end());

        std::vector<int32_t> inChannels = decoderChannels;

        inChannels.insert(inChannels.begin(), encoderChannels[0]);
        inChannels.pop_back();

        std::vector<int32_t> skipChannels = encoderChannels;

        skipChannels.erase(skipChannels.begin());
        skipChannels.emplace_back(0);

        std::vector<int32_t> outChannels = decoderChannels;

        for (int32_t i = 0; i < inChannels.size(); ++i) {
            decoderBlocks.emplace_back(DecoderBlock(inChannels[i], skipChannels[i], outChannels[i]));

            register_module("decoder_block_" + std::to_string(i), decoderBlocks.back());
        }

        segmentationHead = SegmentationHead(decoderChannels.back(), 5);

        register_module("encoder", encoder);
        register_module("segmentationHead", segmentationHead);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        std::vector<at::Tensor> encoderFeatures = encoder->forward(x);
        std::reverse(encoderFeatures.begin(), encoderFeatures.end());

        auto _x = encoderFeatures[0];

        for (std::size_t i = 0; i < decoderBlocks.size(); ++i) {
            if (i + 1 < encoderFeatures.size()) {
                _x = decoderBlocks[i]->forward(_x, encoderFeatures[i + 1]);
            } else {
                _x = decoderBlocks[i]->forward(_x);
            }
        }

        return segmentationHead->forward(_x);
    }
};

TORCH_MODULE(UNet);

// Unet3D model
// ======================================================================================

struct DecoderBlock3DImpl : torch::nn::Module {
    torch::nn::Sequential conv1 { nullptr };
    torch::nn::Sequential conv2 { nullptr };

    DecoderBlock3DImpl(int64_t in_channels, int64_t skip_channels, int64_t out_channels)
    {
        conv1 = torch::nn::Sequential(
            torch::nn::Conv3d(torch::nn::Conv3dOptions(in_channels + skip_channels, out_channels, 3).padding(1).bias(false)),
            torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true)),
            torch::nn::BatchNorm3d(out_channels));

        conv2 = torch::nn::Sequential(
            torch::nn::Conv3d(torch::nn::Conv3dOptions(out_channels, out_channels, 3).padding(1).bias(false)),
            torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true)),
            torch::nn::BatchNorm3d(out_channels));

        register_module("conv1", conv1);
        register_module("conv2", conv2);
    }

    torch::Tensor forward(torch::Tensor x, torch::Tensor skip = torch::Tensor())
    {
        x = torch::nn::functional::interpolate(x, torch::nn::functional::InterpolateFuncOptions().scale_factor(std::vector<double>({ 2.0, 2.0, 2.0 })).mode(torch::kTrilinear).align_corners(true));

        if (skip.defined()) {
            x = torch::cat({ x, skip }, 1);
        }

        x = conv1->forward(x);
        x = conv2->forward(x);

        return x;
    }
};

TORCH_MODULE(DecoderBlock3D);

struct SegmentationHead3DImpl : torch::nn::Module {
    torch::nn::Conv3d conv1 { nullptr };

    SegmentationHead3DImpl() = default;

    SegmentationHead3DImpl(int64_t in_channels, int64_t out_channels, int64_t kernel_size = 3)
    {
        conv1 = torch::nn::Conv3d(torch::nn::Conv3dOptions(in_channels, out_channels, kernel_size).padding(kernel_size / 2));

        register_module("conv1", conv1);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        return conv1->forward(x);
    }
};

TORCH_MODULE(SegmentationHead3D);

struct UNet3DImpl : torch::nn::Module {
    ResNet50Backbone3D encoder { nullptr };
    SegmentationHead3D segmentationHead { nullptr };

    std::vector<DecoderBlock3D> decoderBlocks {};

    UNet3DImpl()
    {
        encoder = ResNet50Backbone3D();

        std::vector<int32_t> encoderChannels = { 64, 256, 512, 1024, 2048 };
        std::vector<int32_t> decoderChannels = { 256, 128, 64, 32, 16 };

        std::reverse(encoderChannels.begin(), encoderChannels.end());

        std::vector<int32_t> inChannels = decoderChannels;

        inChannels.insert(inChannels.begin(), encoderChannels[0]);
        inChannels.pop_back();

        std::vector<int32_t> skipChannels = encoderChannels;

        skipChannels.erase(skipChannels.begin());
        skipChannels.emplace_back(0);

        std::vector<int32_t> outChannels = decoderChannels;

        for (int32_t i = 0; i < inChannels.size(); ++i) {
            decoderBlocks.emplace_back(DecoderBlock3D(inChannels[i], skipChannels[i], outChannels[i]));

            register_module("decoder_block_" + std::to_string(i), decoderBlocks.back());
        }

        segmentationHead = SegmentationHead3D(decoderChannels.back(), 1);

        register_module("encoder", encoder);
        register_module("segmentationHead", segmentationHead);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        std::vector<at::Tensor> encoderFeatures = encoder->forward(x);
        std::reverse(encoderFeatures.begin(), encoderFeatures.end());

        auto _x = encoderFeatures[0];

        for (std::size_t i = 0; i < decoderBlocks.size(); ++i) {
            if (i + 1 < encoderFeatures.size()) {
                _x = decoderBlocks[i]->forward(_x, encoderFeatures[i + 1]);
            } else {
                _x = decoderBlocks[i]->forward(_x);
            }
        }

        return segmentationHead->forward(_x);
    }
};

TORCH_MODULE(UNet3D);

#endif