#include <fstream>

#include "pdk/convertor.hpp"

#define WRITE_TO_BINARY(file, var) file.write(reinterpret_cast<const char*>(&var), sizeof(var))
#define READ_FROM_BINARY(file, var) file.read(reinterpret_cast<char*>(&var), sizeof(var))
// General functions
// ======================================================================================

inline static MetalLayer convertNameToML(const char* t_name)
{
    static const std::unordered_map<std::string, MetalLayer> nameToMLMap = {
        { "li1", MetalLayer::L1 },
        { "met1", MetalLayer::M1 },
        { "met2", MetalLayer::M2 },
        { "met3", MetalLayer::M3 },
        { "met4", MetalLayer::M4 },
        { "met5", MetalLayer::M5 },
        { "met6", MetalLayer::M6 },
        { "met7", MetalLayer::M7 },
        { "met8", MetalLayer::M8 },
        { "met9", MetalLayer::M9 }
    };

    auto it = nameToMLMap.find(t_name);

    if (it != nameToMLMap.end()) {
        return it->second;
    }

    return MetalLayer::NONE;
}

inline static std::filesystem::path getExtensionAfterFirstDot(const std::filesystem::path& t_path)
{
    std::string filename = t_path.filename().string();
    std::size_t pos = filename.find('.');

    if (pos != std::string::npos) {
        return filename.substr(pos);
    }

    return "";
}

// Class methods
// ======================================================================================

void Convertor::serialize(const std::string& t_directory, const std::string& t_libPath)
{
    // Parse all lef files in directory
    // ======================================================================================

    if (!std::filesystem::exists(t_directory) || !std::filesystem::is_directory(t_directory)) {
        throw std::runtime_error("Invalid directory: " + t_directory);
    }

    int initStatusLef = lefrInitSession();

    if (initStatusLef != 0) {
        throw std::runtime_error("Error: cant't initialize lef parser!");
    }

    lefrSetMacroBeginCbk(&macroCallback);
    lefrSetMacroSizeCbk(&macroSizeCallback);
    lefrSetPinCbk(&pinCallback);
    lefrSetObstructionCbk(&obstructionCallback);

    Data* data = new Data();

    for (const auto& entry : std::filesystem::recursive_directory_iterator(t_directory)) {
        if (std::filesystem::is_regular_file(entry) && getExtensionAfterFirstDot(entry.path()) == ".lef") {
            auto file = fopen(entry.path().c_str(), "r");

            if (file == nullptr) {
                throw std::runtime_error("Error: Can't open a file: " + entry.path().string());
            }

            int readStatus = lefrRead(file, entry.path().c_str(), data);

            if (readStatus != 0) {
                throw std::runtime_error("Error: Can't read a file: " + entry.path().string());
            }

            fclose(file);
        }
    }

    lefrClear();

    // Write to binary
    // ======================================================================================

    std::ofstream outFile(t_libPath, std::ios::binary);

    WRITE_TO_BINARY(outFile, data->pdk.scale);

    size_t mapSize = data->pdk.macros.size();
    WRITE_TO_BINARY(outFile, mapSize);

    for (auto& [first, second] : data->pdk.macros) {
        // Write map objest's key
        size_t keySize = first.size();
        WRITE_TO_BINARY(outFile, keySize);
        outFile.write(first.c_str(), keySize);

        // Write macro
        size_t macroNameSize = second.name.size();
        WRITE_TO_BINARY(outFile, macroNameSize);
        outFile.write(second.name.c_str(), macroNameSize);

        WRITE_TO_BINARY(outFile, second.size);

        // Write macro's pins
        size_t pinVectorSize = second.pins.size();
        WRITE_TO_BINARY(outFile, pinVectorSize);

        for (auto& pin : second.pins) {
            size_t pinName = pin.name.size();
            WRITE_TO_BINARY(outFile, pinName);
            outFile.write(pin.name.c_str(), pinName);

            size_t pinUseSize = pin.use.size();
            WRITE_TO_BINARY(outFile, pinUseSize);
            outFile.write(pin.use.c_str(), pinUseSize);

            // Write pin's ports
            size_t portVectorSize = pin.ports.size();
            WRITE_TO_BINARY(outFile, portVectorSize);

            for (auto& port : pin.ports) {
                WRITE_TO_BINARY(outFile, port.type);
                WRITE_TO_BINARY(outFile, port.layer);

                for (auto& vertex : port.vertex) {
                    WRITE_TO_BINARY(outFile, vertex);
                }
            }
        }

        // Write macro's obs
        size_t obsVectorSize = second.obstruction.geometry.size();
        WRITE_TO_BINARY(outFile, obsVectorSize);

        for (auto& rect : second.obstruction.geometry) {
            WRITE_TO_BINARY(outFile, rect.type);
            WRITE_TO_BINARY(outFile, rect.layer);

            for (auto& vertex : rect.vertex) {
                WRITE_TO_BINARY(outFile, vertex);
            }
        }
    }

    outFile.close();

    delete data;
};

void Convertor::deserialize(const std::string& t_libPath, PDK& t_pdk)
{
    std::ifstream inFile(t_libPath, std::ios::binary);

    READ_FROM_BINARY(inFile, t_pdk.scale);

    size_t mapSize;
    READ_FROM_BINARY(inFile, mapSize);

    for (size_t i = 0; i < mapSize; ++i) {
        // Read key
        size_t keyStringSize;
        READ_FROM_BINARY(inFile, keyStringSize);
        std::string key(keyStringSize, '\0');
        inFile.read(&key[0], keyStringSize);

        // Read macro
        PDK::Macro macro {};

        size_t nameStringSize;
        READ_FROM_BINARY(inFile, nameStringSize);
        std::string name(nameStringSize, '\0');
        inFile.read(&name[0], nameStringSize);

        macro.name = name;

        PointF size {};
        READ_FROM_BINARY(inFile, size);

        macro.size = size;

        size_t pinVectorSize {};
        READ_FROM_BINARY(inFile, pinVectorSize);

        for (std::size_t i = 0; i < pinVectorSize; ++i) {
            PDK::Macro::Pin pin {};

            size_t pinNameSize {};
            READ_FROM_BINARY(inFile, pinNameSize);
            std::string pinName(pinNameSize, '\0');
            inFile.read(&pinName[0], pinNameSize);

            pin.name = pinName;

            size_t pinUseSize {};
            READ_FROM_BINARY(inFile, pinUseSize);
            std::string pinUse(pinUseSize, '\0');
            inFile.read(&pinUse[0], pinUseSize);

            pin.use = pinUse;

            size_t portVectorSize {};
            READ_FROM_BINARY(inFile, portVectorSize);

            for (std::size_t j = 0; j < portVectorSize; ++j) {
                RectangleF port {};

                READ_FROM_BINARY(inFile, port.type);
                READ_FROM_BINARY(inFile, port.layer);

                for (std::size_t k = 0; k < 4; ++k) {
                    READ_FROM_BINARY(inFile, port.vertex[k]);
                }

                pin.ports.emplace_back(port);
            }

            macro.pins.emplace_back(pin);
        }

        size_t obsVectorSize {};
        READ_FROM_BINARY(inFile, obsVectorSize);

        for (std::size_t j = 0; j < obsVectorSize; ++j) {
            RectangleF port {};

            READ_FROM_BINARY(inFile, port.type);
            READ_FROM_BINARY(inFile, port.layer);

            for (std::size_t k = 0; k < 4; ++k) {
                READ_FROM_BINARY(inFile, port.vertex[k]);
            }

            macro.obstruction.geometry.emplace_back(port);
        }

        t_pdk.macros[key] = macro;
    }
}

int Convertor::macroCallback(lefrCallbackType_e t_type, const char* t_string, void* t_userData)
{
    if (t_type != lefrMacroBeginCbkType) {
        return 2;
    }

    Data* data = static_cast<Data*>(t_userData);

    data->pdk.macros[t_string] = PDK::Macro();
    data->pdk.macros[t_string].name = t_string;
    data->lastMacro = t_string;

    return 0;
};

int Convertor::macroSizeCallback(lefrCallbackType_e t_type, lefiNum t_numbers, void* t_userData)
{
    if (t_type != lefrMacroSizeCbkType) {
        return 2;
    }

    Data* data = static_cast<Data*>(t_userData);

    data->pdk.macros[data->lastMacro].size = PointF(t_numbers.x, t_numbers.y);

    return 0;
};

int Convertor::pinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData)
{
    if (t_type != lefrPinCbkType) {
        return 2;
    }

    Data* data = static_cast<Data*>(t_userData);
    PDK::Macro::Pin pin {};
    double pinMaxScale {};
    const char* layer {};

    for (std::size_t i = 0; i < t_pin->numPorts(); ++i) {
        lefiGeometries* portGeom = t_pin->port(i);

        for (std::size_t j = 0; j < portGeom->numItems(); ++j) {
            switch (portGeom->itemType(j)) {
            case lefiGeomEnum::lefiGeomLayerE: {
                layer = portGeom->getLayer(j);
                break;
            }
            case lefiGeomEnum::lefiGeomRectE: {
                lefiGeomRect* portRect = portGeom->getRect(j);
                double xLeft = portRect->xl;
                double yTop = portRect->yl;
                double xRight = portRect->xh;
                double yBottom = portRect->yh;

                pinMaxScale = std::max(pinMaxScale, std::min(std::abs(xRight - xLeft) / 2.0, std::abs(yBottom - yTop)) / 2.0);

                pin.ports.emplace_back(RectangleF(xLeft, yTop, xRight, yBottom, convertNameToML(layer)));
                break;
            }
            default:
                break;
            }
        }
    }

    if (t_pin->hasUse()) {
        pin.use = std::string(t_pin->use());
    }

    pin.name = std::string(t_pin->name());
    data->pdk.macros[data->lastMacro].pins.emplace_back(pin);

    if (pinMaxScale != 0) {
        data->pdk.scale = std::min(data->pdk.scale, pinMaxScale);
    }
    return 0;
}

int Convertor::obstructionCallback(lefrCallbackType_e t_type, lefiObstruction* t_obstruction, void* t_userData)
{
    if (t_type != lefrObstructionCbkType) {
        return 2;
    }

    PDK::Macro::OBS obs {};
    lefiGeometries* osbrGeom = t_obstruction->geometries();
    const char* layer {};

    for (std::size_t i = 0; i < osbrGeom->numItems(); ++i) {
        switch (osbrGeom->itemType(i)) {
        case lefiGeomEnum::lefiGeomLayerE: {
            layer = osbrGeom->getLayer(i);
            break;
        }
        case lefiGeomEnum::lefiGeomRectE: {
            lefiGeomRect* portRect = osbrGeom->getRect(i);
            double xLeft = portRect->xl;
            double yTop = portRect->yl;
            double xRight = portRect->xh;
            double yBottom = portRect->yh;

            obs.geometry.emplace_back(RectangleF(xLeft, yTop, xRight, yBottom, convertNameToML(layer)));
            break;
        }
        default:
            break;
        }
    }

    Data* data = static_cast<Data*>(t_userData);

    data->pdk.macros[data->lastMacro].obstruction = obs;

    return 0;
}