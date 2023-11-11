#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <cstdio>
#include <defrReader.hpp>
#include <functional>
#include <lefrReader.hpp>
#include <memory>
#include <string_view>

#include "data.hpp"
#include "pdk/pdk.hpp"
#include "thread_pool.hpp"

class Encoder {
    static ThreadPool s_threadPool;

public:
    static PDK s_pdk;
    
    Encoder() = default;
    ~Encoder() = default;

    Encoder(const Encoder&) = delete;
    Encoder& operator=(const Encoder&) = delete;

    void readDef(const std::string_view& t_fileName, const std::shared_ptr<Data>& t_data);

private:
    static void addToWorkingCells(const std::shared_ptr<Rectangle>& t_target, Data* t_data);

    // Def callbacks
    // ======================================================================================

    static int defDieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData);

    static int defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData);

    static int defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData);

    static int defSpecialNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData);

    static int defPinCallback(defrCallbackType_e t_type, defiPin* t_pinProp, void* t_userData);

    static int defTrackCallback(defrCallbackType_e t_type, defiTrack* t_track, void* t_userData);
};

#endif