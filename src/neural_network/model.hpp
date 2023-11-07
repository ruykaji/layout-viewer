#ifndef __MODEL_H__
#define __MODEL_H__

#include "data.hpp"

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

// Backbone
// ======================================================================================

struct BottleneckImpl : torch::nn::Module {
    torch::nn::Conv2d conv1 { nullptr };
    torch::nn::Conv2d conv2 { nullptr };
    torch::nn::Conv2d conv3 { nullptr };
    torch::nn::Conv2d downsample_conv { nullptr };

    torch::nn::BatchNorm2d bn1 { nullptr };
    torch::nn::BatchNorm2d bn2 { nullptr };
    torch::nn::BatchNorm2d bn3 { nullptr };
    torch::nn::BatchNorm2d downsample_bn { nullptr };

    torch::nn::ReLU relu { nullptr };

    bool isDownsample { false };

    BottleneckImpl(int32_t in_channels, int32_t mid_channels, int32_t out_channels, int32_t t_stride = 1, bool t_downsample = false)
    {
        conv1 = torch::nn::Conv2d(torch::nn::Conv2dOptions(in_channels, mid_channels, 1).stride(1).bias(false));
        bn1 = torch::nn::BatchNorm2d(mid_channels);
        conv2 = torch::nn::Conv2d(torch::nn::Conv2dOptions(mid_channels, mid_channels, 3).stride(t_stride).padding(1).bias(false));
        bn2 = torch::nn::BatchNorm2d(mid_channels);
        conv3 = torch::nn::Conv2d(torch::nn::Conv2dOptions(mid_channels, out_channels, 1).stride(1).bias(false));
        bn3 = torch::nn::BatchNorm2d(out_channels);
        relu = torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true));

        if (t_downsample) {
            downsample_conv = torch::nn::Conv2d(torch::nn::Conv2dOptions(in_channels, out_channels, 1).stride(t_stride).bias(false));
            downsample_bn = torch::nn::BatchNorm2d(out_channels);
        }

        register_module("conv1", conv1);
        register_module("bn1", bn1);
        register_module("conv2", conv2);
        register_module("bn2", bn2);
        register_module("conv3", conv3);
        register_module("bn3", bn3);
        register_module("relu", relu);

        if (t_downsample) {
            register_module("downsample_conv", downsample_conv);
            register_module("downsample_bn", downsample_bn);
        }

        isDownsample = t_downsample;
    }

    torch::Tensor forward(torch::Tensor x)
    {
        torch::Tensor identity = x.clone();

        x = relu(bn1(conv1(x)));
        x = relu(bn2(conv2(x)));
        x = bn3(conv3(x));

        if (isDownsample) {
            identity = downsample_bn(downsample_conv(identity));
        }

        x = x + identity;
        x = relu(x);

        return x;
    }
};

TORCH_MODULE(Bottleneck);

struct ResNet50BackboneImpl : torch::nn::Module {
    torch::nn::Conv2d conv1 { nullptr };
    torch::nn::BatchNorm2d bn1 { nullptr };
    torch::nn::ReLU relu { nullptr };

    torch::nn::Sequential layer1 { nullptr };
    torch::nn::Sequential layer2 { nullptr };
    torch::nn::Sequential layer3 { nullptr };
    torch::nn::Sequential layer4 { nullptr };

    ResNet50BackboneImpl() = default;

    ResNet50BackboneImpl(const std::string& t_modelPath)
    {
        torch::jit::script::Module jitModel = torch::jit::load(t_modelPath);

        conv1 = torch::nn::Conv2d(torch::nn::Conv2dOptions(11, 64, 7).stride(2).padding(3).bias(false));

        for (const auto& pair : jitModel.named_parameters()) {
            if (pair.name == "conv1.weight") {
                auto convWeight = pair.value.clone();
                auto repeated_weights = convWeight.repeat({ 1, static_cast<int32_t>(std::ceil(11.0 / convWeight.size(1))), 1, 1 });
                auto selected_weights = repeated_weights.index_select(1, torch::arange(11));

                conv1->named_parameters()["weight"].set_data(selected_weights);
            }
        }

        bn1 = torch::nn::BatchNorm2d(64);

        for (const auto& pair : jitModel.named_parameters()) {
            if (pair.name == "bn1.weight") {
                (*bn1).named_parameters()["weight"] = pair.value.clone();
            }

            if (pair.name == "bn1.bias") {
                (*bn1).named_parameters()["bias"] = pair.value.clone();
            }
        }

        relu = torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true));

        auto layer1Bottleneck0 = Bottleneck(64, 64, 256, 1, true);
        auto layer1Bottleneck1 = Bottleneck(256, 64, 256);
        auto layer1Bottleneck2 = Bottleneck(256, 64, 256);

        loadWeightsForBottleneck(layer1Bottleneck0, jitModel, "layer1.0");
        loadWeightsForBottleneck(layer1Bottleneck1, jitModel, "layer1.1");
        loadWeightsForBottleneck(layer1Bottleneck2, jitModel, "layer1.2");

        layer1 = torch::nn::Sequential(layer1Bottleneck0, layer1Bottleneck1, layer1Bottleneck2);

        auto layer2Bottleneck0 = Bottleneck(256, 128, 512, 2, true);
        auto layer2Bottleneck1 = Bottleneck(512, 128, 512);
        auto layer2Bottleneck2 = Bottleneck(512, 128, 512);
        auto layer2Bottleneck3 = Bottleneck(512, 128, 512);

        loadWeightsForBottleneck(layer2Bottleneck0, jitModel, "layer2.0");
        loadWeightsForBottleneck(layer2Bottleneck1, jitModel, "layer2.1");
        loadWeightsForBottleneck(layer2Bottleneck2, jitModel, "layer2.2");
        loadWeightsForBottleneck(layer2Bottleneck3, jitModel, "layer2.3");

        layer2 = torch::nn::Sequential(layer2Bottleneck0, layer2Bottleneck1, layer2Bottleneck2, layer2Bottleneck3);

        auto layer3Bottleneck0 = Bottleneck(512, 256, 1024, 2, true);
        auto layer3Bottleneck1 = Bottleneck(1024, 256, 1024);
        auto layer3Bottleneck2 = Bottleneck(1024, 256, 1024);
        auto layer3Bottleneck3 = Bottleneck(1024, 256, 1024);
        auto layer3Bottleneck4 = Bottleneck(1024, 256, 1024);
        auto layer3Bottleneck5 = Bottleneck(1024, 256, 1024);

        loadWeightsForBottleneck(layer3Bottleneck0, jitModel, "layer3.0");
        loadWeightsForBottleneck(layer3Bottleneck1, jitModel, "layer3.1");
        loadWeightsForBottleneck(layer3Bottleneck2, jitModel, "layer3.2");
        loadWeightsForBottleneck(layer3Bottleneck3, jitModel, "layer3.3");
        loadWeightsForBottleneck(layer3Bottleneck4, jitModel, "layer3.4");
        loadWeightsForBottleneck(layer3Bottleneck5, jitModel, "layer3.5");

        layer3 = torch::nn::Sequential(layer3Bottleneck0, layer3Bottleneck1, layer3Bottleneck2, layer3Bottleneck3, layer3Bottleneck4, layer3Bottleneck5);

        auto layer4Bottleneck0 = Bottleneck(1024, 512, 2048, 2, true);
        auto layer4Bottleneck1 = Bottleneck(2048, 512, 2048);
        auto layer4Bottleneck2 = Bottleneck(2048, 512, 2048);

        loadWeightsForBottleneck(layer4Bottleneck0, jitModel, "layer4.0");
        loadWeightsForBottleneck(layer4Bottleneck1, jitModel, "layer4.1");
        loadWeightsForBottleneck(layer4Bottleneck2, jitModel, "layer4.2");

        layer4 = torch::nn::Sequential(layer4Bottleneck0, layer4Bottleneck1, layer4Bottleneck2);

        register_module("conv1", conv1);
        register_module("bn1", bn1);
        register_module("layer1", layer1);
        register_module("layer2", layer2);
        register_module("layer3", layer3);
        register_module("layer4", layer4);
    };

    std::vector<torch::Tensor> forward(torch::Tensor x)
    {
        std::vector<torch::Tensor> features {};

        x = conv1->forward(x);

        features.emplace_back(x);

        x = bn1->forward(x);
        x = relu->forward(x);
        x = torch::nn::functional::max_pool2d(x, torch::nn::functional::MaxPool2dFuncOptions(3).stride(2).padding(1).dilation(1).ceil_mode(false));

        features.emplace_back(layer1->forward(x));
        features.emplace_back(layer2->forward(features.back()));
        features.emplace_back(layer3->forward(features.back()));
        features.emplace_back(layer4->forward(features.back()));

        return features;
    }

    void loadWeightsForBottleneck(Bottleneck& t_bottleneck, const torch::jit::script::Module& t_jitModel, const std::string& t_layerName)
    {
        static auto load_parameters = [&](const std::string& t_name, torch::nn::Module& t_module) {
            if (t_module.named_parameters().contains("weight")) {
                for (const auto& pair : t_jitModel.named_parameters()) {
                    if (pair.name == t_layerName + "." + t_name + ".weight") {
                        t_module.named_parameters()["weight"] = pair.value.clone();
                    }
                }
            }

            if (t_module.named_parameters().contains("bias")) {
                for (const auto& pair : t_jitModel.named_parameters()) {

                    if (pair.name == t_layerName + "." + t_name + ".bias") {
                        t_module.named_parameters()["bias"] = pair.value.clone();
                    }
                }
            }
        };

        load_parameters("conv1", *t_bottleneck->conv1);
        load_parameters("bn1", *t_bottleneck->bn1);
        load_parameters("conv2", *t_bottleneck->conv2);
        load_parameters("bn2", *t_bottleneck->bn2);
        load_parameters("conv3", *t_bottleneck->conv3);
        load_parameters("bn3", *t_bottleneck->bn3);

        if (t_bottleneck->isDownsample) {
            load_parameters("downsample.0", *t_bottleneck->downsample_conv);
            load_parameters("downsample.1", *t_bottleneck->downsample_bn);
        }
    }
};

TORCH_MODULE(ResNet50Backbone);

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

    torch::Tensor forward(torch::Tensor x, torch::Tensor t_nets)
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