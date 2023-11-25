#ifndef __EFFICIENTNETB3_H__
#define __EFFICIENTNETB3_H__

#include <cmath>
#include <tuple>

#include "torch_include.hpp"

struct SwishImplementation : torch::autograd::Function<SwishImplementation> {
    static torch::autograd::variable_list forward(torch::autograd::AutogradContext* t_ctx, torch::autograd::Variable t_input)
    {
        t_ctx->save_for_backward({ t_input });

        auto result = t_input * torch::sigmoid(t_input);

        return { result };
    }

    static torch::autograd::variable_list backward(torch::autograd::AutogradContext* t_ctx, torch::autograd::variable_list t_gradOutputs)
    {
        auto saved = t_ctx->get_saved_variables();
        auto input = saved[0];

        auto sigmoid_i = torch::sigmoid(input);
        auto grad_input = t_gradOutputs[0] * (sigmoid_i * (1 + input * (1 - sigmoid_i)));

        return { grad_input };
    }
};

struct MemoryEfficientSwishImpl : torch::nn::Module {
    torch::Tensor forward(torch::Tensor t_input)
    {
        return SwishImplementation::apply(t_input)[0];
    }
};

TORCH_MODULE(MemoryEfficientSwish);

struct Conv2dStaticSamePaddingImpl : torch::nn::Conv2dImpl {
    torch::nn::Functional staticPadding { nullptr };

    Conv2dStaticSamePaddingImpl(int64_t t_inChannels, int64_t t_outChannels, int64_t t_kernelSize, int64_t t_stride = 1, bool t_bias = true, c10::optional<int64_t> t_imageSize = c10::nullopt, int64_t t_groups = 1)
        : Conv2dImpl(torch::nn::Conv2dOptions(t_inChannels, t_outChannels, t_kernelSize).groups(t_groups).stride(t_stride).bias(t_bias))
    {
        if (!t_imageSize.has_value()) {
            throw std::invalid_argument("image size must be specified");
        }

        int64_t ih, iw;

        ih = iw = t_imageSize.value();

        auto kh = options.kernel_size()->at(0);
        auto kw = options.kernel_size()->at(1);

        auto sh = options.stride()->at(0);
        auto sw = options.stride()->at(1);

        auto oh = static_cast<int64_t>(std::ceil(static_cast<float>(ih) / sh));
        auto ow = static_cast<int64_t>(std::ceil(static_cast<float>(iw) / sw));

        auto pad_h = std::max((oh - 1) * sh + (kh - 1) * options.dilation()->at(0) + 1 - ih, static_cast<int64_t>(0));
        auto pad_w = std::max((ow - 1) * sw + (kw - 1) * options.dilation()->at(1) + 1 - iw, static_cast<int64_t>(0));

        if (pad_h > 0 || pad_w > 0) {
            staticPadding = register_module("staticPadding", torch::nn::Functional(torch::nn::functional::pad, torch::nn::functional::PadFuncOptions({ pad_w / 2, pad_w - pad_w / 2, pad_h / 2, pad_h - pad_h / 2 }).mode(torch::kConstant).value(0)));
        } else {
            staticPadding = register_module("staticPadding", torch::nn::Functional([](const torch::Tensor& input) { return input; }));
        }
    }

    torch::Tensor forward(torch::Tensor x)
    {
        x = staticPadding(x);
        return torch::nn::Conv2dImpl::forward(x);
    }
};

TORCH_MODULE(Conv2dStaticSamePadding);

inline int64_t calculate_output_image_size(int64_t t_inputImageSize, int64_t t_stride)
{
    return static_cast<int64_t>(std::ceil(static_cast<double>(t_inputImageSize) / t_stride));
}

struct MBConvBlockImpl : torch::nn::Module {
    int64_t inputFilters;
    int64_t outputFilters;
    int64_t stride;
    int64_t expandRatio;
    torch::nn::BatchNorm2d bn0 { nullptr }, bn1 { nullptr }, bn2 { nullptr };
    Conv2dStaticSamePadding expandConv { nullptr }, seReduce { nullptr }, seExpand { nullptr }, depthwiseConv { nullptr }, projectConv { nullptr };
    MemoryEfficientSwish swish;

    MBConvBlockImpl(int64_t t_inputFilters, int64_t t_outputFilters, int64_t t_kernel_size, int64_t t_stride, int64_t t_expandRatio, int64_t t_seRatio, int64_t t_imageSize)
        : inputFilters(t_inputFilters)
        , outputFilters(t_outputFilters)
        , expandRatio(t_expandRatio)
        , stride(t_stride)
    {
        if (expandRatio != 1) {
            expandConv = Conv2dStaticSamePadding(inputFilters, inputFilters * expandRatio, 1, 1, false, t_imageSize);
            bn0 = torch::nn::BatchNorm2d(torch::nn::BatchNorm2dOptions(inputFilters * expandRatio).eps(0.001).momentum(0.01).affine(true).track_running_stats(true));
        }

        depthwiseConv = Conv2dStaticSamePadding(inputFilters * expandRatio, inputFilters * expandRatio, t_kernel_size, stride, false, t_imageSize, inputFilters * expandRatio);
        bn1 = torch::nn::BatchNorm2d(torch::nn::BatchNorm2dOptions(inputFilters * expandRatio).eps(0.001).momentum(0.01).affine(true).track_running_stats(true));

        if (t_seRatio > 0) {
            auto reduced_dim = std::max(static_cast<int64_t>(1), inputFilters / t_seRatio);
            seReduce = Conv2dStaticSamePadding(inputFilters * expandRatio, reduced_dim, 1, 1, true, 1);
            seExpand = Conv2dStaticSamePadding(reduced_dim, inputFilters * expandRatio, 1, 1, true, 1);
        }

        projectConv = Conv2dStaticSamePadding(inputFilters * expandRatio, outputFilters, 1, 1, false, calculate_output_image_size(t_imageSize, stride));
        bn2 = torch::nn::BatchNorm2d(torch::nn::BatchNorm2dOptions(outputFilters).eps(0.001).momentum(0.01).affine(true).track_running_stats(true));

        swish = MemoryEfficientSwish();

        if (expandRatio != 1) {
            register_module("expandConv", expandConv);
            register_module("bn0", bn0);
        }

        register_module("depthwiseConv", depthwiseConv);
        register_module("bn1", bn1);

        if (t_seRatio > 0) {
            register_module("seReduce", seReduce);
            register_module("seExpand", seExpand);
        }

        register_module("projectConv", projectConv);
        register_module("bn2", bn2);
        register_module("swish", swish);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        torch::Tensor out;

        if (expandRatio != 1) {
            out = swish(bn0(expandConv(x)));
        } else {
            out = x;
        }

        out = swish(bn1(depthwiseConv(out)));

        if (seReduce && seExpand) {
            auto outSqueezed = torch::nn::functional::adaptive_avg_pool2d(out, torch::nn::functional::AdaptiveAvgPool2dFuncOptions(1));
            outSqueezed = seReduce->forward(outSqueezed);
            outSqueezed = swish(outSqueezed);
            outSqueezed = seExpand->forward(outSqueezed);
            out = torch::sigmoid(outSqueezed) * out;
        }

        out = bn2(projectConv(out));

        if (inputFilters == outputFilters && stride == 1) {
            x = torch::nn::functional::dropout2d(x, torch::nn::functional::Dropout2dFuncOptions().p(0.3).training(is_training()));
            out = out + x;
        }

        return out;
    }
};

TORCH_MODULE(MBConvBlock);

struct EfficientNetB3Impl : torch::nn::Module {
    Conv2dStaticSamePadding stemConv { nullptr };
    torch::nn::BatchNorm2d stemBn { nullptr };

    std::vector<MBConvBlock> blocks {};

    MemoryEfficientSwish swish;

    // Conv2dStaticSamePadding headConv { nullptr };
    // torch::nn::BatchNorm2d headBn { nullptr };

    // torch::nn::AdaptiveAvgPool2d avgpool { nullptr };
    // torch::nn::Dropout dropout { nullptr };
    torch::nn::Linear fc { nullptr };

    EfficientNetB3Impl(const int64_t t_outputSize)
    {
        int64_t imageSize = 300;

        register_module("swish", swish);

        stemConv = Conv2dStaticSamePadding(5, 40, 3, 2, false, imageSize);
        register_module("stemConv", stemConv);

        stemBn = torch::nn::BatchNorm2d(40);
        register_module("stemBn", stemBn);

        imageSize = calculate_output_image_size(imageSize, 2);

        blocks.emplace_back(MBConvBlock(40, 24, 3, 1, 1, 4, imageSize)); // block 0
        blocks.emplace_back(MBConvBlock(24, 24, 3, 1, 1, 4, imageSize)); // block 1
        blocks.emplace_back(MBConvBlock(24, 32, 3, 2, 6, 24, imageSize)); // block 2

        imageSize = calculate_output_image_size(imageSize, 2);

        blocks.emplace_back(MBConvBlock(32, 32, 3, 1, 6, 24, imageSize)); // block 3
        blocks.emplace_back(MBConvBlock(32, 32, 3, 1, 6, 24, imageSize)); // block 4
        blocks.emplace_back(MBConvBlock(32, 48, 5, 2, 6, 24, imageSize)); // block 5

        imageSize = calculate_output_image_size(imageSize, 2);

        blocks.emplace_back(MBConvBlock(48, 48, 5, 1, 6, 24, imageSize)); // block 6
        blocks.emplace_back(MBConvBlock(48, 48, 5, 1, 6, 24, imageSize)); // block 7
        blocks.emplace_back(MBConvBlock(48, 96, 3, 2, 6, 24, imageSize)); // block 8

        imageSize = calculate_output_image_size(imageSize, 2);

        blocks.emplace_back(MBConvBlock(96, 96, 3, 1, 6, 24, imageSize)); // block 9
        blocks.emplace_back(MBConvBlock(96, 96, 3, 1, 6, 24, imageSize)); // block 10
        blocks.emplace_back(MBConvBlock(96, 96, 3, 1, 6, 24, imageSize)); // block 11
        blocks.emplace_back(MBConvBlock(96, 96, 3, 1, 6, 24, imageSize)); // block 12
        blocks.emplace_back(MBConvBlock(96, 136, 5, 1, 6, 24, imageSize)); // block 13
        blocks.emplace_back(MBConvBlock(136, 136, 5, 1, 6, 24, imageSize)); // block 14
        blocks.emplace_back(MBConvBlock(136, 136, 5, 1, 6, 24, imageSize)); // block 15
        blocks.emplace_back(MBConvBlock(136, 136, 5, 1, 6, 24, imageSize)); // block 16
        blocks.emplace_back(MBConvBlock(136, 136, 5, 1, 6, 24, imageSize)); // block 17

        imageSize = calculate_output_image_size(imageSize, 2);

        blocks.emplace_back(MBConvBlock(136, 232, 5, 2, 6, 24, imageSize)); // block 18
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, imageSize)); // block 19
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, imageSize)); // block 20
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, imageSize)); // block 21
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, imageSize)); // block 22
        blocks.emplace_back(MBConvBlock(232, 232, 5, 1, 6, 24, imageSize)); // block 23
        blocks.emplace_back(MBConvBlock(232, 384, 3, 1, 6, 24, imageSize)); // block 24
        blocks.emplace_back(MBConvBlock(384, 384, 3, 1, 6, 24, imageSize)); // block 25

        for (std::size_t i = 0; i < blocks.size(); ++i) {
            register_module("block_" + std::to_string(i), blocks[i]);
        }

        // headConv = Conv2dStaticSamePadding(384, 1536, 1, 1, false, imageSize);
        // register_module("headConv", headConv);

        // headBn = torch::nn::BatchNorm2d(1536);
        // register_module("headBn", headBn);

        // avgpool = torch::nn::AdaptiveAvgPool2d(torch::nn::AdaptiveAvgPool2dOptions(1));
        // register_module("avgpool", avgpool);

        // dropout = torch::nn::Dropout(torch::nn::DropoutOptions(0.3).inplace(false));
        // register_module("dropout", dropout);

        fc = torch::nn::Linear(86400, t_outputSize);
        register_module("fc", fc);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        x = stemBn(stemConv(x));

        x = swish(x);

        for (std::size_t i = 0; i < blocks.size(); ++i) {
            x = blocks[i]->forward(x);
        }

        // x = headBn->forward(headConv->forward(x));
        // x = fc->forward(dropout->forward(avgpool->forward(x)).flatten());

        x = fc->forward(x.flatten());

        return x;
    }
};

TORCH_MODULE(EfficientNetB3);

#endif