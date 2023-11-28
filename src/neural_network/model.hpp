#ifndef __MODEL_H__
#define __MODEL_H__

#include <opencv2/opencv.hpp>

#include "data.hpp"
#include "encoders/efficientnetb3.hpp"
#include "encoders/resnet50.hpp"

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
            cv::normalize(matrix, normalized, 0, 255, cv::NORM_MINMAX, CV_8UC1);

            cv::Mat imageWithBorder;
            cv::copyMakeBorder(normalized, imageWithBorder, 2, 2, 2, 2, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
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

struct QModelImpl : torch::nn::Module {
    std::vector<torch::nn::Sequential> blocks {};

    QModelImpl(const int64_t& t_inputSize, const int64_t& t_midSize, const int64_t& t_outputSize, const int64_t t_numBlocks)
    {
        blocks.emplace_back(torch::nn::Sequential(torch::nn::Linear(t_inputSize, t_midSize), torch::nn::ReLU(torch::nn::ReLUOptions(true))));
        register_module("blockStart", blocks.back());

        for (std::size_t i = 0; i < t_numBlocks; ++i) {
            blocks.emplace_back(torch::nn::Sequential(torch::nn::Linear(t_midSize, t_midSize), torch::nn::ReLU(torch::nn::ReLUOptions(true))));
            register_module("block" + std::to_string(i), blocks.back());
        }

        blocks.emplace_back(torch::nn::Sequential(torch::nn::Linear(t_midSize, t_outputSize)));
        register_module("blockEnd", blocks.back());
    };

    torch::Tensor forward(torch::Tensor x)
    {
        for (auto& block : blocks) {
            x = block->forward(x);
        }

        return x;
    }
};

TORCH_MODULE(QModel);

struct DQNImpl : torch::nn::Module {
    DQNImpl(int64_t t_states, int64_t t_numActions)
    {
        conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(3, 16, 3).stride(1)));
        conv2 = register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(16, 32, 3).stride(1)));

        fc1 = register_module("fc1", torch::nn::Linear(25088, 256));
        fc2 = register_module("fc2", torch::nn::Linear(t_states, 64));
        fc3 = register_module("fc3", torch::nn::Linear(320, t_numActions));
    }

    torch::Tensor forward(torch::Tensor t_environment, torch::Tensor t_state, bool t_showConv = false)
    {
        t_environment = torch::relu(conv1->forward(t_environment));

        if (t_showConv) {
            displayConv(t_environment, 4, "conv1");
        }

        t_environment = torch::relu(conv2->forward(t_environment));

        if (t_showConv) {
            displayConv(t_environment, 8, "conv2");
        }

        t_environment = t_environment.flatten();
        t_environment = torch::relu(fc1->forward(t_environment)).unsqueeze(0).repeat({ t_state.size(0), 1 });

        t_state /= static_cast<double>(t_environment.size(-1));
        t_state = fc2->forward(t_state);

        return fc3->forward(torch::cat({ t_state, t_environment }, -1));
    }

    torch::nn::Conv2d conv1 { nullptr }, conv2 { nullptr };
    torch::nn::Linear fc1 { nullptr }, fc2 { nullptr }, fc3 { nullptr };
};

TORCH_MODULE(DQN);

struct TRLMImpl : torch::nn::Module {
    // EfficientNetB3 environmentEncoder { nullptr };
    ResNet50 environmentEncoder { nullptr };
    torch::nn::Sequential stateEncoder { nullptr };
    SelfAttention attention { nullptr };
    QModel qModelAdvantage { nullptr };
    QModel qModelValue { nullptr };

    TRLMImpl(const int64_t t_states)
    {
        environmentEncoder = ResNet50(256);
        // environmentEncoder = EfficientNetB3(256);
        register_module("environmentEncoder", environmentEncoder);

        stateEncoder = torch::nn::Sequential(torch::nn::Linear(t_states, 64));
        register_module("stateEncoder", stateEncoder);

        attention = SelfAttention(2048 + 64);
        register_module("attention", attention);

        qModelValue = QModel(2048 + 64, 256, 1, 0);
        register_module("qModelValue", qModelValue);

        qModelAdvantage = QModel(2048 + 64, 256, 6, 0);
        register_module("qModelAdvantage", qModelAdvantage);
    }

    torch::Tensor forward(torch::Tensor environment, torch::Tensor state)
    {
        state /= static_cast<double>(environment.size(-1));
        environment /= environment.max();

        state = stateEncoder->forward(state);
        environment = environmentEncoder->forward(environment).unsqueeze(0).repeat({ state.size(0), 1 });

        torch::Tensor statedEnvironment = torch::cat({ state, environment }, -1);

        statedEnvironment = attention->forward(statedEnvironment);

        auto advantage = qModelAdvantage->forward(statedEnvironment);
        auto value = qModelValue->forward(statedEnvironment);

        return value + advantage - advantage.mean(1, true);
    }
};

TORCH_MODULE(TRLM);

#endif