#ifndef __MODEL_H__
#define __MODEL_H__

#include "data.hpp"
#include "encoders/efficientnetb3.hpp"

struct QModelImpl : torch::nn::Module {
    std::vector<torch::nn::Sequential> blocks {};

    QModelImpl(const int64_t& t_inputSize, const int64_t& t_midSize, const int64_t& t_outputSize, const int64_t t_numBlocks)
    {
        blocks.emplace_back(torch::nn::Sequential(torch::nn::Linear(t_inputSize, t_midSize), torch::nn::BatchNorm1d(t_midSize), torch::nn::ReLU6(torch::nn::ReLU6Options(true))));
        register_module("blockStart", blocks.back());

        for (std::size_t i = 0; i < t_numBlocks; ++i) {
            blocks.emplace_back(torch::nn::Sequential(torch::nn::Linear(t_midSize, t_midSize), torch::nn::BatchNorm1d(t_midSize), torch::nn::ReLU6(torch::nn::ReLU6Options(true))));
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
    torch::nn::Sequential stateEncoder { nullptr };
    QModel qModel { nullptr };

    TRLMImpl(const int64_t t_states)
    {
        environmentEncoder = EfficientNetB3(1000);
        register_module("environmentEncoder", environmentEncoder);

        stateEncoder = torch::nn::Sequential(torch::nn::Linear(t_states, 256), torch::nn::BatchNorm1d(256), torch::nn::ReLU6(torch::nn::ReLU6Options(true)));
        register_module("stateEncoder", stateEncoder);

        qModel = QModel(1256, 2048, 6, 4);
        register_module("qModel", qModel);
    }

    torch::Tensor forward(torch::Tensor environment, torch::Tensor state)
    {
        state = stateEncoder->forward(state);
        environment = environmentEncoder->forward(environment).unsqueeze(0).repeat({ state.size(0), 1 });

        torch::Tensor statedEnvironment = torch::cat({ state, environment }, -1);

        return qModel->forward(statedEnvironment);
    }
};

TORCH_MODULE(TRLM);

#endif