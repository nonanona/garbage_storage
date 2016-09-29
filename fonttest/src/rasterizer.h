#pragma once

#include "glyf.h"
#include <vector>

class TrueType;
class Gui;

class Rasterizer {
 public:
  Rasterizer(int px, int unit_per_em, const TrueType& tt)
      : Rasterizer(unit_per_em / px, tt) {}
  explicit Rasterizer(int grid_size, const TrueType& tt)
      : grid_size_(grid_size), tt_(tt) {}

  void rasterize(const SimpleGlyphData& glyphData, std::vector<char>* out,
                 int* x_pixel_num, Gui* gui);
  bool isBitOn(
      const SimpleGlyphData& glyph,
      const std::vector<Contour>& resolved, int ix, int iy,
      Gui* gui, bool debug_out);
  bool isBitOnByRule2a(
      const SimpleGlyphData& glyph,
      const std::vector<Contour>& resolved, int ix, int iy,
      Gui* gui, bool debug_out);

  bool isBitOnByRule2b(
      const SimpleGlyphData& glyph,
      const std::vector<Contour>& resolved, int ix, int iy,
      Gui* gui, bool debug_out);

  int grid_size() const { return grid_size_; }

 private:
  int grid_size_;
  const TrueType& tt_;
};

