#pragma once

#include <stddef.h>
#include <stdint.h>

class HeadSubTable {
 public:
   HeadSubTable(const void* ptr, size_t length);

   uint32_t unit_per_em() const { return unit_per_em_; }

 private:
   uint32_t unit_per_em_;

};
