#include "cvt.h"

#include "utils.h"
#include <glog/logging.h>

CvtSubTable::CvtSubTable(const void* ptr, size_t length) {
  if (length % sizeof(int16_t) != 0) {
    LOG(FATAL) << "length is invalid. : " << length;
  }
  size_t n = length / sizeof(int16_t);
  for (size_t i = 0; i < n; ++i) {
    cvt_.push_back(readS16(ptr, i * sizeof(int16_t)));
  }
}
