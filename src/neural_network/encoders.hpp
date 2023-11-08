#ifndef __ENCODERS_H__
#define __ENCODERS_H__

#include <cmath>
#include <tuple>

#include "torch_include.hpp"

// ResNet50
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

private:
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

// EfficientNet
// ======================================================================================

struct SwishImplementation : torch::autograd::Function<SwishImplementation> {
    static torch::autograd::variable_list forward(torch::autograd::AutogradContext* ctx, torch::autograd::Variable input)
    {
        // Save input for backward pass
        ctx->save_for_backward({ input });

        // Apply the Swish function
        auto result = input * torch::sigmoid(input);

        return { result };
    }

    // Backward pass
    static torch::autograd::variable_list backward(torch::autograd::AutogradContext* ctx, torch::autograd::variable_list grad_outputs)
    {
        // Retrieve saved input
        auto saved = ctx->get_saved_variables();
        auto input = saved[0];

        // Compute the gradients
        auto sigmoid_i = torch::sigmoid(input);
        auto grad_input = grad_outputs[0] * (sigmoid_i * (1 + input * (1 - sigmoid_i)));

        return { grad_input };
    }
};

struct MemoryEfficientSwishImpl : torch::nn::Module {
    torch::Tensor forward(torch::Tensor input)
    {
        return SwishImplementation::apply(input)[0];
    }
};

TORCH_MODULE(MemoryEfficientSwish);

struct Conv2dStaticSamePaddingImpl : torch::nn::Conv2dImpl {
    torch::nn::Functional static_padding { nullptr };

    Conv2dStaticSamePaddingImpl(int64_t in_channels, int64_t out_channels, int64_t kernel_size, int64_t stride = 1, bool bias = true, c10::optional<int64_t> image_size = c10::nullopt, int64_t groups = 1)
        : Conv2dImpl(torch::nn::Conv2dOptions(in_channels, out_channels, kernel_size).groups(groups).stride(stride).bias(bias))
    {
        if (!image_size.has_value()) {
            throw std::invalid_argument("image_size must be specified");
        }

        int64_t ih, iw;
        ih = iw = image_size.value();

        auto kh = options.kernel_size()->at(0);
        auto kw = options.kernel_size()->at(1);
        auto sh = options.stride()->at(0);
        auto sw = options.stride()->at(1);
        auto oh = static_cast<int64_t>(std::ceil(static_cast<float>(ih) / sh));
        auto ow = static_cast<int64_t>(std::ceil(static_cast<float>(iw) / sw));
        auto pad_h = std::max((oh - 1) * sh + (kh - 1) * options.dilation()->at(0) + 1 - ih, static_cast<int64_t>(0));
        auto pad_w = std::max((ow - 1) * sw + (kw - 1) * options.dilation()->at(1) + 1 - iw, static_cast<int64_t>(0));

        if (pad_h > 0 || pad_w > 0) {
            static_padding = register_module("static_padding", torch::nn::Functional(torch::nn::functional::pad, torch::nn::functional::PadFuncOptions({ pad_w / 2, pad_w - pad_w / 2, pad_h / 2, pad_h - pad_h / 2 }).mode(torch::kConstant).value(0)));
        } else {
            static_padding = register_module("static_padding", torch::nn::Functional([](const torch::Tensor& input) { return input; }));
        }
    }

    torch::Tensor forward(torch::Tensor x)
    {
        x = static_padding(x);
        return torch::nn::Conv2dImpl::forward(x);
    }
};

TORCH_MODULE(Conv2dStaticSamePadding);

inline int32_t calculate_output_image_size(int32_t input_image_size, int32_t stride)
{
    return static_cast<int32_t>(std::ceil(static_cast<double>(input_image_size) / stride));
}

struct MBConvBlockImpl : torch::nn::Module {
    int64_t input_filters;
    int64_t output_filters;
    int64_t stride;
    int64_t expand_ratio;
    torch::nn::BatchNorm2d bn0 { nullptr }, bn1 { nullptr }, bn2 { nullptr };
    Conv2dStaticSamePadding expand_conv { nullptr }, se_reduce { nullptr }, se_expand { nullptr }, depthwise_conv { nullptr }, project_conv { nullptr };
    MemoryEfficientSwish swish;

    MBConvBlockImpl(int32_t input_filters, int32_t output_filters, int32_t kernel_size, int32_t stride, int32_t expand_ratio, int32_t se_ratio, int32_t image_size)
        : input_filters(input_filters)
        , output_filters(output_filters)
        , expand_ratio(expand_ratio)
        , stride(stride)
    {
        if (expand_ratio != 1) {
            expand_conv = Conv2dStaticSamePadding(input_filters, input_filters * expand_ratio, 1, 1, false, image_size);
            bn0 = torch::nn::BatchNorm2d(torch::nn::BatchNorm2dOptions(input_filters * expand_ratio).eps(0.001).momentum(0.01).affine(true).track_running_stats(true));
        }

        depthwise_conv = Conv2dStaticSamePadding(input_filters * expand_ratio, input_filters * expand_ratio, kernel_size, stride, false, image_size, input_filters * expand_ratio);
        bn1 = torch::nn::BatchNorm2d(torch::nn::BatchNorm2dOptions(input_filters * expand_ratio).eps(0.001).momentum(0.01).affine(true).track_running_stats(true));

        if (se_ratio > 0) {
            auto reduced_dim = std::max(1, input_filters / se_ratio);
            se_reduce = Conv2dStaticSamePadding(input_filters * expand_ratio, reduced_dim, 1, 1, true, 1);
            se_expand = Conv2dStaticSamePadding(reduced_dim, input_filters * expand_ratio, 1, 1, true, 1);
        }

        project_conv = Conv2dStaticSamePadding(input_filters * expand_ratio, output_filters, 1, 1, false, calculate_output_image_size(image_size, stride));
        bn2 = torch::nn::BatchNorm2d(torch::nn::BatchNorm2dOptions(output_filters).eps(0.001).momentum(0.01).affine(true).track_running_stats(true));

        swish = MemoryEfficientSwish();

        if (expand_ratio != 1) {
            register_module("expand_conv", expand_conv);
            register_module("bn0", bn0);
        }

        register_module("depthwise_conv", depthwise_conv);
        register_module("bn1", bn1);

        if (se_ratio > 0) {
            register_module("se_reduce", se_reduce);
            register_module("se_expand", se_expand);
        }

        register_module("project_conv", project_conv);
        register_module("bn2", bn2);
        register_module("swish", swish);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        torch::Tensor out;

        if (expand_ratio != 1) {
            out = swish(bn0(expand_conv(x)));
        } else {
            out = x;
        }

        out = swish(bn1(depthwise_conv(out)));

        if (se_reduce && se_expand) {
            auto out_squeezed = torch::nn::functional::adaptive_avg_pool2d(out, torch::nn::functional::AdaptiveAvgPool2dFuncOptions(1));
            out_squeezed = se_reduce->forward(out_squeezed);
            out_squeezed = swish(out_squeezed);
            out_squeezed = se_expand->forward(out_squeezed);
            out = torch::sigmoid(out_squeezed) * out;
        }

        out = bn2(project_conv(out));

        if (input_filters == output_filters && stride == 1) {
            out = out + x;
        }

        return out;
    }
};

TORCH_MODULE(MBConvBlock);

struct EfficientNetB3Impl : torch::nn::Module {
    Conv2dStaticSamePadding stem_conv { nullptr };
    torch::nn::BatchNorm2d stem_bn { nullptr };

    std::vector<MBConvBlock> blocks {};

    Conv2dStaticSamePadding head_conv { nullptr };
    torch::nn::BatchNorm2d head_bn { nullptr };

    MemoryEfficientSwish swish;

    torch::nn::AdaptiveAvgPool2d avgpool { nullptr };
    torch::nn::Dropout dropout;
    torch::nn::Linear fc { nullptr };

    EfficientNetB3Impl(int64_t num_classes)
    {
        int32_t image_size = 300;

        stem_conv = Conv2dStaticSamePadding(3, 40, 3, 2, false, image_size);
        stem_bn = torch::nn::BatchNorm2d(40);

        image_size = calculate_output_image_size(image_size, 2);

        blocks.emplace_back(MBConvBlock(40, 24, 3, 1, 1, 4, image_size)); // block 0
        blocks.emplace_back(MBConvBlock(24, 24, 3, 1, 1, 4, image_size)); // block 1
        blocks.emplace_back(MBConvBlock(24, 32, 3, 2, 6, 24, image_size)); // block 2

        image_size = calculate_output_image_size(image_size, 2);

        blocks.emplace_back(MBConvBlock(32, 32, 3, 1, 6, 24, image_size)); // block 3
        blocks.emplace_back(MBConvBlock(32, 32, 3, 1, 6, 24, image_size)); // block 4
        blocks.emplace_back(MBConvBlock(32, 48, 5, 2, 6, 24, image_size)); // block 5

        image_size = calculate_output_image_size(image_size, 2);

        blocks.emplace_back(MBConvBlock(48, 48, 5, 1, 6, 24, image_size)); // block 6
        blocks.emplace_back(MBConvBlock(48, 48, 5, 1, 6, 24, image_size)); // block 7
        blocks.emplace_back(MBConvBlock(48, 96, 3, 2, 6, 24, image_size)); // block 8

        image_size = calculate_output_image_size(image_size, 2);

        blocks.emplace_back(MBConvBlock(96, 96, 3, 1, 6, 24, image_size)); // block 9
        blocks.emplace_back(MBConvBlock(96, 96, 3, 1, 6, 24, image_size)); // block 10
        blocks.emplace_back(MBConvBlock(96, 96, 3, 1, 6, 24, image_size)); // block 11
        blocks.emplace_back(MBConvBlock(96, 96, 3, 1, 6, 24, image_size)); // block 12
        blocks.emplace_back(MBConvBlock(96, 136, 5, 1, 6, 24, image_size)); // block 13
        blocks.emplace_back(MBConvBlock(136, 136, 5, 1, 6, 24, image_size)); // block 14
        blocks.emplace_back(MBConvBlock(136, 136, 5, 1, 6, 24, image_size)); // block 15
        blocks.emplace_back(MBConvBlock(136, 136, 5, 1, 6, 24, image_size)); // block 16
        blocks.emplace_back(MBConvBlock(136, 136, 5, 1, 6, 24, image_size)); // block 17

        image_size = calculate_output_image_size(image_size, 2);

        blocks.emplace_back(MBConvBlock(136, 232, 5, 2, 6, 24, image_size)); // block 18
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, image_size)); // block 19
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, image_size)); // block 20
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, image_size)); // block 21
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, image_size)); // block 22
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, image_size)); // block 23
        blocks.emplace_back(MBConvBlock(232, 384, 3, 1, 6, 24, image_size)); // block 24
        blocks.emplace_back(MBConvBlock(384, 384, 3, 1, 6, 24, image_size)); // block 25

        for (std::size_t i = 0; i < blocks.size(); ++i) {
            register_module("block_" + std::to_string(i), blocks[i]);
        }

        // Head Convolution
        head_conv = Conv2dStaticSamePadding(384, 1536, 1, 1, false, 300); // Example for the last block's output size
        head_bn = torch::nn::BatchNorm2d(1536);

        swish = MemoryEfficientSwish();

        // Pooling and final dense layer
        avgpool = torch::nn::AdaptiveAvgPool2d(torch::nn::AdaptiveAvgPool2dOptions({ 1, 1 }));
        dropout = torch::nn::Dropout(0.3); // Dropout rate example
        fc = torch::nn::Linear(1536, num_classes);

        // Register modules
        register_module("stem_conv", stem_conv);
        register_module("stem_bn", stem_bn);

        register_module("head_conv", head_conv);
        register_module("head_bn", head_bn);

        register_module("swish", swish);

        register_module("avgpool", avgpool);
        register_module("dropout", dropout);
        register_module("fc", fc);
    }

    std::vector<torch::Tensor> forward(torch::Tensor x)
    {
        std::vector<torch::Tensor> features {};

        x = stem_bn(stem_conv(x));

        features.emplace_back(x);

        x = swish(x);

        for (std::size_t i = 0; i < blocks.size(); ++i) {
            x = blocks[i]->forward(x);

            if (i == 4 || i == 7 || i == 17 || i == 25) {
                features.emplace_back(x);
            }
        }

        x = swish(head_bn(head_conv(x)));

        x = avgpool(x);

        x = torch::flatten(x, 1);
        x = dropout(x);
        x = fc(x);

        return features;
    }
};

TORCH_MODULE(EfficientNetB3);

#endif