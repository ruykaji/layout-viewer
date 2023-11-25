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

inline static void createHeatMap(const torch::Tensor& t_tensor)
{
    auto tensor = t_tensor.detach().to(torch::kCPU).squeeze().to(torch::kFloat32);

    for (std::size_t i = 0; i < tensor.size(0); ++i) {
        tensor[i] = tensor[i] * (2.0 * (i + 1));
    }

    tensor = torch::sum(tensor, 0);
    tensor = (tensor / tensor.max().item<float>());

    cv::Mat matrix(cv::Size(tensor.size(0), tensor.size(1)), CV_32FC1, tensor.data_ptr<float>());

    cv::Mat normalized;
    matrix.convertTo(normalized, CV_8UC1, 255.0);

    cv::Mat heatmap;
    cv::applyColorMap(normalized, heatmap, cv::COLORMAP_SUMMER);

    cv::imwrite("./images/heatmap.png", heatmap);
}

void Train::train(TrainTopologyDataset& t_trainDataset)
{
    std::size_t maxEpoch = 500;
    std::size_t bufferSize = 2000;
    std::size_t samples = 4;
    std::size_t batchSize = 4;

    Agent agent {};
    Environment environment {};
    torch::optim::AdamW optimizer(agent.getModelParameters(), torch::optim::AdamWOptions(1e-4));

    for (std::size_t epoch = 0; epoch < maxEpoch; ++epoch) {
        ReplayBuffer replayBuffer {};
        double epochReward {};
        double epochLoss {};

        agent.eval();

        std::size_t ki = 0;

        for (std::size_t i = 0; i < samples; ++i) {
            std::pair<at::Tensor, at::Tensor> pair = t_trainDataset.get();

            environment.set(pair.first, pair.second);

            for (std::size_t j = 0; j < bufferSize / samples; ++j) {
                if (environment.getState().size(0) <= 1) {
                    break;
                }

                torch::Tensor qValues = agent.replayAction(environment.getEnv(), environment.getState());
                ReplayBuffer::Data replay = environment.replayStep(qValues);

                replayBuffer.add(replay);

                epochReward += replay.rewards.mean().item<double>();

                displayProgressBar(++ki, bufferSize, 50, "Epoch replay-buffer");
                createHeatMap(replay.env);
            }
        }

        std::cout << '\n'
                  << std::flush;

        agent.train();

        std::size_t kj = 0;

        for (std::size_t i = 0; i < replayBuffer.size() / batchSize; ++i) {
            optimizer.zero_grad();

            torch::Tensor qTrue {};
            torch::Tensor qPred {};

            for (std::size_t j = 0; j < batchSize; ++j) {
                ReplayBuffer::Data replay = replayBuffer.get();
                std::pair<at::Tensor, at::Tensor> pair = agent.trainAction(replay);

                if (qTrue.defined()) {
                    qTrue = torch::cat({ qTrue, pair.first }, 0);
                    qPred = torch::cat({ qPred, pair.second }, 0);
                } else {
                    qTrue = pair.first;
                    qPred = pair.second;
                }

                displayProgressBar(++kj, replayBuffer.size(), 50, "Epoch training: ");
            }

            torch::Tensor loss = torch::nn::functional::huber_loss(qTrue, qPred);

            loss.backward();
            optimizer.step();

            epochLoss += loss.item<double>();

            if (kj % 100 == 0) {
                agent.updateTargetModel();
            }
        }

        std::cout << "\n\n"
                  << "Epoch - " << epoch + 1 << '\n'
                  << "MSE Loss: " << epochLoss / replayBuffer.size() << '\n'
                  << "Mean Reward: " << epochReward / replayBuffer.size() << "\n\n"
                  << std::flush;
    }
}