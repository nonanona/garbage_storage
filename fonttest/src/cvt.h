#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

class CvtSubTable {
 public:
  CvtSubTable(const void* ptr, size_t length);

  const std::vector<int16_t>& cvt() const { return cvt_; }

 private:
  std::vector<int16_t> cvt_;
};
