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

class Encoder {
public:
    Encoder() = default;
    ~Encoder() = default;

    COPY_CONSTRUCTOR_REMOVE(Encoder);
    ASSIGN_OPERATOR_REMOVE(Encoder);

    void readDef(const std::string_view t_fileName, const std::shared_ptr<Def> t_def);

private:
    // Lef callbacks
    // ==================================================================================================================================================
    static int lefPinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData);

    static int lefObstructionCallback(lefrCallbackType_e t_type, lefiObstruction* t_obstruction, void* t_userData);

    // Def callbacks
    // ==================================================================================================================================================

    static int defDieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData);

    static int defComponentStartCallback(defrCallbackType_e t_type, int t_number, void* t_userData);
    
    static int defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData);

    static int defComponentEndCallback(defrCallbackType_e t_type, void* t, void* t_userData);

    static int defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData);

    static int defSpecialNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData);

    static int defPinCallback(defrCallbackType_e t_type, defiPin* t_pinProp, void* t_userData);

    static int defViaCallback(defrCallbackType_e t_type, defiVia* t_via, void* t_userData);
};

#endif