#ifndef __TORCH_INCLUDE_H__
#define __TORCH_INCLUDE_H__

#ifdef QT_VERSION
    #undef slots
    #include <torch/torch.h>
    #include <torch/script.h>
    #define slots Q_SLOTS
#else
    #include <torch/torch.h>
    #include <torch/script.h>
#endif

#endif