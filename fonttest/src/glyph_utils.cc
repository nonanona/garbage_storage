#include "glyph_utils.h"

#include <stddef.h>
#include <stdint.h>
#include "glyf.h"

std::vector<GlyphPoint> flattenPoints(const std::vector<GlyphPoint>& points) {
  std::vector<GlyphPoint> out;
  for (size_t j = 0; j < points.size(); ++j) {
    const GlyphPoint* cur = &points[j];

    size_t prev_idx = j == 0 ? points.size() - 1 : j - 1;
    const GlyphPoint* prev = &points[prev_idx];

    if (!prev->on_curve && !cur->on_curve) {
      out.push_back(
          GlyphPoint((cur->x + prev->x) / 2, (cur->y + prev->y) / 2, true));
    }

    out.push_back(*cur);
  }

  return out;
}
