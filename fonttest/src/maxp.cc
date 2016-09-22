#include "maxp.h"

#include "utils.h"
#include <glog/logging.h>

MaxpSubTable::MaxpSubTable(const void* ptr, size_t length) {
  uint8_t* bytes = (uint8_t*)ptr;
  uint32_t version = readU32(bytes, 0);
  if (version == 0x00005000) {
    num_glyphs_ = readU16(bytes, 4);
  } else if (version == 0x00010000) {
    num_glyphs_ = readU16(bytes, 4);
  } else {
    LOG(FATAL) << "Invalid version number";
  }
}
