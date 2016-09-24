#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>
#include <memory>
#include <string>
#include <sstream>

struct GlyphPoint {
  GlyphPoint(int16_t x, int16_t y, bool on) : x(x), y(y), on_curve(on) {}
  int16_t x;
  int16_t y;
  bool on_curve;

  std::string toString() const {
    std::stringstream ss;
    ss << "(" << x << ", " << y << "): ";
    ss << (on_curve ? "ON" : "OFF");
    return ss.str();
  }
};

struct Contour {
  std::vector<GlyphPoint> points;
};

struct GlyfData {
  int16_t num_of_contours;
  int16_t x_min;
  int16_t y_min;
  int16_t x_max;
  int16_t y_max;
};

struct SimpleGlyphData : public GlyfData {
  std::vector<uint8_t> instructions;
  std::vector<Contour> contours;
};

struct CompositeGlyphData : public GlyfData {
};

class GlyfSubTable {
 public:
  GlyfSubTable(const void* ptr, size_t length)
      : ptr_((const uint8_t*)ptr), length_(length) {}

  std::unique_ptr<GlyfData> getGlyfData(uint32_t offset) const;

 private:
  const uint8_t* ptr_;
  size_t length_;
};
