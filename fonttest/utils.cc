#include "utils.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <glog/logging.h>

uint32_t makeTag(char c1, char c2, char c3, char c4) {
  return c1 << 24 | c2 << 16 | c3 << 8 | c4;
}

uint32_t readU32(const void* data, size_t offset) {
  const uint8_t* bytes = (const uint8_t*) data;
  return bytes[offset] << 24 | bytes[offset + 1] << 16 |
      bytes[offset + 2] << 8 | bytes[offset + 3];
}

uint32_t readU24(const void* data, size_t offset) {
  const uint8_t* bytes = (const uint8_t*) data;
  return bytes[offset] << 16 | bytes[offset + 1] << 8 | bytes[offset + 1];
}

uint16_t readU16(const void* data, size_t offset) {
  const uint8_t* bytes = (const uint8_t*) data;
  return bytes[offset] << 8 | bytes[offset + 1];
}

uint16_t readS16(const void* data, size_t offset) {
  const uint8_t* bytes = (const uint8_t*) data;
  return static_cast<int16_t>(bytes[offset] << 8 | bytes[offset + 1]);
}
