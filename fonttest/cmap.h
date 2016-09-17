#pragma once

#include <stddef.h>
#include <stdint.h>

class CmapSubTable {
 public:
  CmapSubTable(const void* ptr, size_t length);

  uint32_t findGlyphId(uint32_t ch, uint32_t vs);

 private:
  uint32_t findFromFormat4(uint32_t ch);
  uint32_t findFromFormat12(uint32_t ch);

  uint8_t* format4_ptr_;
  uint8_t* format12_ptr_;
  uint8_t* format14_ptr_;

};
