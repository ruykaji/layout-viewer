#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <cstdio>
#include <defrReader.hpp>
#include <functional>
#include <lefrReader.hpp>
#include <memory>
#include <string_view>

#include "def.hpp"
#include "macro.hpp"

class DEFEncoder {
    std::shared_ptr<Def> m_def {};

public:
    explicit DEFEncoder() = default;
    ~DEFEncoder() = default;

    COPY_CONSTRUCTOR_REMOVE(DEFEncoder);
    ASSIGN_OPERATOR_REMOVE(DEFEncoder);

    std::shared_ptr<Def> read(const std::string_view t_fileName);

private:
    static Geometry::ML convertNameToML(const char* t_name);

    static std::string findLef(const std::string& t_folder, const std::string& t_fileName);

    static void setGeomOrientation(const int8_t t_orientation, int32_t& t_x, int32_t& t_y);

    static int lefPinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData);

    static int defBlockageCallback(defrCallbackType_e t_type, defiBlockage* t_blockage, void* t_userData);

    static int defDieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData);

    static int defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData);

    static int defComponentMaskShiftLayerCallback(defrCallbackType_e t_type, defiComponentMaskShiftLayer* t_shiftLayers, void* t_userData);

    static int defDoubleCallback(defrCallbackType_e t_type, double* t_number, void* t_userData);

    static int defFillCallback(defrCallbackType_e t_type, defiFill* t_shiftLayers, void* t_userData);

    static int defGcellGridCallback(defrCallbackType_e t_type, defiGcellGrid* t_grid, void* t_userData);

    static int defGroupCallback(defrCallbackType_e t_type, defiGroup* t_shiftLayers, void* t_userData);

    static int defIntegerCallback(defrCallbackType_e t_type, int t_number, void* t_userData);

    static int defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData);

    static int defSpecialNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData);

    static int defNonDefaultCallback(defrCallbackType_e t_type, defiNonDefault* t_rul, void* t_userData);

    static int defPathCallback(defrCallbackType_e t_type, defiPath* t_path, void* t_userData);

    static int defPinCallback(defrCallbackType_e t_type, defiPin* t_pinProp, void* t_userData);

    static int defPropCallback(defrCallbackType_e t_type, defiProp* t_prop, void* t_userData);

    static int defRegionCallback(defrCallbackType_e t_type, defiRegion* t_region, void* t_userData);

    static int defRowCallback(defrCallbackType_e t_type, defiRow* t_row, void* t_userData);

    static int defScanchainCallback(defrCallbackType_e t_type, defiScanchain* t_scanchain, void* t_userData);

    static int defSlotCallback(defrCallbackType_e t_type, defiSlot* t_slot, void* t_userData);

    static int defStringCallback(defrCallbackType_e t_type, const char* t_string, void* t_userData);

    static int defStylesCallback(defrCallbackType_e t_type, defiStyles* t_style, void* t_userData);

    static int defTrackCallback(defrCallbackType_e t_type, defiTrack* t_track, void* t_userData);

    static int defViaCallback(defrCallbackType_e t_type, defiVia* t_via, void* t_userData);

    static int defVoidCallback(defrCallbackType_e t_type, void* t_variable, void* t_userData);
};

#endif