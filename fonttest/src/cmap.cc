#include "cmap.h"

#include "utils.h"

#include <glog/logging.h>

CmapSubTable::CmapSubTable(const void* ptr, size_t length)
    : format4_ptr_(nullptr), format12_ptr_(nullptr), format14_ptr_(nullptr) {
  uint8_t* bytes = (uint8_t*)ptr;
  uint32_t table_version = readU16(bytes, 0);
  if (table_version != 0)
    LOG(FATAL) << "Invalid table version.";
  uint32_t num_tables = readU16(bytes, 2);
  if (num_tables == 0)
    LOG(FATAL) << "Empty tables.";

  for (size_t i = 0; i < num_tables; ++i) {
    const size_t kHeaderSize = 8;
    uint32_t headerOffset =  4 + kHeaderSize * i;

    uint32_t platform_id = readU16(bytes, headerOffset);
    uint32_t encoding_id = readU16(bytes, headerOffset + 2);
    uint32_t offset = readU32(bytes, headerOffset + 4);

    if (platform_id == 3 && encoding_id == 1) {
      format4_ptr_ = (uint8_t*)ptr + offset;
    } else if (platform_id == 3 && encoding_id == 10) {
      format12_ptr_ = (uint8_t*)ptr + offset;
    } else if (platform_id == 0 && encoding_id == 15) {
      format14_ptr_ =  (uint8_t*)ptr + offset;
    }
  }
}

uint32_t CmapSubTable::findGlyphId(uint32_t ch, uint32_t vs) {
  if (vs != 0) {
    LOG(FATAL) << "Vs is not supported";
  }

  if (format12_ptr_) {
    return findFromFormat12(ch);
  }

  if (format4_ptr_) {
    return findFromFormat4(ch);
  }
  LOG(FATAL) << "Not Found.";
}

uint32_t CmapSubTable::findFromFormat4(uint32_t ch) {
  const size_t kSegCountOffset = 6;

  uint32_t format = readU16(format4_ptr_, 0);
  if (format != 4)
    LOG(FATAL) << "Invalid format id";
  uint32_t length = readU16(format4_ptr_, 2);
  uint32_t seg_count = readU16(format4_ptr_, kSegCountOffset) >> 1;

  for (size_t i = 0; i < seg_count; ++i) {
    const size_t kEndCountOffset = 14;

    const size_t endCountOffset = kEndCountOffset + 2 * i;
    const size_t startCountOffset = kEndCountOffset + 2 +  2 * (seg_count + i);
    uint32_t end = readU16(format4_ptr_, endCountOffset);
    uint32_t start = readU16(format4_ptr_, startCountOffset);

    const size_t deltaOffsetX = startCountOffset + 2 * seg_count;
    if (ch < start || end < ch)
      continue;

    if (end < start)
      LOG(FATAL) << "Invlaid segment range";

    const size_t rangeOffset = startCountOffset + 2 * (2 * seg_count);
    uint32_t rangeOffsetValue = readU16(format4_ptr_, rangeOffset);
    if (rangeOffsetValue == 0) {
      const size_t deltaOffset = startCountOffset + 2 * seg_count;
      int16_t deltaOffsetValue =
          static_cast<int16_t>(readU16(format4_ptr_, deltaOffset));
      return ch + deltaOffsetValue;
    } else {
      LOG(ERROR) << "Not yet implemented.";
    }

  }
  return (uint32_t)-1;
}

uint32_t CmapSubTable::findFromFormat12(uint32_t ch) {
  uint32_t format = readU16(format12_ptr_, 0);
  if (format != 12)
    LOG(FATAL) << "Invalid format id";
  uint32_t nGroups = readU32(format12_ptr_, 12);

  const size_t kGroupOffset = 16;
  for (size_t i = 0; i < nGroups; ++i) {
    uint32_t start = readU32(format12_ptr_, kGroupOffset + i * 12);
    uint32_t end = readU32(format12_ptr_, kGroupOffset + i * 12 + 4);
    uint32_t glyphId = readU32(format12_ptr_, kGroupOffset + i * 12 + 8);
    if (ch < start || end < ch)
      continue;
    return glyphId + (ch - start);
  }
  return (uint32_t)-1;
}
