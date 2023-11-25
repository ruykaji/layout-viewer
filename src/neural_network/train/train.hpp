#ifndef __TRAIN_H__
#define __TRAIN_H__

#include "neural_network/dataset.hpp"

class Train {
public:
    Train() = default;
    ~Train() = default;

    void train(TrainTopologyDataset& t_trainDataset);
};

#endif