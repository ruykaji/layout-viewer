#include <matplotlibcpp.h>
#include <opencv2/opencv.hpp>

#include "agent.hpp"
#include "evironment.hpp"
#include "train.hpp"

inline static void displayProgressBar(const int32_t& t_current, const int32_t& t_total, const int32_t& t_barWidth, const std::string& t_title)
{
    double progress = static_cast<double>(t_current) / t_total;
    int32_t pos = t_barWidth * progress;

    std::cout << t_title << " [";

    for (int32_t i = 0; i < t_barWidth; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }

    std::cout << "] " << t_current << "/" << t_total << " \r";
    std::cout.flush();
}

inline static void createHeatMap(const std::vector<torch::Tensor>& t_tensor)
{
    for (std::size_t i = 0; i < t_tensor.back().size(-3); ++i) {
        auto tensor = t_tensor.back().detach().to(torch::kCPU).squeeze(0).to(torch::kFloat32)[i];
        auto tensorObst = (tensor != 0.5).toType(torch::kFloat);
        auto tensorTracks = (tensor == 0.25).toType(torch::kFloat);
        auto tensorTargets = (tensor == 1.0).toType(torch::kFloat);

        for (std::size_t j = 0; j < t_tensor.size() - 1; ++j) {
            auto __tensor = t_tensor[j].detach().to(torch::kCPU).squeeze(0).to(torch::kFloat32)[i];

            tensorTargets += (__tensor == 1.0).toType(torch::kFloat) * static_cast<double>(t_tensor.size() - j - 1.0);
        }

        tensor = tensorObst + tensorTracks + tensorTargets;
        tensor = tensor / tensor.max();

        cv::Mat matrix(cv::Size(tensor.size(0), tensor.size(1)), CV_32FC1, tensor.data_ptr<float>());

        cv::Mat normalized;
        matrix.convertTo(normalized, CV_8UC1, 255.0);

        cv::Mat heatmap;
        cv::applyColorMap(normalized, heatmap, cv::COLORMAP_VIRIDIS);

        cv::imwrite("./images/layer_" + std::to_string(i + 1) + ".png", heatmap);
    }
}

inline static void plot(const std::vector<double>& t_y, const std::vector<double>& t_x, const std::string& t_name)
{
    matplotlibcpp::plot(t_x, t_y);
    matplotlibcpp::xlabel("Steps");
    matplotlibcpp::ylabel(t_name);
    matplotlibcpp::title(t_name);
    matplotlibcpp::grid(true);
    matplotlibcpp::save("./images/loss_" + t_name + ".png");
    matplotlibcpp::close();
};

inline static void clip_grad_norm(std::vector<torch::Tensor> parameters, double max_norm)
{
    double total_norm = 0;

    for (const auto& param : parameters) {
        if (param.grad().defined()) {
            total_norm += param.grad().data().norm(2).item<double>();
        }
    }

    total_norm = std::sqrt(total_norm);

    if (total_norm > max_norm) {
        double clip_coef = max_norm / (total_norm + 1e-6);

        for (auto& param : parameters) {
            if (param.grad().defined()) {
                param.grad().data().mul_(clip_coef);
            }
        }
    }
}

void Train::train(TrainTopologyDataset& t_trainDataset)
{
    std::size_t nEpisodes = 1000;
    std::size_t maxEpisodeSteps = 50000;
    std::size_t batchSize = 32;
    std::size_t replayBufferSize = 20000;
    std::size_t stepsToUpdateTargetModel = 10000;
    std::size_t updateTargetModelStep = 0;

    double totalLoss {};
    double totalReward {};

    std::vector<double> plotLoss {};
    std::vector<double> plotRewards {};
    std::vector<double> plotEpisodes {};

    Agent agent {};
    Environment environment {};
    ReplayBuffer replayBuffer(replayBufferSize);
    torch::optim::Adam optimizer(agent.getModelParameters(), torch::optim::AdamOptions(0.00025));

    for (std::size_t epoch = 0; epoch < t_trainDataset.size(); ++epoch) {
        std::pair<at::Tensor, at::Tensor> pair = t_trainDataset.get();

        for (std::size_t episode = 0; episode < nEpisodes; ++episode) {
            environment.set(pair.first, pair.second);

            for (std::size_t i = 0; i < maxEpisodeSteps; ++i) {
                ++updateTargetModelStep;

                torch::Tensor qValues = agent.replayAction(environment.getEnv(), environment.getState());
                std::pair<ReplayBuffer::Data, bool> replay = environment.replayStep(qValues);

                replayBuffer.add(replay.first);

                createHeatMap(replay.first.nextEnv);

                if (replayBuffer.size() % 4 == 0 && replayBuffer.size() >= batchSize) {
                    std::vector<at::Tensor> parameters = agent.getModelParameters();
                    torch::Tensor loss {};

                    optimizer.zero_grad();

                    for (std::size_t j = 0; j < batchSize; ++j) {
                        ReplayBuffer::Data replay = replayBuffer.get();
                        std::pair<at::Tensor, at::Tensor> pair = agent.trainAction(replay);

                        if (loss.defined()) {
                            loss += torch::nn::functional::huber_loss(pair.first, pair.second);
                        } else {
                            loss = torch::nn::functional::huber_loss(pair.first, pair.second);
                        }
                    }

                    loss.backward();
                    clip_grad_norm(parameters, 1.0);
                    optimizer.step();

                    totalLoss += loss.item<double>() / batchSize;
                    totalReward += replay.first.rewards.mean().item<double>();

                    displayProgressBar(updateTargetModelStep, stepsToUpdateTargetModel, 50, "Experience steps: ");

                    if (stepsToUpdateTargetModel == updateTargetModelStep) {
                        agent.updateTargetModel();

                        plotLoss.emplace_back(totalLoss / stepsToUpdateTargetModel);
                        plotRewards.emplace_back(totalReward / stepsToUpdateTargetModel);
                        plotEpisodes.emplace_back(plotEpisodes.size());

                        plot(plotLoss, plotEpisodes, "Loss");
                        plot(plotRewards, plotEpisodes, "Reward");

                        totalLoss = 0.0;
                        totalReward = 0.0;
                        updateTargetModelStep = 0;

                        std::cout << "\n\n"
                                  << "Epoch - " << epoch + 1 << "/" << t_trainDataset.size() << '\n'
                                  << "Episode - " << episode + 1 << "/" << nEpisodes << '\n'
                                  << "Huber Loss: " << plotLoss.back() << '\n'
                                  << "Mean Reward: " << plotRewards.back() << '\n'
                                  << "Exp to Expl: " << environment.getExpToExpl() << "%\n\n"
                                  << std::flush;
                    }
                }

                if (replay.second) {
                    break;
                }
            }
        }
    }
}