#pragma once

#include "glyf.h"
#include <vector>

class CvtSubTable;

class Rasterizer {
 public:
  Rasterizer(int px, int unit_per_em, CvtSubTable* cvt)
      : Rasterizer(unit_per_em / px, cvt) {}
  explicit Rasterizer(int grid_size, CvtSubTable* cvt)
      : grid_size_(grid_size), cvt_(cvt) {}

  void rasterize(const SimpleGlyphData& glyphData, std::vector<char>* out,
                 int* x_pixel_num);

  int grid_size() const { return grid_size_; }

 private:
  int grid_size_;
  CvtSubTable* cvt_;
};

