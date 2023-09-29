#ifndef __DEF_H__
#define __DEF_H__

struct Rect {
    int32_t left {};
    int32_t top {};
    int32_t width {};
    int32_t height {};

    Rect() = default;
    ~Rect() = default;

    Rect(const int32_t& t_left, const int32_t& t_top, const int32_t& t_width, const int32_t& t_height)
        : left(t_left)
        , top(t_top)
        , width(t_width)
        , height(t_height) {};
};

#pragma pack(push, 1)
struct Def {
    Rect dieArea {};

    Def() = default;
    ~Def() = default;
};
#pragma pack(push)

#endif