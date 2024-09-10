#ifndef __RGB_HPP__
#define __RGB_HPP__

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

extern "C"
{
#include <zlib.h>
}

namespace rgb
{

namespace details
{

inline uint32_t
swap_endian(const uint32_t value)
{
  return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) | ((value << 24) & 0xFF000000);
}

} // namespace details

constexpr uint8_t __signature__[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

struct IHDR
{
  uint32_t m_size        = details::swap_endian(0x0D);
  uint8_t  m_id[4]       = { 0x49, 0x48, 0x44, 0x52 };
  uint8_t  m_bpp         = 0x08;
  uint8_t  m_color       = 0x02;
  uint8_t  m_compression = 0x00;
  uint8_t  m_filter      = 0x00;
  uint8_t  m_interlace   = 0x00;
};

struct IDAT
{
  uint8_t m_id[4] = { 0x49, 0x44, 0x41, 0x54 };
};

struct IEND
{
  uint32_t m_size     = 0x00;
  uint8_t  m_id[4]    = { 0x49, 0x45, 0x4E, 0x44 };
  uint8_t  m_crc32[4] = { 0xAE, 0x42, 0x60, 0x82 };
};

void
make_and_save_image(const uint8_t* data, const uint32_t width, const uint32_t height, const std::filesystem::path& save_path);

} // namespace rgb

#endif