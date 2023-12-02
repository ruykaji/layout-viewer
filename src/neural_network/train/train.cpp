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
    std::vector<cv::Mat> layers {};

    for (std::size_t i = 0; i < t_tensor.back().size(-3); ++i) {
        auto tensor = t_tensor.back().detach().to(torch::kCPU).squeeze(0).to(torch::kFloat32)[i];
        auto tensorObst = (tensor != 1.0 / 8.0).toType(torch::kFloat);
        auto tensorRoutes = (tensor == 1.0 / 4.0).toType(torch::kFloat);
        auto tensorTargets = (tensor == 1.0 / 2.0).toType(torch::kFloat);

        for (std::size_t j = 0; j < t_tensor.size() - 1; ++j) {
            auto __tensor = t_tensor[j].detach().to(torch::kCPU).squeeze(0).to(torch::kFloat32)[i];

            tensorTargets += (__tensor == 1.0 / 2.0).toType(torch::kFloat) * static_cast<double>(t_tensor.size() - j - 1.0);
        }

        tensor = tensorObst + tensorRoutes + tensorTargets;
        tensor = tensor / tensor.max();

        cv::Mat matrix(cv::Size(tensor.size(0), tensor.size(1)), CV_32FC1, tensor.data_ptr<float>());

        cv::Mat normalized;
        matrix.convertTo(normalized, CV_8UC1, 255.0);

        cv::Mat heatmap;
        cv::applyColorMap(normalized, heatmap, cv::COLORMAP_VIRIDIS);

        cv::resize(heatmap, heatmap, cv::Size(420, 420), 0, 0, cv::INTER_AREA);

        layers.emplace_back(heatmap);
    }

    cv::Mat concatenatedImage;
    cv::hconcat(layers.data(), layers.size(), concatenatedImage);
    cv::imshow("Layers", concatenatedImage);
    cv::waitKey(1);
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

void Train::train(TrainTopologyDataset& t_trainDataset)
{
    torch::manual_seed(42);

    cv::namedWindow("Layers", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("conv1", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("conv2", cv::WINDOW_AUTOSIZE);

    std::size_t nEpisodes = 100000;
    std::size_t maxEpisodeSteps = 500;
    std::size_t batchSize = 32;
    std::size_t totalFrames = 0;

    double totalLoss {};
    double totalReward {};
    double totalWins {};

    std::vector<double> plotLoss {};
    std::vector<double> plotRewards {};
    std::vector<double> plotEpisodes {};

    Agent agent {};
    torch::optim::Adam optimizer(agent.parameters(), torch::optim::AdamOptions(0.001));
    torch::optim::StepLR lrScheduler(optimizer, 5, 0.95);

    std::size_t totalCells = 1;
    std::vector<Environment> environments {};
    std::vector<std::pair<at::Tensor, at::Tensor>> pairs {};

    for (std::size_t i = 0; i < totalCells; ++i) {
        pairs.emplace_back(t_trainDataset.get());
        environments.emplace_back(Environment());
    }

    for (std::size_t episode = 0; episode < nEpisodes; ++episode) {
        for (std::size_t e = 0; e < totalCells; ++e) {
            torch::Tensor env = torch::tensor({
                                                  { 0.125, 0.00, 0.00, 0.00, 0.00, 0.00, 0.125, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 },
                                                  { 0.125, 0.00, 0.125, 0.125, 0.125, 0.00, 0.125, 0.00, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.00 },
                                                  { 0.125, 0.00, 0.00, 0.00, 0.125, 0.00, 0.00, 0.00, 0.125, 0.00, 0.00, 0.00, 0.00, 0.00, 0.125, 0.00 },
                                                  { 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.00, 0.125, 0.125, 0.125, 0.00, 0.125, 0.00 },
                                                  { 0.125, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.125, 0.00, 0.125, 0.00, 0.00, 0.00 },
                                                  { 0.125, 0.00, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.00, 0.125, 0.125, 0.125, 0.00 },
                                                  { 0.125, 0.00, 0.00, 0.00, 0.125, 0.00, 0.125, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.125, 0.00 },
                                                  { 0.125, 0.00, 0.125, 0.00, 0.125, 0.00, 0.125, 0.00, 0.125, 0.125, 0.125, 0.125, 0.125, 0.00, 0.125, 0.00 },
                                                  { 0.125, 0.00, 0.125, 0.00, 0.125, 0.00, 0.125, 0.00, 0.00, 0.00, 0.125, 0.00, 0.00, 0.00, 0.125, 0.00 },
                                                  { 0.125, 0.125, 0.125, 0.00, 0.125, 0.00, 0.125, 0.125, 0.125, 0.00, 0.125, 0.00, 0.125, 0.125, 0.125, 0.00 },
                                                  { 0.125, 0.00, 0.00, 0.00, 0.125, 0.00, 0.00, 0.00, 0.00, 0.00, 0.125, 0.00, 0.00, 0.00, 0.00, 0.00 },
                                                  { 0.125, 0.00, 0.125, 0.00, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.00, 0.125, 0.125, 0.125, 0.125 },
                                                  { 0.125, 0.00, 0.125, 0.00, 0.125, 0.00, 0.00, 0.00, 0.00, 0.00, 0.125, 0.00, 0.00, 0.00, 0.00, 0.00 },
                                                  { 0.125, 0.00, 0.125, 0.125, 0.125, 0.00, 0.125, 0.125, 0.125, 0.00, 0.125, 0.125, 0.125, 0.125, 0.125, 0.00 },
                                                  { 0.125, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.125, 0.00, 0.00, 0.00, 0.125, 0.00, 0.00, 0.00 },
                                                  { 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125 },
                                              })
                                    .unsqueeze(0)
                                    .unsqueeze(0);

            torch::Tensor state = torch::tensor({ 0.0, 0.0, 0.0, 12.0, 9.0, 0.0 }).unsqueeze(0);

            environments[e].set(env, state);

            std::vector<Frame> frames {};

            for (std::size_t i = 0; i < maxEpisodeSteps; ++i) {
                ++totalFrames;

                auto [actionLogits, values] = agent.action(environments[e].getEnv(), environments[e].getState());
                auto [frame, done, win] = environments[e].step(actionLogits);

                frame.values = values;
                frames.emplace_back(frame);

                totalReward += frame.rewards.mean().item<double>();

                createHeatMap(environments[e].getEnv());

                if (win) {
                    ++totalWins;
                }

                if (done) {
                    break;
                }

                displayProgressBar(i, maxEpisodeSteps, 50, "Experience steps: ");
            }

            optimizer.zero_grad();

            auto loss = agent.learn(frames);

            loss.backward();
            optimizer.step();

            totalLoss += loss.item<double>();

            plotLoss.emplace_back(totalLoss);
            plotRewards.emplace_back(totalReward);
            plotEpisodes.emplace_back(plotEpisodes.size());

            plot(plotLoss, plotEpisodes, "Loss");
            plot(plotRewards, plotEpisodes, "Reward");

            totalLoss = 0.0;
            totalReward = 0.0;

            std::cout << "\n\n"
                      << "Episode - " << episode + 1 << "/" << nEpisodes << '\n'
                      << "Huber Loss: " << plotLoss.back() << '\n'
                      << "Mean Reward: " << plotRewards.back() << '\n'
                      << "Total wins: " << totalWins << '\n'
                      << "Total frames: " << totalFrames << '\n'
                      << std::flush;
        }

        lrScheduler.step();
    }
}