#include <fstream>

#include "pdk/convertor.hpp"

#define WRITE_TO_BINARY(file, var) file.write(reinterpret_cast<const char*>(&var), sizeof(var))

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

// Class methods
// ======================================================================================

void Convertor::serialize(const std::string& t_directory)
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

    PDK pdk {};

    for (const auto& entry : std::filesystem::recursive_directory_iterator(t_directory)) {
        if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".lef") {
            auto file = fopen(entry.path().c_str(), "r");

            if (file == nullptr) {
                throw std::runtime_error("Error: Can't open a file: " + entry.path().string());
            }

            PDK::Macro* macro = new PDK::Macro();

            int readStatus = lefrRead(file, entry.path().c_str(), macro);

            if (readStatus != 0) {
                throw std::runtime_error("Error: Can't read a file: " + entry.path().string());
            }

            fclose(file);

            pdk.macros[macro->name] = *macro;

            delete macro;
        }
    }

    lefrClear();

    // Write to binary
    // ======================================================================================

    std::ofstream outFile("pdk.bin", std::ios::binary);

    size_t mapSize = pdk.macros.size();
    outFile.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));

    for (auto& [first, second] : pdk.macros) {
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
};

void Convertor::deserialize(const std::string& t_fileName, PDK& t_pdk)
{
    std::ifstream inFile(t_fileName, std::ios::binary);

    size_t mapSize;
    inFile.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));

    for (size_t i = 0; i < mapSize; ++i) {
        // Read key
        size_t keyStringSize;
        inFile.read(reinterpret_cast<char*>(&keyStringSize), sizeof(keyStringSize));
        std::string key(keyStringSize, '\0');
        inFile.read(&key[0], keyStringSize);

        // Read macro
        PDK::Macro macro {};

        size_t nameStringSize;
        inFile.read(reinterpret_cast<char*>(&nameStringSize), sizeof(nameStringSize));
        std::string name(nameStringSize, '\0');
        inFile.read(&name[0], nameStringSize);

        macro.name = name;

        PointF size {};
        inFile.read(reinterpret_cast<char*>(&size), sizeof(size));

        macro.size = size;

        size_t pinVectorSize {};
        inFile.read(reinterpret_cast<char*>(&pinVectorSize), sizeof(pinVectorSize));

        for (std::size_t i = 0; i < pinVectorSize; ++i) {
            PDK::Macro::Pin pin {};

            size_t pinNameSize {};
            inFile.read(reinterpret_cast<char*>(&pinNameSize), sizeof(pinNameSize));
            std::string pinName(pinNameSize, '\0');
            inFile.read(&pinName[0], pinNameSize);

            pin.name = pinName;

            size_t pinUseSize {};
            inFile.read(reinterpret_cast<char*>(&pinUseSize), sizeof(pinUseSize));
            std::string pinUse(pinUseSize, '\0');
            inFile.read(&pinUse[0], pinUseSize);

            pin.use = pinUse;

            size_t portVectorSize {};
            inFile.read(reinterpret_cast<char*>(&portVectorSize), sizeof(portVectorSize));

            for (std::size_t j = 0; j < portVectorSize; ++j) {
                RectangleF port {};

                inFile.read(reinterpret_cast<char*>(&port.type), sizeof(port.type));
                inFile.read(reinterpret_cast<char*>(&port.layer), sizeof(port.layer));

                for (std::size_t k = 0; k < 4; ++k) {
                    inFile.read(reinterpret_cast<char*>(&port.vertex[k]), sizeof(port.vertex[k]));
                }

                pin.ports.emplace_back(port);
            }

            macro.pins.emplace_back(pin);
        }

        size_t obsVectorSize {};
        inFile.read(reinterpret_cast<char*>(&obsVectorSize), sizeof(obsVectorSize));

        for (std::size_t i = 0; i < obsVectorSize; ++i) {
            RectangleF port {};

            inFile.read(reinterpret_cast<char*>(&port.type), sizeof(port.type));
            inFile.read(reinterpret_cast<char*>(&port.layer), sizeof(port.layer));

            for (std::size_t k = 0; k < 4; ++k) {
                inFile.read(reinterpret_cast<char*>(&port.vertex[k]), sizeof(port.vertex[k]));
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

    PDK::Macro* macro = static_cast<PDK::Macro*>(t_userData);

    macro->name = std::string(t_string);

    return 0;
};

int Convertor::macroSizeCallback(lefrCallbackType_e t_type, lefiNum t_numbers, void* t_userData)
{
    if (t_type != lefrMacroSizeCbkType) {
        return 2;
    }

    PDK::Macro* macro = static_cast<PDK::Macro*>(t_userData);

    macro->size = PointF(t_numbers.x, t_numbers.y);

    return 0;
};

int Convertor::pinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData)
{
    if (t_type != lefrPinCbkType) {
        return 2;
    }

    PDK::Macro::Pin pin {};
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

                pin.ports.emplace_back(RectangleF(xLeft, yTop, xRight, yBottom, convertNameToML(layer)));
                break;
            }
            default:
                break;
            }
        }
    }

    PDK::Macro* macro = static_cast<PDK::Macro*>(t_userData);

    if (t_pin->hasUse()) {
        pin.use = std::string(t_pin->use());
    }

    pin.name = std::string(t_pin->name());
    macro->pins.emplace_back(pin);

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

    PDK::Macro* macro = static_cast<PDK::Macro*>(t_userData);

    macro->obstruction = obs;

    return 0;
}