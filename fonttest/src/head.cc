#include "head.h"

#include "utils.h"
#include <glog/logging.h>

HeadSubTable::HeadSubTable(const void* ptr, size_t length) {
  uint8_t* bytes = (uint8_t*)ptr;
  uint32_t major_version = readU16(bytes, 0);
  uint32_t minor_version = readU16(bytes, 2);
  if (major_version != 0x0001 || minor_version != 0x0000) {
    LOG(FATAL) << "unsupported head version";
  }
  uint32_t magic = readU32(bytes, 12);
  if (magic != 0x5F0F3CF5) {
    LOG(FATAL) << "Unknown magic number: " << std::hex << magic;
  }

  unit_per_em_ = readU16(bytes, 18);
}
