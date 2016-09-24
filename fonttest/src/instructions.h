#pragma once

#include "glyf.h"

class TrueType;

class HintStackMachine {
 public:
  static void dumpInstructions(const std::vector<uint8_t>& inst);

  static std::vector<Contour> execute(
      const SimpleGlyphData& glyph,
      int grid_size,
      const TrueType& truetype);
};
