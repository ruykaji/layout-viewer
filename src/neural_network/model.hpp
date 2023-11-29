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

    cv::imwrite("./images/" + t_name + ".png", totalImage);
}

struct SelfAttentionImpl : torch::nn::Module {

    SelfAttentionImpl(const int64_t& t_embedSize)
    {
        m_query = register_module("query", torch::nn::Linear(t_embedSize, t_embedSize));
        m_key = register_module("key", torch::nn::Linear(t_embedSize, t_embedSize));
        m_value = register_module("value", torch::nn::Linear(t_embedSize, t_embedSize));
    }

    torch::Tensor forward(torch::Tensor x)
    {
        auto Q = m_query->forward(x);
        auto K = m_key->forward(x);
        auto V = m_value->forward(x);

        return scaledDotProductAttention(Q, K, V);
    }

private:
    torch::nn::Linear m_query { nullptr };
    torch::nn::Linear m_key { nullptr };
    torch::nn::Linear m_value { nullptr };

    torch::Tensor scaledDotProductAttention(const torch::Tensor& t_query, const torch::Tensor& t_key, const torch::Tensor& t_value)
    {
        auto d_k = t_query.size(-1);
        auto scores = torch::matmul(t_query, t_key.transpose(-2, -1)) / std::sqrt(d_k);
        auto attentionWeights = torch::softmax(scores, -1);

        return torch::matmul(attentionWeights, t_value);
    }
};

TORCH_MODULE(SelfAttention);

struct DQNImpl : torch::nn::Module {
    DQNImpl(int64_t t_states, int64_t t_numActions)
    {
        conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(4, 32, 8).stride(4)));
        conv2 = register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(32, 64, 4).stride(2)));
        conv3 = register_module("conv3", torch::nn::Conv2d(torch::nn::Conv2dOptions(64, 64, 3).stride(1)));

        // fc1 = register_module("fc1", torch::nn::Sequential(torch::nn::Linear(t_states, 64)));
        // fc2 = register_module("fc2", torch::nn::Sequential(torch::nn::Linear(32 * 9 * 9, 64)));

        fcAdvantage = register_module("fcAdvantage", torch::nn::Sequential(torch::nn::Linear(64 * 7 * 7, 512), torch::nn::ReLU(torch::nn::ReLUOptions(true)), torch::nn::Linear(512, t_numActions)));
        fcValue = register_module("fcValue", torch::nn::Sequential(torch::nn::Linear(64 * 7 * 7, 512), torch::nn::ReLU(torch::nn::ReLUOptions(true)), torch::nn::Linear(512, 1)));
    }

    torch::Tensor forward(torch::Tensor t_environment, torch::Tensor t_state, bool t_showConv = false)
    {
        // t_state = fc1->forward(t_state / static_cast<double>(t_environment.size(-1)));

        t_environment = torch::relu(conv1->forward(t_environment));

        if (t_showConv) {
            displayConv(t_environment, 4, "conv1");
        }

        t_environment = torch::relu(conv2->forward(t_environment));

        if (t_showConv) {
            displayConv(t_environment, 8, "conv2");
        }

        t_environment = torch::relu(conv3->forward(t_environment));

        if (t_showConv) {
            displayConv(t_environment, 8, "conv3");
        }

        t_environment = t_environment.flatten().unsqueeze(0);
        t_environment = t_environment.repeat({ t_state.size(0), 1 });

        auto advantage = fcAdvantage->forward(t_environment);
        auto value = fcValue->forward(t_environment);

        return value + advantage - advantage.mean(1, true);
    }

    torch::nn::Conv2d conv1 { nullptr };
    torch::nn::Conv2d conv2 { nullptr };
    torch::nn::Conv2d conv3 { nullptr };

    // torch::nn::Sequential fc1 { nullptr };
    // torch::nn::Sequential fc2 { nullptr };
    torch::nn::Sequential fcAdvantage;
    torch::nn::Sequential fcValue { nullptr };
};

TORCH_MODULE(DQN);

#endif