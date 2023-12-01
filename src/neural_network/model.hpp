#ifndef __MODEL_H__
#define __MODEL_H__

#include <opencv2/opencv.hpp>

#include "data.hpp"

inline static void displayConv(const torch::Tensor& t_tensor, const double& t_inRow, const std::string& t_name)
{
    auto tensor = t_tensor.detach().to(torch::kCPU).squeeze(0).to(torch::kFloat32);
    cv::Mat totalImage;
    std::vector<cv::Mat> rowImages;

    int rows = std::ceil(static_cast<double>(tensor.size(0)) / t_inRow);
    for (int i = 0; i < rows; ++i) {
        std::vector<cv::Mat> colImages;

        for (int j = 0; j < t_inRow && i * t_inRow + j < tensor.size(0); ++j) {
            auto channel = tensor[i * t_inRow + j];
            cv::Mat matrix(cv::Size(tensor.size(1), tensor.size(2)), CV_32FC1, channel.data_ptr<float>());

            cv::Mat normalized;
            matrix.convertTo(normalized, CV_8UC1, 255.0);

            cv::Mat heatmap;
            cv::applyColorMap(normalized, heatmap, cv::COLORMAP_VIRIDIS);

            cv::Mat imageWithBorder;
            cv::copyMakeBorder(heatmap, imageWithBorder, 2, 2, 2, 2, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
            colImages.push_back(imageWithBorder);
        }

        cv::Mat rowImage;
        if (!colImages.empty()) {
            cv::hconcat(colImages.data(), colImages.size(), rowImage);
            rowImages.push_back(rowImage);
        }
    }

    if (!rowImages.empty()) {
        cv::vconcat(rowImages.data(), rowImages.size(), totalImage);
    }

    // cv::imwrite("./images/" + t_name + ".png", totalImage);
    cv::resize(totalImage, totalImage, cv::Size(), 5, 5, cv::INTER_AREA);
    cv::imshow(t_name, totalImage);
    cv::waitKey(1);
}

struct ConvSelfAttentionImpl : torch::nn::Module {
    ConvSelfAttentionImpl(const int64_t& t_inChannels)
    {
        m_query = register_module("query", torch::nn::Conv2d(torch::nn::Conv2dOptions(t_inChannels, t_inChannels, 1)));
        m_key = register_module("key", torch::nn::Conv2d(torch::nn::Conv2dOptions(t_inChannels, t_inChannels, 1)));
        m_value = register_module("value", torch::nn::Conv2d(torch::nn::Conv2dOptions(t_inChannels, t_inChannels, 1)));
        m_out = register_module("out", torch::nn::Conv2d(torch::nn::Conv2dOptions(t_inChannels, t_inChannels, 1)));
    }

    torch::Tensor forward(torch::Tensor x)
    {
        auto Q = m_query->forward(x);
        auto K = m_key->forward(x);
        auto V = m_value->forward(x);

        return m_out->forward(scaledDotProductAttention(Q, K, V)) + x;
    }

private:
    torch::nn::Conv2d m_query { nullptr };
    torch::nn::Conv2d m_key { nullptr };
    torch::nn::Conv2d m_value { nullptr };
    torch::nn::Conv2d m_out { nullptr };

    torch::Tensor scaledDotProductAttention(const torch::Tensor& t_query, const torch::Tensor& t_key, const torch::Tensor& t_value)
    {
        auto scores = torch::matmul(t_query.view({ t_query.size(1), -1 }).transpose(0, 1), t_key.view({ t_query.size(1), -1 }));
        auto attentionWeights = torch::softmax(scores, -1);

        return torch::matmul(t_value.view({ t_query.size(1), -1 }), attentionWeights).view(t_query.sizes());
    }
};

TORCH_MODULE(ConvSelfAttention);

struct DQNImpl : torch::nn::Module {
    DQNImpl(int64_t t_states, int64_t t_numActions)
    {
        conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(4, 16, 3)));
        conv2 = register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(16, 64, 3)));
        conv3 = register_module("conv3", torch::nn::Conv2d(torch::nn::Conv2dOptions(64, 64, 3)));

        fc = register_module("fc", torch::nn::Sequential(torch::nn::Linear(64 * 12 * 12, 128), torch::nn::Linear(128, t_numActions)));
    }

    torch::Tensor forward(torch::Tensor t_environment, torch::Tensor t_state, bool t_showConv = false)
    {
        t_environment = torch::nn::functional::max_pool2d(torch::relu(conv1->forward(t_environment)), torch::nn::functional::MaxPool2dFuncOptions(2));

        if (t_showConv) {
            displayConv(t_environment, 4, "conv1");
        }

        t_environment = torch::nn::functional::max_pool2d(torch::relu(conv2->forward(t_environment)), torch::nn::functional::MaxPool2dFuncOptions(2));

        if (t_showConv) {
            displayConv(t_environment, 8, "conv2");
        }

        t_environment = torch::relu(conv3->forward(t_environment));

        t_environment = t_environment.flatten().unsqueeze(0);
        t_environment = t_environment.repeat({ t_state.size(0), 1 });

        return fc->forward(t_environment);
    }

    torch::nn::Conv2d conv1 { nullptr };
    torch::nn::Conv2d conv2 { nullptr };
    torch::nn::Conv2d conv3 { nullptr };

    torch::nn::Sequential fc { nullptr };
};

TORCH_MODULE(DQN);

#endif