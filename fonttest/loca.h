#pragma once

#include <stddef.h>
#include <stdint.h>

class LocaSubTable {
 public:
  LocaSubTable(const void* ptr, size_t length, uint32_t num_glyphs);

  uint32_t findGlyfOffset(uint16_t glyph_id) const;

 private:
  uint32_t num_glyphs_;

  const uint8_t* ptr_;
  bool is_short_;
};
