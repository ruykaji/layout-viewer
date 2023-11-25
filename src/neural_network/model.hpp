#ifndef __MODEL_H__
#define __MODEL_H__

#include "data.hpp"
#include "encoders/efficientnetb3.hpp"
#include "encoders/resnet50.hpp"

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
            if (i % 2 == 0) {
                blocks.emplace_back(torch::nn::Sequential(torch::nn::Linear(t_midSize, t_midSize), torch::nn::Dropout(torch::nn::DropoutOptions(0.2).inplace(false)), torch::nn::ReLU(torch::nn::ReLUOptions(true))));
            } else {
                blocks.emplace_back(torch::nn::Sequential(torch::nn::Linear(t_midSize, t_midSize), torch::nn::ReLU(torch::nn::ReLUOptions(true))));
            }

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

struct TRLMImpl : torch::nn::Module {
    EfficientNetB3 environmentEncoder { nullptr };
    // ResNet50 environmentEncoder { nullptr };
    torch::nn::Sequential stateEncoder { nullptr };
    SelfAttention attention { nullptr };
    QModel qModel { nullptr };

    TRLMImpl(const int64_t t_states)
    {
        // environmentEncoder = ResNet50(256);
        environmentEncoder = EfficientNetB3(1024);
        register_module("environmentEncoder", environmentEncoder);

        stateEncoder = torch::nn::Sequential(torch::nn::Linear(t_states, 256));
        register_module("stateEncoder", stateEncoder);

        attention = SelfAttention(1280);
        register_module("attention", attention);

        qModel = QModel(1280, 1280, 6, 4);
        register_module("qModel", qModel);
    }

    torch::Tensor forward(torch::Tensor environment, torch::Tensor state)
    {
        state = stateEncoder->forward(state);
        environment = environmentEncoder->forward(environment).unsqueeze(0).repeat({ state.size(0), 1 });

        torch::Tensor statedEnvironment = torch::cat({ state, environment }, -1);

        return qModel->forward(attention->forward(statedEnvironment));
    }
};

TORCH_MODULE(TRLM);

#endif