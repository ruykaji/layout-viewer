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

inline static void createHeatMap(const std::array<torch::Tensor, 3>& t_tensor)
{
    for (std::size_t i = 0; i < t_tensor.back().size(-3); ++i) {
        auto tensor1 = t_tensor[0].detach().to(torch::kCPU).squeeze(0).to(torch::kFloat32)[i];
        auto tensor2 = t_tensor[1].detach().to(torch::kCPU).squeeze(0).to(torch::kFloat32)[i];
        auto tensor3 = t_tensor[2].detach().to(torch::kCPU).squeeze(0).to(torch::kFloat32)[i];

        auto tensorObst = torch::logical_or(torch::logical_or(tensor1 != 0.2, tensor2 != 0.2), tensor3 != 0.2).toType(torch::kFloat);
        auto tensorTracks = (tensor1 == 0.35).toType(torch::kFloat) + (tensor2 == 0.35).toType(torch::kFloat) + (tensor3 == 0.35).toType(torch::kFloat);
        auto tensorTargets = (tensor1 == 1.0).toType(torch::kFloat) + (tensor2 == 1.0).toType(torch::kFloat) + (tensor3 == 1.0).toType(torch::kFloat);

        auto tensor = tensorObst + tensorTracks + tensorTargets;
        tensor = tensor / tensor.max();

        cv::Mat matrix(cv::Size(tensor.size(0), tensor.size(1)), CV_32FC1, tensor.data_ptr<float>());

        cv::Mat normalized;
        matrix.convertTo(normalized, CV_8UC1, 255.0);

        cv::Mat heatmap;
        cv::applyColorMap(normalized, heatmap, cv::COLORMAP_JET);

        cv::imwrite("./images/layer_" + std::to_string(i + 1) + ".png", heatmap);
    }
}

void Train::train(TrainTopologyDataset& t_trainDataset)
{
    std::size_t nEpisodes = 1000;
    std::size_t nSteps = 100;
    std::size_t batchSize = 32;
    std::size_t updateSteps = 3000;
    std::size_t updateCounter = 0;

    std::size_t stepsToPrintInfo = 0;
    double totalLoss {};
    double totalReward {};

    Agent agent {};
    Environment environment {};
    ReplayBuffer replayBuffer(100000);
    torch::optim::AdamW optimizer(agent.getModelParameters(), torch::optim::AdamWOptions(1e-3));

    agent.train();

    for (std::size_t epoch = 0; epoch < t_trainDataset.size(); ++epoch) {
        // std::pair<at::Tensor, at::Tensor> pair = t_trainDataset.get();

        torch::Tensor source = torch::tensor({
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                                 { 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2, 0.00, 0.2 },
                                                 { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 },
                                             })
                                   .unsqueeze(0)
                                   .unsqueeze(0);

        torch::Tensor connections = torch::tensor({ 2.0, 2.0, 12.0, 20.0 }).unsqueeze(0);

        std::pair<at::Tensor, at::Tensor> pair = std::pair(source, connections);

        for (std::size_t episode = 0; episode < nEpisodes; ++episode) {
            environment.set(pair.first, pair.second);

            for (std::size_t step = 0; step < nSteps; ++step) {
                torch::Tensor qValues = agent.replayAction(environment.getEnv(), environment.getState());
                std::pair<ReplayBuffer::Data, bool> replay = environment.replayStep(qValues);

                replayBuffer.add(replay.first);

                if (step % 1 == 0) {
                    createHeatMap(replay.first.nextEnv);
                }

                if (replayBuffer.size() >= batchSize) {
                    torch::Tensor loss {};

                    for (std::size_t j = 0; j < batchSize; ++j) {
                        ReplayBuffer::Data replay = replayBuffer.get();
                        std::pair<at::Tensor, at::Tensor> pair = agent.trainAction(replay);

                        if (loss.defined()) {
                            loss += torch::nn::functional::huber_loss(pair.first, pair.second);
                        } else {
                            loss = torch::nn::functional::huber_loss(pair.first, pair.second);
                        }
                    }

                    optimizer.zero_grad();
                    loss.backward();
                    optimizer.step();

                    totalLoss += loss.item<double>() / batchSize;
                    totalReward += replay.first.rewards.mean().item<double>();

                    ++updateCounter;
                    ++stepsToPrintInfo;

                    displayProgressBar(stepsToPrintInfo, 200, 50, "Steps: ");

                    if (stepsToPrintInfo == 200) {
                        std::cout << "\n\n"
                                  << "Epoch - " << epoch + 1 << "/" << t_trainDataset.size() << '\n'
                                  << "Episode - " << episode + 1 << "/" << nEpisodes << '\n'
                                  << "Huber Loss: " << totalLoss / stepsToPrintInfo << '\n'
                                  << "Mean Reward: " << totalReward / stepsToPrintInfo << '\n'
                                  << "Exp to Expl: " << environment.getExpToExpl() << "%\n\n"
                                  << std::flush;

                        totalLoss = 0.0;
                        totalReward = 0.0;
                        stepsToPrintInfo = 0;
                    }
                }

                agent.softUpdateTargetModel(0.05);

                if (replay.second) {
                    break;
                }
            }
        }
    }
}