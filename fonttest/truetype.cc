#include "truetype.h"

#include "utils.h"
#include "cmap.h"
#include "maxp.h"
#include "loca.h"
#include "glyf.h"
#include "head.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <glog/logging.h>

TrueType::TrueType(const std::string& fname) {
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
  readHeader();
}

TrueType::~TrueType() {
  munmap(ptr_, length_);
}

void TrueType::readHeader() {
  const size_t kOpenTypeHeaderSize = 12;
  if (length_ < kOpenTypeHeaderSize)
    LOG(FATAL) << "Invalid length of file";
  uint32_t sfnt_ver = readU32(ptr_, 0);
  if (sfnt_ver != 0x00010000)
    LOG(FATAL) << "Invalid sfnt version";
  num_tables_ = readU16(ptr_, 4);
  if (num_tables_ == 0)
    LOG(FATAL) << "Table is empty";
  if (length_ < num_tables_ * sizeof(OffsetTable) + kOpenTypeHeaderSize)
    LOG(FATAL) << "Invalid length of file: less than offset table size";
  range_shift_ = readU16(ptr_, 6);
  entry_selector_ = readU16(ptr_, 8);
  range_shift_ = readU16(ptr_, 10);

  tables_.resize(num_tables_);

  for (size_t i = 0; i < num_tables_; ++i) {
    const size_t tableOffset = kOpenTypeHeaderSize + sizeof(OffsetTable) * i;
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
