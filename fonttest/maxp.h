#pragma once

#include <stddef.h>
#include <stdint.h>

class MaxpSubTable {
 public:
  MaxpSubTable(const void* ptr, size_t length);

  uint32_t num_glyphs() const { return num_glyphs_; }

 private:
  uint32_t num_glyphs_;
};
