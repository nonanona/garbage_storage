#include "rasterizer.h"

#include <math.h>
#include "glyf.h"
#include "cvt.h"
#include "fpgm.h"
#include "glog/logging.h"
#include "glyph_utils.h"
#include "instructions.h"
#include "prep.h"

#include "gui.h"

bool Rasterizer::isBitOn(const SimpleGlyphData& glyph,
    const std::vector<Contour>& resolved,int ix, int iy) {
  int cross_count = 0;

  int c_grid_x = glyph.x_min + ix * grid_size_ + grid_size_ / 2;
  int c_grid_y = glyph.y_min + iy * grid_size_ + grid_size_ / 2;

  for (size_t i = 0; i < resolved.size(); ++i) {
    std::vector<GlyphPoint> points = flattenPoints(resolved[i].points);

    for (size_t j = 0; j < points.size(); ++j) {
      GlyphPoint* prev = &points[j == 0 ? points.size() - 1 : j - 1];
      GlyphPoint* cur = &points[j];
      GlyphPoint* next = &points[j == points.size() - 1 ? 0 : j + 1];

      // move grid center to avoid on the contours.
      while (prev->y == c_grid_y || next->y == c_grid_y)
        c_grid_y++;

      if (cur->on_curve) {
        if (prev->on_curve) {
          // Line
          if (prev->y == cur->y)
            continue;
          double t = (double)(c_grid_y - cur->y) / (double)(prev->y - cur->y);
          double x = prev->x * t + cur->x * (1.0 - t);
          if (x < c_grid_x)
            continue;
          if (0 < t && t < 1.0) {
            int dir = (prev->x - cur->x) * (prev->y - cur->y);
            if (dir > 0)
              cross_count++;
            else if (dir < 0)
              cross_count--;
            else if (prev->x == cur->x)  // dir == 0
              cross_count += (prev->y > cur->y) ? 1 : -1;
            else
              cross_count += (prev->x > cur->x) ? 1 : -1;
          }
        } else {
          // Skip already checked.
        }
      } else {
        if (!prev->on_curve || !next->on_curve)
          LOG(FATAL) << "Must not happen";

        // On the previous on-curve point, treat as crossing.
        if (prev->x > c_grid_x && prev->y == c_grid_y) {
          cross_count++;
          continue;
        }

        // Curve (x0, y0) - (x1, y1) - (x2, y2)
        // (x1, y1) is the control point.
        // y(t) = ay t**2 + by * t + cy
        // Here,
        // ay = y2 - y1 * 2 + y0
        // by = 2 * y1 - 2 * y0
        // cy = y0
        int ay = next->y - 2 * cur->y + prev->y;
        int by = 2 * cur->y - 2 * prev->y;
        int cy = prev->y;
        // Solve, y(t) == c_grid_y
        int D = by * by - 4 * ay * (cy - c_grid_y);
        if (D < 0)
          continue;

        double t0;
        double t1;
        if (ay != 0) {
          t0 = (- by + sqrt(D) ) / (2.0 * ay);
          t1 = (- by - sqrt(D) ) / (2.0 * ay);
        } else if (by != 0) {
          t0 = (double)(c_grid_y - cy) / (double)by;
          t1 = 1e+100;  // invalid value
        } else {
          LOG(FATAL) << "!?!?!?!?!?";
        }

        // x(t) =  ax t**2 + bx * t + cx
        // ax = x2 - 2 * x1 + x0
        // bx = 2 * x1 - 2 * x0
        // cx = x0
        double ax = next->x - 2 * cur->x + prev->x;
        double bx = 2 * cur->x - 2 * prev->x;
        double cx = prev->x;

        double cross_x0 = ax * t0 * t0 + bx * t0 + cx;
        double cross_x1 = ax * t1 * t1 + bx * t1 + cx;

        if (cross_x0 > c_grid_x &&  0 < t0 && t0 < 1.0) {
          // drivertive
          // x'(t) = 2 * ax * t + bx
          // y'(t) = 2 * ay * t + by
          double dx_dt = 2.0 * ax * t0 + bx;
          double dy_dt = 2.0 * ay * t0 + by;
          if (dy_dt == 0)
            cross_count += dx_dt > 0 ? 1 : -1;
          else if (dx_dt == 0)
            cross_count += dy_dt > 0 ? 1 : -1;
          else
            cross_count += (dy_dt * dx_dt) > 0 ? 1 : -1;
        }

        if (cross_x1 > c_grid_x && 0 < t1 && t1 < 1.0) {
          double dx_dt = 2.0 * ax * t1 + bx;
          double dy_dt = 2.0 * ay * t1 + by;
          if (dy_dt == 0)
            cross_count += dx_dt > 0 ? 1 : -1;
          else if (dx_dt == 0)
            cross_count += dy_dt > 0 ? 1 : -1;
          else
            cross_count += (dy_dt * dx_dt) > 0 ? 1 : -1;
        }
      }
    }
  }
  return cross_count % 2 != 0;
}


void Rasterizer::rasterize(const SimpleGlyphData& glyph,
                           std::vector<char>* out,
                           int* x_pixel_num,
                           Gui* gui) {
  int width = glyph.x_max - glyph.x_min;
  int height = glyph.y_max - glyph.y_min;
  int x_grid_num = width / grid_size_ + 1;
  int y_grid_num = height / grid_size_ + 1;

  *x_pixel_num = x_grid_num;
  out->resize(x_grid_num * y_grid_num);

  // Hinting here.
  std::vector<Contour> resolved =
    HintStackMachine::execute(glyph, grid_size_, tt_);

  for (int ix = 0; ix < x_grid_num; ++ix) {
    for (int iy = 0; iy < y_grid_num; ++iy) {
      (*out)[iy * x_grid_num + ix] = isBitOn(glyph, resolved, ix, iy);
    }
  }
}

