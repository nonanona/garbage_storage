#include "glyf.h"

#include "utils.h"
#include <glog/logging.h>

std::unique_ptr<GlyfData> GlyfSubTable::getGlyfData(uint32_t offset) const {
  const uint8_t* glyph = ptr_ + offset;
  int16_t num_of_contours = readS16(glyph, 0);
  if (num_of_contours < 0) 
    LOG(FATAL) << "Composite glyph is not supported.";

  std::unique_ptr<SimpleGlyphData> data(new SimpleGlyphData());
  data->num_of_contours = num_of_contours;
  data->x_min = readS16(glyph, 2);
  data->y_min = readS16(glyph, 4);
  data->x_max = readS16(glyph, 6);
  data->y_max = readS16(glyph, 8);

  const size_t kSimpleGlyfOffset = 10;
  const size_t instLengthOffset = kSimpleGlyfOffset + num_of_contours * 2;
  size_t instLength = readU16(glyph, instLengthOffset);
  data->instructions.resize(instLength);
  memcpy(&data->instructions[0], glyph + instLengthOffset + 2, instLength);

  const uint16_t total_pts =
      readU16(glyph, kSimpleGlyfOffset + (num_of_contours - 1) * 2) + 1;
  std::vector<uint8_t> flags;
  size_t flagOffset = instLengthOffset + 2 + instLength;
  for (; flags.size() < total_pts; ++flagOffset) {
    uint8_t flag = glyph[flagOffset];
    flags.push_back(flag);
    if (flag & (1 << 3)) {
      // repeate
      uint8_t repeate_count = glyph[++flagOffset];
      for (uint8_t j = 0; j < repeate_count; ++j) {
        flags.push_back(flag);
      }
    }
  }
  if (flags.size() != total_pts) {
    LOG(FATAL) << "Invalid count of flags";
  }

  size_t x_cord_offset = flagOffset;
  int16_t prev_x = 0;
  std::vector<int16_t> abs_x;
  for (int pt = 0; pt < total_pts; ++pt) {
    uint8_t flag = flags[pt];

    int16_t x = 0;
    if (flags[pt] & (1 << 1)) {
      x = glyph[x_cord_offset];
      x_cord_offset ++;
      if (!(flags[pt] & (1 << 4))) 
        x *= -1;
    } else {
      if (flags[pt] & (1 << 4)) {
        x = 0;
      } else {
        x = readU16(glyph, x_cord_offset);
        x_cord_offset += 2;
      }
    }
    abs_x.push_back(prev_x += x);
  }

  size_t y_cord_offset = x_cord_offset;
  int16_t prev_y = 0;
  std::vector<int16_t> abs_y;
  for (int pt = 0; pt < total_pts; ++pt) {
    uint8_t flag = flags[pt];

    int16_t y = 0;
    if (flags[pt] & (1 << 2)) {
      y = glyph[y_cord_offset];
      y_cord_offset ++;
      if (!(flags[pt] & (1 << 5))) 
        y *= -1;
    } else {
      if (flags[pt] & (1 << 5)) {
        y = 0;
      } else {
        y = readU16(glyph, y_cord_offset);
        y_cord_offset += 2;
      }
    }
    abs_y.push_back(prev_y += y);
  }

  if (flags.size() != abs_x.size() || abs_x.size() != abs_y.size()) {
    LOG(FATAL) << "Invalid point numbers.";
  }

  int pt = 0;
  for (size_t c = 0; c < num_of_contours; ++c) {
    data->contours.push_back(Contour());
    Contour& contour = data->contours.back();
    uint16_t end_pt_of_contour = readU16(glyph, kSimpleGlyfOffset + c * 2);
    for (; pt <= end_pt_of_contour; ++pt) {
      contour.points.push_back(GlyphPoint(abs_x[pt], abs_y[pt], flags[pt] & 0x01));
    }
  }
  return std::unique_ptr<GlyfData>(data.release());
}
