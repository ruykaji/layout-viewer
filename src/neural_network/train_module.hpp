#ifndef __TRAIN_MODULE_H__
#define __TRAIN_MODULE_H__

#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "dataset.hpp"
#include "model.hpp"

inline static void showProgressBar(int progress, int total)
{
    const int bar_width = 70; // Width of the progress bar in characters

    float progress_percent = (float)progress / total;
    int pos = (int)(bar_width * progress_percent);

    std::cout << "[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << int(progress_percent * 100.0) << " %\r";
    std::cout.flush();
}

inline static void clearDirectory(const std::string& path)
{
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (std::filesystem::is_directory(entry)) {
                clearDirectory(entry.path().string());
            } else {
                std::filesystem::remove(entry);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error while clearing directory: " << e.what() << std::endl;
    }
}

class TrainModule {
    UNet m_model;
    torch::Device m_device;

public:
    explicit TrainModule(int8_t t_device = 0)
        : m_device(torch::Device(t_device ? torch::kCPU : torch::kCUDA)) {};

    torch::Tensor dice_loss(const torch::Tensor& input, const torch::Tensor& target)
    {
        auto input_flat = input.view(-1);
        auto target_flat = target.view(-1);

        auto intersection = (input_flat * target_flat).sum();
        auto dice_score = (2.0 * intersection + 1.0) / (input_flat.sum() + target_flat.sum() + 1.0);
        auto dice_loss = 1.0 - dice_score;

        return dice_loss;
    }

    void train(TopologyDataset& t_trainDataset, TopologyDataset& t_validDataset, std::size_t t_epochs)
    {
        clearDirectory("./cells");

        torch::optim::AdamW optimizer(m_model->parameters(), torch::optim::AdamWOptions(1e-3).weight_decay(0.1));

        m_model->to(m_device);

        for (size_t epoch = 0; epoch != t_epochs; ++epoch) {
            m_model->train();

            double trainLoss = 0.0;
            double totalCells = 0.0;

            for (std::size_t i = 0; i < t_trainDataset.size(); ++i) {
                auto cells = t_trainDataset.get(i)->cells;

                std::size_t numY = cells.size();
                std::size_t numX = cells[0].size();
                int32_t startX = 0;
                int32_t startY = 0;
                int32_t progress = 0;

                std::cout << "\nTopology: " << i + 1 << " Num cells: " << numX << " " << numY << std::endl;

                while (startY != numY) {
                    int32_t x = startX;
                    int32_t y = startY;

                    while (x >= 0 && y < numY) {
                        optimizer.zero_grad();

                        torch::Tensor data = cells[y][x]->source.to(m_device);
                        torch::Tensor nets = cells[y][x]->cellInformation.to(m_device);

                        torch::Tensor output = m_model->forward(data, nets);
                        torch::Tensor target = cells[y][x]->target.to(m_device);

                        torch::Tensor loss = dice_loss(torch::sigmoid(output), target) + torch::nn::functional::cross_entropy(output, target);

                        loss.backward();
                        optimizer.step();

                        trainLoss += loss.item<double>();

                        if((i + 1) % 10 == 0){
                            auto img1 = tensorToImage(data.squeeze(0));
                            auto img2 = tensorToImage(target.squeeze(0));
                            auto img3 = tensorToImage(torch::sigmoid(output).squeeze(0));

                            cv::Mat stackedImage;

                            cv::hconcat(img1, img2, stackedImage);
                            cv::hconcat(stackedImage, img3, stackedImage);

                            std::stringstream ss;

                            ss << "./cells/cell_" << (x + 1) << "_" << (y + 1) << ".png";

                            cv::imwrite(ss.str(), stackedImage);
                        }

                        ++y;
                        --x;
                        ++progress;
                    }

                    if (startX < numX - 1) {
                        ++startX;
                    } else {
                        ++startY;
                    }

                    showProgressBar(progress, numY * numX);
                }

                totalCells += progress;
            }

            std::cout << "\nEpoch: " << epoch
                      << " | Training Loss: " << trainLoss / totalCells << std::endl;
        }
    }

    void valid(TopologyDataset& t_validDataset, std::size_t t_epochs)
    {
        for (size_t epoch = 0; epoch != t_epochs; ++epoch) {
            m_model->eval();

            double validLoss = 0.0;
            double totalCells = 0.0;

            for (std::size_t i = 0; i < t_validDataset.size(); ++i) {
                auto cells = t_validDataset.get(i)->cells;

                std::size_t numY = cells.size();
                std::size_t numX = cells[0].size();
                int32_t startX = 0;
                int32_t startY = 0;
                int32_t progress = 0;

                std::cout << "\nTopology: " << i + 1 << " Num cells: " << numX << " " << numY << std::endl;

                while (startY != numY) {
                    int32_t x = startX;
                    int32_t y = startY;

                    while (x >= 0 && y < numY) {
                        torch::Tensor data = cells[y][x]->source.to(m_device).requires_grad_(true);
                        torch::Tensor nets = cells[y][x]->cellInformation.to(m_device).requires_grad_(true);

                        torch::Tensor output = torch::sigmoid(m_model->forward(data, nets));
                        torch::Tensor target = cells[y][x]->target.to(m_device);

                        ++y;
                        --x;
                        ++progress;
                    }

                    if (startX < numX - 1) {
                        ++startX;
                    } else {
                        ++startY;
                    }

                    showProgressBar(progress, numY * numX);
                }

                totalCells += progress;
            }

            std::cout << "\nEpoch: " << epoch
                      << " | Valid Loss: " << validLoss / totalCells << std::endl;
        }
    }

    cv::Mat tensorToImage(const torch::Tensor& t_tensor)
    {
        auto tensor = t_tensor.detach().to(torch::kCPU);
        tensor = (torch::sum(tensor, 0) > 0).toType(torch::kFloat32);

        cv::Mat image(tensor.size(0), tensor.size(1), CV_32FC1, tensor.data_ptr<float>());
        cv::normalize(image, image, 0, 255, cv::NORM_MINMAX);

        image.convertTo(image, CV_8U);

        return image;
    }
};

#endif