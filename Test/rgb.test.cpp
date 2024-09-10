#include <gtest/gtest.h>

#include "Include/RGB.hpp"

std::vector<uint8_t>
generate_colorful_checkerboard(const uint32_t width, const uint32_t height)
{
  static const std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> colors = {
    { 255, 0, 0 },   /**  Red     */
    { 0, 255, 0 },   /**  Green   */
    { 0, 0, 255 },   /**  Blue    */
    { 255, 255, 0 }, /**  Yellow  */
    { 0, 255, 255 }, /**  Cyan    */
    { 255, 0, 255 }, /**  Magenta */
    { 255, 165, 0 }, /**  Orange  */
    { 128, 0, 128 }  /**  Purple  */
  };

  std::vector<uint8_t> image_data(3 * width * height);

  for(uint32_t y = 0; y < height; ++y)
    {
      for(uint32_t x = 0; x < width; ++x)
        {
          int color_index                     = ((x / 10) + (y / 10)) % 8;
          auto [r, g, b]                      = colors[color_index];

          image_data[(y * width + x) * 3 + 0] = r;
          image_data[(y * width + x) * 3 + 1] = g;
          image_data[(y * width + x) * 3 + 2] = b;
        }
    }

  return image_data;
}

TEST(RGBTest, MakeColorfulCheckerboardImage)
{
  const uint32_t       width      = 256;
  const uint32_t       height     = 256;
  std::vector<uint8_t> image_data = generate_colorful_checkerboard(width, height);

  rgb::make_and_save_image(image_data.data(), width, height, "./test_image.png");
}

int
main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
