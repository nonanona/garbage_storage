#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

class FpgmSubTable {
 public:
  FpgmSubTable(const void* ptr, size_t length);

  const std::vector<uint8_t>& instructions() const {
    return instructions_;
  }

 private:
  std::vector<uint8_t> instructions_;
};
