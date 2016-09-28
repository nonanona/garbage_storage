#include "glyf.h"

#include "utils.h"
#include "loca.h"
#include <glog/logging.h>

std::unique_ptr<GlyfData> GlyfSubTable::getGlyfData(uint32_t offset, LocaSubTable* loca) const {
  const uint8_t* glyph = ptr_ + offset;
  int16_t num_of_contours = readS16(glyph, 0);
  if (num_of_contours < 0) 
    return std::unique_ptr<GlyfData>(getCompositeGlyfData(offset, loca).release());
  else
    return std::unique_ptr<GlyfData>(getSimpleGlyfData(offset).release());
}

std::unique_ptr<SimpleGlyphData> GlyfSubTable::getCompositeGlyfData(uint32_t offset, LocaSubTable* loca) const {
  std::unique_ptr<SimpleGlyphData> data(new SimpleGlyphData());
  const uint8_t* glyph = ptr_ + offset;
  int16_t num_of_contours = readS16(glyph, 0);
  if (num_of_contours > 0) 
    LOG(FATAL) << "Simple glyph is specified.";
  data->x_min = readS16(glyph, 2);
  data->y_min = readS16(glyph, 4);
  data->x_max = readS16(glyph, 6);
  data->y_max = readS16(glyph, 8);
  const size_t kCompositeGlyfOffset = 10;

  uint16_t flag;
  size_t composition_glyph_offset = kCompositeGlyfOffset;
  do {
    flag = readU16(glyph, composition_glyph_offset );
    LOG(ERROR) << std::hex << "0x" << flag;
    uint16_t glyph_index = readU16(glyph, composition_glyph_offset + 2);
    LOG(ERROR) << std::hex << "Glyph Index: " << glyph_index;

    int16_t arg1, arg2;
    size_t nextOffset;
    if ((flag & 0x01) != 0) {
      arg1 = readS16(glyph, composition_glyph_offset + 4);
      arg2 = readS16(glyph, composition_glyph_offset + 6);
      composition_glyph_offset += 8;
    } else {
      arg1 = (int8_t)glyph[composition_glyph_offset + 4];
      arg2 = (int8_t)glyph[composition_glyph_offset + 5];
      composition_glyph_offset += 6;
    }

    if ((flag & 0x02) == 0) {
      LOG(FATAL) << "Point coordinate is not supported.";
    }

    std::unique_ptr<SimpleGlyphData> compose_glyph = getSimpleGlyfData(
        loca->findGlyfOffset(glyph_index));

    for (int i = 0; i < compose_glyph->contours.size(); ++i) {
      for (int j = 0; j < compose_glyph->contours[i].points.size(); ++j) {
        compose_glyph->contours[i].points[j].x += arg1;
        compose_glyph->contours[i].points[j].y += arg2;
      }
      data->contours.push_back(compose_glyph->contours[i]);
    }

  } while ((flag & (1 << 5u)) != 0);
  return data;
}

std::unique_ptr<SimpleGlyphData> GlyfSubTable::getSimpleGlyfData(uint32_t offset) const {
  const uint8_t* glyph = ptr_ + offset;
  int16_t num_of_contours = readS16(glyph, 0);
  if (num_of_contours < 0) 
    LOG(FATAL) << "Composite glyph is specified.";
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
    LOG(FATAL) << "Invalid count of flags: flags:" << flags.size() << ", total points: " << total_pts;
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
  return data;
}
