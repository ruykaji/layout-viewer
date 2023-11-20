#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <cstdio>
#include <defrReader.hpp>
#include <functional>
#include <lefrReader.hpp>
#include <memory>
#include <string_view>

#include "config.hpp"
#include "data.hpp"
#include "pdk/pdk.hpp"
#include "thread_pool.hpp"

class Encoder {
    static ThreadPool s_threadPool;

    struct Container {
        std::shared_ptr<Config> config {};
        std::shared_ptr<PDK> pdk {};
        std::shared_ptr<Data> data {};
    };

public:
    Encoder() = default;
    ~Encoder() = default;

    Encoder(const Encoder&) = delete;
    Encoder& operator=(const Encoder&) = delete;

    void readDef(const std::string_view& t_fileName, const std::shared_ptr<Data>& t_data, const std::shared_ptr<PDK>& t_pdk, const std::shared_ptr<Config>& t_config);

private:
    static void addGeometryToWorkingCells(const std::shared_ptr<Rectangle>& t_target, Container* t_container);

    static void addPinToWorkingCells(const std::vector<std::shared_ptr<Rectangle>>& t_target, const std::string& t_pinName, Container* t_container);

    // Def callbacks
    // ======================================================================================

    static int defDesignCallback(defrCallbackType_e t_type, const char* t_design, void* t_userData);

    static int defDieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData);

    static int defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData);

    static int defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData);

    static int defSpecialNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData);

    static int defPinCallback(defrCallbackType_e t_type, defiPin* t_pinProp, void* t_userData);

    static int defTrackCallback(defrCallbackType_e t_type, defiTrack* t_track, void* t_userData);
};

#endif