#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>
#include <unordered_map>

class Config {
public:
    enum class Mode {
        MAKE_PDK,
        TRAIN,
        TEST
    };

    Config() = default;
    ~Config() = default;

    Config(int t_argc, char const* t_argv[]);

    constexpr inline Mode getMode() const noexcept
    {
        return m_mode;
    }

    constexpr inline double getPdkScaleFactor() const noexcept
    {
        return m_pdkScaleFactor;
    };

    constexpr inline int32_t getCellSize() const noexcept
    {
        return m_cellSize;
    };

    constexpr inline int32_t getBorderSize() const noexcept
    {
        return m_borderSize;
    };

private:
    std::string m_configPath {};
    std::unordered_map<std::string, int8_t> m_settingsEnum {
        { "mode", 0 },
        { "pdk_scale_factor", 1 },
        { "cell_size", 2 },
        { "border_size", 3 },
        { "border_route_size", 4 },
    };

    // Global config
    // =================================================================================

    Mode m_mode { Mode::TRAIN };

    // PDK config
    // =================================================================================
    double m_pdkScaleFactor { 2.0 };

    // Parser config
    // =================================================================================

    int32_t m_cellSize { 84 };
    int32_t m_borderSize { 0 };

    // Train config
    // =================================================================================
};

#endif