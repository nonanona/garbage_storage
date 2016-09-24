#include "prep.h"

#include "utils.h"
#include <glog/logging.h>

PrepSubTable::PrepSubTable(const void* ptr, size_t length) {
  uint8_t* bytes = (uint8_t*)ptr;
  instructions_.assign(bytes, bytes + length);
}
