#ifndef __RESNET50_H__
#define __RESNET50_H__

#include "torch_include.hpp"

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

    ResNet50BackboneImpl()
    {
        conv1 = torch::nn::Conv2d(torch::nn::Conv2dOptions(11, 64, 7).stride(2).padding(3).bias(false));
        bn1 = torch::nn::BatchNorm2d(64);
        relu = torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true));

        layer1 = torch::nn::Sequential(Bottleneck(64, 64, 256, 1, true), Bottleneck(256, 64, 256), Bottleneck(256, 64, 256));
        layer2 = torch::nn::Sequential(Bottleneck(256, 128, 512, 2, true), Bottleneck(512, 128, 512), Bottleneck(512, 128, 512), Bottleneck(512, 128, 512));
        layer3 = torch::nn::Sequential(Bottleneck(512, 256, 1024, 2, true), Bottleneck(1024, 256, 1024), Bottleneck(1024, 256, 1024), Bottleneck(1024, 256, 1024), Bottleneck(1024, 256, 1024), Bottleneck(1024, 256, 1024));
        layer4 = torch::nn::Sequential(Bottleneck(1024, 512, 2048, 2, true), Bottleneck(2048, 512, 2048), Bottleneck(2048, 512, 2048));

        register_module("conv1", conv1);
        register_module("bn1", bn1);
        register_module("layer1", layer1);
        register_module("layer2", layer2);
        register_module("layer3", layer3);
        register_module("layer4", layer3);
    };

    std::vector<torch::Tensor> forward(torch::Tensor x)
    {
        std::vector<torch::Tensor> features {};

        x = conv1->forward(x);
        x = bn1->forward(x);
        x = relu->forward(x);

        features.emplace_back(x);

        x = torch::nn::functional::max_pool2d(x, torch::nn::functional::MaxPool2dFuncOptions(3).stride(2).padding(1).dilation(1).ceil_mode(false));

        features.emplace_back(layer1->forward(x));
        features.emplace_back(layer2->forward(features.back()));
        features.emplace_back(layer3->forward(features.back()));
        features.emplace_back(layer4->forward(features.back()));

        return features;
    }
};

TORCH_MODULE(ResNet50Backbone);

#endif