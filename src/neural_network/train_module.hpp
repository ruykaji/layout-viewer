#ifndef __TRAIN_MODULE_H__
#define __TRAIN_MODULE_H__

#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "dataset.hpp"
#include "model.hpp"

inline static void showProgressBar(int progress, int total, double epochLoss)
{
    const int bar_width = 100; // Width of the progress bar in characters

    float progress_percent = (float)progress / total;
    int pos = (int)(bar_width * progress_percent);

    std::cout << progress + 1 << "/" << total + 1 << " [";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] "
              << " Epoch loss: " << epochLoss << "\r";
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

class DiceBCELossImpl : public torch::nn::Module {
public:
    DiceBCELossImpl() { }

    torch::Tensor forward(torch::Tensor inputs, torch::Tensor targets, float smooth = 1.0)
    {
        inputs = torch::sigmoid(inputs);

        inputs = inputs.view({ -1 });
        targets = targets.view({ -1 });

        torch::Tensor intersection = (inputs * targets).sum();
        torch::Tensor dice_loss = 1.0 - (2.0 * intersection + smooth) / (inputs.sum() + targets.sum() + smooth);
        torch::Tensor BCE = torch::nn::functional::binary_cross_entropy(inputs, targets, torch::nn::functional::BinaryCrossEntropyFuncOptions().reduction(torch::kMean));
        torch::Tensor Dice_BCE = BCE + dice_loss;

        return Dice_BCE;
    }
};

TORCH_MODULE(DiceBCELoss);

class TrainModule {
    DiceBCELoss m_DiceBCEloss;
    UNet3D m_model;
    torch::Device m_device;

public:
    explicit TrainModule(int8_t t_device = 0)
        : m_device(torch::Device(t_device ? torch::kCPU : torch::kCUDA)) {};

    void train(TopologyDataset& t_trainDataset, std::size_t t_epochs)
    {
        clearDirectory("./cells");

        torch::optim::AdamW optimizer(m_model->parameters(), torch::optim::AdamWOptions(1e-3).weight_decay(0.1));
        torch::optim::StepLR lr_scheduer(optimizer, 2, 0.95);

        m_model->to(m_device);
        m_model->train();

        for (size_t epoch = 0; epoch != t_epochs; ++epoch) {

            double trainLoss = 0.0;
            double progress = 0.0;

            for (int32_t i = 0; i < t_trainDataset.size(); ++i) {
                optimizer.zero_grad();

                std::pair<at::Tensor, at::Tensor> pair = t_trainDataset.get(i);

                torch::Tensor data = pair.first.to(m_device);
                torch::Tensor target = pair.second.to(m_device);
                torch::Tensor output = m_model->forward(data);

                torch::Tensor loss = m_DiceBCEloss->forward(output, target);

                loss.backward();

                optimizer.step();

                trainLoss += loss.item<double>();

                if (i % 25 == 0) {
                    for (std::size_t i = 0; i < target.size(2); ++i) {
                        auto img1 = tensorToImage(target[0][0][i].cpu());
                        auto img2 = tensorToImage((torch::sigmoid(output[0][0][i]) > 0.95).detach().cpu().toType(torch::kFloat32));

                        cv::Mat stackedImage;

                        cv::hconcat(img1, img2, stackedImage);
                        cv::imwrite("./cells/cell_" + std::to_string(i) + ".png", stackedImage);
                    }
                }

                showProgressBar(++progress, t_trainDataset.size(), (trainLoss / (i + 1)));
            }

            lr_scheduer.step();

            std::cout << "\nEpoch: " << epoch
                      << " | Training Loss: " << trainLoss / t_trainDataset.size()
                      << '\n'
                      << std::endl;
        }
    }

    // void valid(TopologyDataset& t_validDataset, std::size_t t_epochs)
    // {
    //     for (size_t epoch = 0; epoch != t_epochs; ++epoch) {
    //         m_model->eval();

    //         double validLoss = 0.0;
    //         double totalCells = 0.0;

    //         for (std::size_t i = 0; i < t_validDataset.size(); ++i) {
    //             auto cells = t_validDataset.get(i)->cells;

    //             std::size_t numY = cells.size();
    //             std::size_t numX = cells[0].size();
    //             int32_t startX = 0;
    //             int32_t startY = 0;
    //             int32_t progress = 0;

    //             std::cout << "\nTopology: " << i + 1 << " Num cells: " << numX << " " << numY << std::endl;

    //             while (startY != numY) {
    //                 int32_t x = startX;
    //                 int32_t y = startY;

    //                 while (x >= 0 && y < numY) {
    //                     torch::Tensor data = cells[y][x]->source.to(m_device).requires_grad_(true);
    //                     torch::Tensor nets = cells[y][x]->cellInformation.to(m_device).requires_grad_(true);

    //                     torch::Tensor output = torch::sigmoid(m_model->forward(data, nets));
    //                     torch::Tensor target = cells[y][x]->target.to(m_device);

    //                     ++y;
    //                     --x;
    //                     ++progress;
    //                 }

    //                 if (startX < numX - 1) {
    //                     ++startX;
    //                 } else {
    //                     ++startY;
    //                 }

    //                 showProgressBar(progress, numY * numX);
    //             }

    //             totalCells += progress;
    //         }

    //         std::cout << "\nEpoch: " << epoch
    //                   << " | Valid Loss: " << validLoss / totalCells << std::endl;
    //     }
    // }

    cv::Mat tensorToImage(const torch::Tensor& t_tensor)
    {
        cv::Mat image(t_tensor.size(0), t_tensor.size(1), CV_32FC1, t_tensor.data_ptr<float>());
        cv::normalize(image, image, 0, 255, cv::NORM_MINMAX);

        image.convertTo(image, CV_8U);

        return image;
    }
};

#endif