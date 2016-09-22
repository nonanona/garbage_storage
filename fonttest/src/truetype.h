#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#include "utils.h"

class CmapSubTable;
class CvtSubTable;
class MaxpSubTable;
class LocaSubTable;
class GlyfSubTable;
class HeadSubTable;

class TrueType {
 public:
  TrueType(const std::string& fname);
  virtual ~TrueType();

  std::unique_ptr<CmapSubTable> getCmap() const;
  std::unique_ptr<MaxpSubTable> getMaxp() const;
  std::unique_ptr<LocaSubTable> getLoca() const;
  std::unique_ptr<GlyfSubTable> getGlyf() const;
  std::unique_ptr<HeadSubTable> getHead() const;
  std::unique_ptr<CvtSubTable> getCvt() const;

  const void* getTable(uint32_t tag, size_t* length) const;
  void dump() const;

 private:
  struct OffsetTable {
    uint32_t tag;
    uint32_t check_sum;
    uint32_t offset;
    uint32_t length;
  };

  void readHeader();

  void* ptr_;
  size_t length_;

  uint16_t num_tables_;
  uint16_t search_range_;
  uint16_t entry_selector_;
  uint16_t range_shift_;

  std::vector<OffsetTable> tables_;
};
