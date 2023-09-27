#include <iostream>
#include <stdexcept>
#include <defrReader.hpp>

#include "def_encoder.hpp"

void Encoder::initParser()
{
    int initStatus = defrInitSession();

    if (initStatus != 0) {
        throw std::runtime_error("Error: cant't initialize parser!");
    }
}

void Encoder::setFile(const std::string_view t_fileName)
{
    m_file.reset(fopen(t_fileName.cbegin(), "r"));

    if (m_file == nullptr) {
        throw std::runtime_error("Error: Can't open a file!");
    }
}