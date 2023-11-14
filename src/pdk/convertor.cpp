#include <algorithm>
#include <fstream>
#include <vector>

#include "pdk/convertor.hpp"

#define WRITE_TO_BINARY(file, var) file.write(reinterpret_cast<const char*>(&var), sizeof(var))
#define READ_FROM_BINARY(file, var) file.read(reinterpret_cast<char*>(&var), sizeof(var))
// General functions
// ======================================================================================

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

    lefrSetUnitsCbk(&unitsCallback);
    lefrSetLayerCbk(&layerCallback);
    lefrSetMacroBeginCbk(&macroCallback);
    lefrSetMacroSizeCbk(&macroSizeCallback);
    lefrSetPinCbk(&pinCallback);
    lefrSetObstructionCbk(&obstructionCallback);

    std::vector<std::string> files {};

    for (const auto& entry : std::filesystem::recursive_directory_iterator(t_directory)) {
        if (std::filesystem::is_regular_file(entry)) {
            if (getExtensionAfterFirstDot(entry.path()) == ".lef") {
                files.emplace_back(entry.path().string());
            } else if (getExtensionAfterFirstDot(entry.path()) == ".tlef") {
                files.insert(files.begin(), entry.path().string());
            }
        }
    }

    Data* data = new Data();

    for (auto& fileName : files) {
        auto file = fopen(fileName.c_str(), "r");

        if (file == nullptr) {
            throw std::runtime_error("Error: Can't open a file: " + fileName);
        }

        int readStatus = lefrRead(file, fileName.c_str(), data);

        if (readStatus != 0) {
            throw std::runtime_error("Error: Can't read a file: " + fileName);
        }

        fclose(file);
    }

    lefrClear();

    // Write to binary
    // ======================================================================================

    std::ofstream outFile(t_libPath, std::ios::binary);

    WRITE_TO_BINARY(outFile, data->pdk.scale);

    std::size_t layerMapSize = data->pdk.layers.size();
    WRITE_TO_BINARY(outFile, layerMapSize);

    for (auto& [first, second] : data->pdk.layers) {
        std::size_t keySize = first.size();
        WRITE_TO_BINARY(outFile, keySize);
        outFile.write(first.c_str(), keySize);

        std::size_t typeSize = first.size();
        WRITE_TO_BINARY(outFile, typeSize);
        outFile.write(first.c_str(), typeSize);

        WRITE_TO_BINARY(outFile, second.width);
        WRITE_TO_BINARY(outFile, second.metal);
    }

    std::size_t mapSize = data->pdk.macros.size();
    WRITE_TO_BINARY(outFile, mapSize);

    for (auto& [first, second] : data->pdk.macros) {
        // Write map objest's key
        std::size_t keySize = first.size();
        WRITE_TO_BINARY(outFile, keySize);
        outFile.write(first.c_str(), keySize);

        // Write macro
        std::size_t macroNameSize = second.name.size();
        WRITE_TO_BINARY(outFile, macroNameSize);
        outFile.write(second.name.c_str(), macroNameSize);

        WRITE_TO_BINARY(outFile, second.size);

        // Write macro's pins
        std::size_t pinVectorSize = second.pins.size();
        WRITE_TO_BINARY(outFile, pinVectorSize);

        for (auto& pin : second.pins) {
            std::size_t pinName = pin.name.size();
            WRITE_TO_BINARY(outFile, pinName);
            outFile.write(pin.name.c_str(), pinName);

            std::size_t pinUseSize = pin.use.size();
            WRITE_TO_BINARY(outFile, pinUseSize);
            outFile.write(pin.use.c_str(), pinUseSize);

            // Write pin's ports
            std::size_t portVectorSize = pin.ports.size();
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
        std::size_t obsVectorSize = second.obstruction.geometry.size();
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

    std::size_t layerMapSize;
    READ_FROM_BINARY(inFile, layerMapSize);

    for (std::size_t i = 0; i < layerMapSize; ++i) {
        PDK::Layer layer {};

        std::size_t keySize;
        READ_FROM_BINARY(inFile, keySize);
        std::string layerName(keySize, '\0');
        inFile.read(&layerName[0], keySize);

        std::size_t typeSize;
        READ_FROM_BINARY(inFile, typeSize);
        std::string typeName(typeSize, '\0');
        inFile.read(&typeName[0], typeSize);

        layer.type = typeName;

        READ_FROM_BINARY(inFile, layer.width);
        READ_FROM_BINARY(inFile, layer.metal);

        t_pdk.layers[layerName] = layer;
    }

    std::size_t mapSize;
    READ_FROM_BINARY(inFile, mapSize);

    for (std::size_t i = 0; i < mapSize; ++i) {
        // Read key
        std::size_t keyStringSize;
        READ_FROM_BINARY(inFile, keyStringSize);
        std::string key(keyStringSize, '\0');
        inFile.read(&key[0], keyStringSize);

        // Read macro
        PDK::Macro macro {};

        std::size_t nameStringSize;
        READ_FROM_BINARY(inFile, nameStringSize);
        std::string name(nameStringSize, '\0');
        inFile.read(&name[0], nameStringSize);

        macro.name = name;

        PointF size {};
        READ_FROM_BINARY(inFile, size);

        macro.size = size;

        std::size_t pinVectorSize {};
        READ_FROM_BINARY(inFile, pinVectorSize);

        for (std::size_t i = 0; i < pinVectorSize; ++i) {
            PDK::Macro::Pin pin {};

            std::size_t pinNameSize {};
            READ_FROM_BINARY(inFile, pinNameSize);
            std::string pinName(pinNameSize, '\0');
            inFile.read(&pinName[0], pinNameSize);

            pin.name = pinName;

            std::size_t pinUseSize {};
            READ_FROM_BINARY(inFile, pinUseSize);
            std::string pinUse(pinUseSize, '\0');
            inFile.read(&pinUse[0], pinUseSize);

            pin.use = pinUse;

            std::size_t portVectorSize {};
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

        std::size_t obsVectorSize {};
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

int Convertor::unitsCallback(lefrCallbackType_e t_type, lefiUnits* t_units, void* t_userData)
{
    if (t_type != lefrUnitsCbkType) {
        return 2;
    }

    Data* data = static_cast<Data*>(t_userData);
    data->pdk.databaseNumber = t_units->databaseNumber();

    return 0;
}

int Convertor::layerCallback(lefrCallbackType_e t_type, lefiLayer* t_layer, void* t_userData)
{
    if (t_type != lefrLayerCbkType) {
        return 2;
    }

    std::string type { t_layer->type() };

    if (type == "ROUTING" || type == "CUT") {
        std::string name { t_layer->name() };
        Data* data = static_cast<Data*>(t_userData);

        if (data->pdk.layers.count(name) == 0) {
            PDK::Layer layer {};

            layer.type = type;
            layer.width = t_layer->width() * data->pdk.databaseNumber;
            layer.metal = static_cast<MetalLayer>(data->pdk.layers.size());

            data->pdk.layers[name] = layer;
            data->pdk.scale = std::min(data->pdk.scale, layer.width / data->config.getPdkScaleFactor());
        }
    }

    return 0;
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

    data->pdk.macros[data->lastMacro].size = PointF(t_numbers.x * data->pdk.databaseNumber, t_numbers.y * data->pdk.databaseNumber);

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
    PDK::Layer layer {};

    for (std::size_t i = 0; i < t_pin->numPorts(); ++i) {
        lefiGeometries* portGeom = t_pin->port(i);

        for (std::size_t j = 0; j < portGeom->numItems(); ++j) {
            switch (portGeom->itemType(j)) {
            case lefiGeomEnum::lefiGeomLayerE: {
                layer = data->pdk.layers[portGeom->getLayer(j)];
                break;
            }
            case lefiGeomEnum::lefiGeomRectE: {
                lefiGeomRect* portRect = portGeom->getRect(j);
                double xLeft = portRect->xl * data->pdk.databaseNumber;
                double yTop = portRect->yl * data->pdk.databaseNumber;
                double xRight = portRect->xh * data->pdk.databaseNumber;
                double yBottom = portRect->yh * data->pdk.databaseNumber;

                pinMaxScale = std::max(pinMaxScale, std::min(std::abs(xRight - xLeft) / data->config.getPdkScaleFactor(), std::abs(yBottom - yTop)) / data->config.getPdkScaleFactor());

                pin.ports.emplace_back(RectangleF(xLeft, yTop, xRight, yBottom, layer.metal));
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

    Data* data = static_cast<Data*>(t_userData);

    PDK::Macro::OBS obs {};
    lefiGeometries* osbrGeom = t_obstruction->geometries();
    PDK::Layer layer {};

    for (std::size_t i = 0; i < osbrGeom->numItems(); ++i) {
        switch (osbrGeom->itemType(i)) {
        case lefiGeomEnum::lefiGeomLayerE: {
            layer = data->pdk.layers[osbrGeom->getLayer(i)];
            break;
        }
        case lefiGeomEnum::lefiGeomRectE: {
            lefiGeomRect* portRect = osbrGeom->getRect(i);
            double xLeft = portRect->xl * data->pdk.databaseNumber;
            double yTop = portRect->yl * data->pdk.databaseNumber;
            double xRight = portRect->xh * data->pdk.databaseNumber;
            double yBottom = portRect->yh * data->pdk.databaseNumber;

            obs.geometry.emplace_back(RectangleF(xLeft, yTop, xRight, yBottom, layer.metal));
            break;
        }
        default:
            break;
        }
    }

    data->pdk.macros[data->lastMacro].obstruction = obs;

    return 0;
}