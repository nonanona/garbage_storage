#include "loca.h"

#include "utils.h"

#include <glog/logging.h>

LocaSubTable::LocaSubTable(const void* ptr, size_t length, uint32_t num_glyphs)
    : ptr_((const uint8_t*)ptr), num_glyphs_(num_glyphs) {
  if ((num_glyphs + 1) * 2 == length) {
    is_short_ = true;
  } else if ((num_glyphs + 1) * 4 == length) {
    is_short_ = false;
  } else {
    LOG(FATAL) << "Invalid loca table format";
  }
}


uint32_t LocaSubTable::findGlyfOffset(uint16_t glyph_id) const {
  if (is_short_) {
      return readU16(ptr_, glyph_id * 2) * 2;
  } else {
      return readU32(ptr_, glyph_id * 4);
  }
}
