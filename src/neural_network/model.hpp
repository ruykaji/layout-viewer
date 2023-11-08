#ifndef __MODEL_H__
#define __MODEL_H__

#include "data.hpp"
#include "encoders.hpp"

// GNN model
// ======================================================================================

struct GNNImpl : torch::nn::Module {
    torch::nn::Linear lin1 { nullptr };
    torch::nn::Linear lin2 { nullptr };
    torch::nn::Linear lin3 { nullptr };
    torch::nn::ReLU relu { nullptr };

    GNNImpl() = default;

    GNNImpl(int size)
    {
        lin1 = torch::nn::Linear(1, size);
        lin2 = torch::nn::Linear(1, size);
        lin3 = torch::nn::Linear(size, size);
        relu = torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true));

        register_module("lin1", lin1);
        register_module("lin2", lin2);
        register_module("lin3", lin3);
    };

    torch::Tensor forward(torch::Tensor t_nets)
    {
        torch::Tensor q = lin1->forward(t_nets);
        torch::Tensor k = lin2->forward(t_nets);
        torch::Tensor x = lin3->forward(relu(torch::matmul(q.transpose(-2, -1), k)));

        return x;
    }
};

TORCH_MODULE(GNN);

// UNet model
// ======================================================================================

struct DecoderBlockImpl : torch::nn::Module {
    torch::nn::Sequential conv1 { nullptr };
    torch::nn::Sequential conv2 { nullptr };

    DecoderBlockImpl(int in_channels, int skip_channels, int out_channels)
    {
        conv1 = torch::nn::Sequential(
            torch::nn::Conv2d(torch::nn::Conv2dOptions(in_channels + skip_channels, out_channels, 3).padding(1).bias(false)),
            torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true)));

        conv2 = torch::nn::Sequential(
            torch::nn::Conv2d(torch::nn::Conv2dOptions(out_channels, out_channels, 3).padding(1).bias(false)),
            torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true)));

        register_module("conv1", conv1);
        register_module("conv2", conv2);
    }

    torch::Tensor forward(torch::Tensor x, torch::Tensor skip = torch::Tensor())
    {
        auto options = torch::nn::functional::InterpolateFuncOptions().scale_factor(std::vector<double>({ 2.0, 2.0 })).mode(torch::kBilinear).align_corners(true);

        x = torch::nn::functional::interpolate(x, options);

        if (skip.defined()) {
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

    SegmentationHeadImpl(int in_channels, int out_channels, int kernel_size = 3)
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
    // GNN gnn;
    ResNet50Backbone encoder { nullptr };
    SegmentationHead segmentationHead { nullptr };

    std::vector<DecoderBlock> decoderBlocks {};

    UNetImpl()
    {
        encoder = ResNet50Backbone("./resnet50_model.pt");

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
            decoderBlocks.emplace_back(DecoderBlock(inChannels[i], skipChannels[i], outChannels[i]));

            register_module("decoder_block_" + std::to_string(i), decoderBlocks.back());
        }

        segmentationHead = SegmentationHead(decoderChannels.back(), 10);

        register_module("encoder", encoder);
        register_module("segmentationHead", segmentationHead);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        // auto gx = torch::cat({ x, gnn->forward(t_nets) }, 1);

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

#endif