#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>

class Image {
 public:
  Image(uint32_t width, uint32_t height, int32_t x_origin, int32_t y_origin)
      : width_(width), height_(height), x_origin_(x_origin), y_origin_(y_origin) {
    alloc();
  }

  void drawLine(int32_t from_x, int32_t from_y, int32_t to_x, int32_t to_y);
  void saveToBMP(const std::string& fname);

 private:
  void alloc();
  uint32_t& pixelAt(uint32_t x, uint32_t y);

  uint32_t width_;
  uint32_t height_;

  int32_t x_origin_;
  int32_t y_origin_;

  uint32_t* data_;
};
