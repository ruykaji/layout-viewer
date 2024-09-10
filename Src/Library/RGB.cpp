#include "Include/RGB.hpp"

namespace rgb
{

void
make_and_save_image(const uint8_t* data, const uint32_t width, const uint32_t height, const std::filesystem::path& save_path)
{
  const std::size_t    data_size = 3 * width * height;
  std::vector<uint8_t> filtered_data;

  /** Add filter byte (0x00 = No filter) for each scanline */
  for(uint32_t y = 0; y < height; ++y)
    {
      filtered_data.push_back(0x00);
      filtered_data.insert(filtered_data.end(), data + y * width * 3, data + (y + 1) * width * 3);
    }

  uLongf               compressed_size = compressBound(filtered_data.size());
  std::vector<uint8_t> compressed_data(compressed_size);

  int                  result = compress(compressed_data.data(), &compressed_size, filtered_data.data(), filtered_data.size());
  compressed_data.resize(compressed_size);

  if(result != Z_OK)
    {
      throw std::runtime_error("Compression failed with error code: " + std::to_string(result));
    }

  std::ofstream file_out(save_path, std::ios::binary);
  if(!file_out.is_open() || !file_out.good())
    {
      throw std::runtime_error("Failed to open file: " + save_path.string());
    }

  const IHDR ihdr;
  const IDAT idat;
  const IEND iend;

  uint32_t   width_be           = details::swap_endian(width);
  uint32_t   height_be          = details::swap_endian(height);
  uint32_t   compressed_size_be = details::swap_endian(compressed_size);

  /** Write signature */
  file_out.write(reinterpret_cast<const char*>(__signature__), sizeof(__signature__));

  /** Write IHDR chunk and calculate CRC */
  file_out.write(reinterpret_cast<const char*>(&ihdr.m_size), sizeof(ihdr.m_size));
  file_out.write(reinterpret_cast<const char*>(ihdr.m_id), sizeof(ihdr.m_id));
  file_out.write(reinterpret_cast<const char*>(&width_be), sizeof(width_be));
  file_out.write(reinterpret_cast<const char*>(&height_be), sizeof(height_be));
  file_out.write(reinterpret_cast<const char*>(&ihdr.m_bpp), sizeof(ihdr.m_bpp));
  file_out.write(reinterpret_cast<const char*>(&ihdr.m_color), sizeof(ihdr.m_color));
  file_out.write(reinterpret_cast<const char*>(&ihdr.m_compression), sizeof(ihdr.m_compression));
  file_out.write(reinterpret_cast<const char*>(&ihdr.m_filter), sizeof(ihdr.m_filter));
  file_out.write(reinterpret_cast<const char*>(&ihdr.m_interlace), sizeof(ihdr.m_interlace));

  /** Calculate IHDR CRC */
  uint32_t ihdr_crc = crc32(0, reinterpret_cast<const uint8_t*>(&ihdr.m_id), 17); /** ID (4 bytes) + rest of IHDR (13 bytes) */
  ihdr_crc          = details::swap_endian(ihdr_crc);
  file_out.write(reinterpret_cast<const char*>(&ihdr_crc), sizeof(ihdr_crc));

  /** Write IDAT chunk and CRC */
  file_out.write(reinterpret_cast<const char*>(&compressed_size_be), sizeof(compressed_size_be));
  file_out.write(reinterpret_cast<const char*>(idat.m_id), sizeof(idat.m_id));
  file_out.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_size);

  /** Calculate IDAT CRC */
  uint32_t idat_crc = crc32(0, reinterpret_cast<const uint8_t*>(idat.m_id), 4 + compressed_size); /** ID (4 bytes) + compressed data */
  idat_crc          = details::swap_endian(idat_crc);
  file_out.write(reinterpret_cast<const char*>(&idat_crc), sizeof(idat_crc));

  /** Write IEND chunk */
  file_out.write(reinterpret_cast<const char*>(&iend.m_size), sizeof(iend.m_size));
  file_out.write(reinterpret_cast<const char*>(&iend.m_id), sizeof(iend.m_id));
  file_out.write(reinterpret_cast<const char*>(&iend.m_crc32), sizeof(iend.m_crc32));

  file_out.close();
}

} // namespace rgb
