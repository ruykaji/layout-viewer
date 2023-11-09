#ifndef __EFFICIENTNETB3_H__
#define __EFFICIENTNETB3_H__

#include <cmath>
#include <tuple>

#include "torch_include.hpp"

struct SwishImplementation : torch::autograd::Function<SwishImplementation> {
    static torch::autograd::variable_list forward(torch::autograd::AutogradContext* ctx, torch::autograd::Variable input)
    {
        ctx->save_for_backward({ input });

        auto result = input * torch::sigmoid(input);

        return { result };
    }

    static torch::autograd::variable_list backward(torch::autograd::AutogradContext* ctx, torch::autograd::variable_list grad_outputs)
    {
        auto saved = ctx->get_saved_variables();
        auto input = saved[0];

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

inline int64_t calculate_output_image_size(int64_t input_image_size, int64_t stride)
{
    return static_cast<int64_t>(std::ceil(static_cast<double>(input_image_size) / stride));
}

struct MBConvBlockImpl : torch::nn::Module {
    int64_t input_filters;
    int64_t output_filters;
    int64_t stride;
    int64_t expand_ratio;
    torch::nn::BatchNorm2d bn0 { nullptr }, bn1 { nullptr }, bn2 { nullptr };
    Conv2dStaticSamePadding expand_conv { nullptr }, se_reduce { nullptr }, se_expand { nullptr }, depthwise_conv { nullptr }, project_conv { nullptr };
    MemoryEfficientSwish swish;

    MBConvBlockImpl(int64_t input_filters, int64_t output_filters, int64_t kernel_size, int64_t stride, int64_t expand_ratio, int64_t se_ratio, int64_t image_size)
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
            auto reduced_dim = std::max(static_cast<int64_t>(1), input_filters / se_ratio);
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
            x = torch::nn::functional::dropout2d(x, torch::nn::functional::Dropout2dFuncOptions().p(0.3).training(is_training()));
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

    EfficientNetB3Impl()
    {
        int64_t image_size = 300;

        stem_conv = Conv2dStaticSamePadding(11, 40, 3, 2, false, image_size);
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

        register_module("stem_conv", stem_conv);
        register_module("stem_bn", stem_bn);

        register_module("swish", swish);
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

        return features;
    }
};

TORCH_MODULE(EfficientNetB3);

#endif