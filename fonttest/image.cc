#include "image.h"

#include "glog/logging.h"

int32_t max(int32_t a, int32_t b) {
  return a > b ? a : b;
}

int32_t min(int32_t a, int32_t b) {
  return a < b ? a : b;
}

double fabs(double x) {
  return x > 0 ? x : -x;
}

void Image::alloc() {
  LOG(ERROR) << width_ << ", " << height_;
  data_ = new uint32_t[width_ * height_];
  for (size_t i = 0; i < width_ * height_; ++i) {
    data_[i] = 0x00FFFFFF;
  }
}

uint32_t& Image::pixelAt(uint32_t x, uint32_t y) {
  return data_[y * width_ + x];
}

void Image::drawLine(int32_t from_x, int32_t from_y, int32_t to_x, int32_t to_y) {
  from_x += x_origin_;
  from_y += y_origin_;
  to_x += x_origin_;
  to_y += y_origin_;

  int32_t delta_x = to_x - from_x;
  int32_t delta_y = to_y - from_y;

  if (delta_x == 0) {
    if (from_x < 0 || from_x >= width_)
      return;
    int32_t draw_y_min = max(min(from_y, to_y), 0);
    int32_t draw_y_max = min(max(from_y, to_y), height_);
    for (int32_t y = draw_y_min; y < draw_y_max; ++y) {
      pixelAt(to_x, y) = 0x00000000;
    }
    return;
  }

  int32_t error_numerator = 0;
  int32_t delta_err_denominator = abs(delta_x);
  int32_t delta_err_numerator = abs(delta_y);

  double delta_err = fabs((double)delta_y / (double)delta_x);

  int32_t y = from_y;
  int32_t step_x = from_x > to_x ? -1 : 1;
  int32_t step_y = from_y > to_y ? -1 : 1;
  for (int32_t x = from_x; x != to_x; x += step_x) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
      pixelAt(x, y) = 0x00000000;
    }

    error_numerator += delta_err_numerator;
    while (error_numerator > delta_err_denominator / 2) {
      if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        pixelAt(x, y) = 0x00000000;
      }
      y += step_y;
      error_numerator -= delta_err_denominator;
    }
  }
}

void leWriteU16(uint8_t* data, size_t offset, uint16_t value) {
  data[offset + 1] = ((value >> 8) & 0xFF);
  data[offset] = (value & 0xFF);
}

void leWriteU32(uint8_t* data, size_t offset, uint16_t value) {
  data[offset + 3] = ((value >> 24) & 0xFF);
  data[offset + 2] = ((value >> 16) & 0xFF);
  data[offset + 1] = ((value >> 8) & 0xFF);
  data[offset] = (value & 0xFF);
}

void Image::saveToBMP(const std::string& fname) {
  uint8_t fileHeader[14] = {};
  fileHeader[0] = 'B';
  fileHeader[1] = 'M';
  leWriteU32(fileHeader, 2, 14 + 40 + width_ * height_ * 4);
  leWriteU32(fileHeader, 10, 14 + 40);

  uint8_t infoHeader[40] = {};
  leWriteU32(infoHeader, 0, 40);
  leWriteU32(infoHeader, 4, width_);
  leWriteU32(infoHeader, 8, height_);
  leWriteU16(infoHeader, 12, 1);
  leWriteU16(infoHeader, 14, 32);

  FILE* fp = fopen(fname.c_str(), "w");
  fwrite(fileHeader, 1, 14, fp);
  fwrite(infoHeader, 1, 40, fp);
  fwrite(data_, 1, width_ * height_ * 4, fp);
  fclose(fp);
}
