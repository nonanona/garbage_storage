#include "truetype.h"

#include "utils.h"
#include "cmap.h"
#include "maxp.h"
#include "loca.h"
#include "glyf.h"
#include "head.h"
#include "cvt.h"
#include "prep.h"
#include "fpgm.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <glog/logging.h>

TrueType::TrueType(const std::string& fname, int index) {
  int fd = open(fname.c_str(), O_RDONLY);
  struct stat st = {};
  if (fstat(fd, &st) != 0){
    LOG(FATAL) << "Failed to open font file: " << fname;
  }
  length_ = st.st_size;
  ptr_ = mmap(nullptr, length_, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr_ == nullptr) {
    LOG(FATAL) << "Failed to map font file: " << fname;
  }
  readHeader(index);
}

TrueType::~TrueType() {
  munmap(ptr_, length_);
}

void TrueType::readHeader(int index) {
  const size_t kOpenTypeHeaderSize = 12;
  if (length_ < kOpenTypeHeaderSize)
    LOG(FATAL) << "Invalid length of file";
  uint32_t sfnt_ver = readU32(ptr_, 0);

  if (sfnt_ver == 0x00010000 || sfnt_ver == makeTag('O', 'T', 'T', 'O')) {
    if (index != 0)
      LOG(FATAL) << "Specified font file is not a collection.";

    readOffsetTables(0);
    return;
  } else if (sfnt_ver == makeTag('t', 't', 'c', 'f')) {
    // Font Collection
    uint32_t version = readU32(ptr_, 4);

    if (version != 0x00010000 && version != 0x00020000)
      LOG(FATAL) << "Not supported version: " << std::hex << version;

    uint32_t numFonts = readU32(ptr_, 8);
    uint32_t offsetTableOffset = readU32(ptr_, 12 + 4 * index);
    readOffsetTables(offsetTableOffset);
  } else {
    LOG(FATAL) << "Unknown sfnt version: " << std::hex << sfnt_ver;
  }
}

void TrueType::readOffsetTables(size_t tblStartOffset) {
  const size_t kOpenTypeHeaderSize = 12;
  uint32_t sfnt_ver = readU32(ptr_, tblStartOffset);

  if (sfnt_ver != 0x00010000 && sfnt_ver != makeTag('O', 'T', 'T', 'O'))
    LOG(FATAL) << "Invalid sfnt version: " << std::hex << sfnt_ver;

  num_tables_ = readU16(ptr_, tblStartOffset + 4);
  if (num_tables_ == 0)
    LOG(FATAL) << "Table is empty";
  if (length_ < num_tables_ * sizeof(OffsetTable) + kOpenTypeHeaderSize)
    LOG(FATAL) << "Invalid length of file: less than offset table size";
  range_shift_ = readU16(ptr_, tblStartOffset + 6);
  entry_selector_ = readU16(ptr_, tblStartOffset + 8);
  range_shift_ = readU16(ptr_, tblStartOffset + 10);

  tables_.resize(num_tables_);

  for (size_t i = 0; i < num_tables_; ++i) {
    const size_t tableOffset = tblStartOffset + kOpenTypeHeaderSize + sizeof(OffsetTable) * i;
    tables_[i].tag = readU32(ptr_, tableOffset);
    tables_[i].check_sum = readU32(ptr_, tableOffset + 4);
    tables_[i].offset = readU32(ptr_, tableOffset + 8);
    tables_[i].length = readU32(ptr_, tableOffset + 12);
  }
}

void TrueType::dump() const {
  LOG(ERROR) << "TrueType:";
  LOG(ERROR) << "  Number of tables: " << num_tables_;
  for (const OffsetTable& table : tables_) {
      LOG(ERROR) << "  Table: "
          << (char)((table.tag >> 24) & 0xFF)
          << (char)((table.tag >> 16) & 0xFF)
          << (char)((table.tag >> 8) & 0xFF)
          << (char)((table.tag));
      LOG(ERROR) << "    Offset: 0x" << std::hex << table.offset;
      LOG(ERROR) << "    Length: " << table.length;
  }
}

const void* TrueType::getTable(uint32_t tag, size_t* length) const {
  for (const OffsetTable& offset_table : tables_) {
    if (offset_table.tag == tag) {
      *length = offset_table.length;
      return (uint8_t*)(ptr_) + offset_table.offset;
    }
  }
  return nullptr;
}

std::unique_ptr<CmapSubTable> TrueType::getCmap() const {
  size_t length;
  const void* ptr = getTable(makeTag('c', 'm', 'a', 'p'), &length);
  return std::unique_ptr<CmapSubTable>(new CmapSubTable(ptr, length));
}

std::unique_ptr<MaxpSubTable> TrueType::getMaxp() const {
  size_t length;
  const void* ptr = getTable(makeTag('m', 'a', 'x', 'p'), &length);
  return std::unique_ptr<MaxpSubTable>(new MaxpSubTable(ptr, length));
}

std::unique_ptr<LocaSubTable> TrueType::getLoca() const {
  size_t length;
  const void* ptr = getTable(makeTag('l', 'o', 'c', 'a'), &length);
  std::unique_ptr<MaxpSubTable> maxp = getMaxp();

  return std::unique_ptr<LocaSubTable>(
      new LocaSubTable(ptr, length, maxp->num_glyphs()));
}

std::unique_ptr<GlyfSubTable> TrueType::getGlyf() const {
  size_t length;
  const void* ptr = getTable(makeTag('g', 'l', 'y', 'f'), &length);
  return std::unique_ptr<GlyfSubTable>(new GlyfSubTable(ptr, length));
}

std::unique_ptr<HeadSubTable> TrueType::getHead() const {
  size_t length;
  const void* ptr = getTable(makeTag('h', 'e', 'a', 'd'), &length);
  return std::unique_ptr<HeadSubTable>(new HeadSubTable(ptr, length));
}

std::unique_ptr<FpgmSubTable> TrueType::getFpgm() const {
  size_t length;
  const void* ptr = getTable(makeTag('f', 'p', 'g', 'm'), &length);
  return std::unique_ptr<FpgmSubTable>(new FpgmSubTable(ptr, length));
}

std::unique_ptr<PrepSubTable> TrueType::getPrep() const {
  size_t length;
  const void* ptr = getTable(makeTag('p', 'r', 'e', 'p'), &length);
  return std::unique_ptr<PrepSubTable>(new PrepSubTable(ptr, length));
}

std::unique_ptr<CvtSubTable> TrueType::getCvt() const {
  size_t length;
  const void* ptr = getTable(makeTag('c', 'v', 't', ' '), &length);
  return std::unique_ptr<CvtSubTable>(new CvtSubTable(ptr, length));
}
