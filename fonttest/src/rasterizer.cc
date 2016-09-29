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
    const std::vector<Contour>& resolved,int ix, int iy, Gui* gui,
    bool debug_out) {
  int cross_count = 0;

  int c_grid_x = glyph.x_min + ix * grid_size_ + grid_size_ / 2;
  int c_grid_y = glyph.y_min + iy * grid_size_ + grid_size_ / 2;

  double scan_line_y = c_grid_y + 0.5;

  if (debug_out) {
    std::vector<GlyphPoint> points;
    points.push_back(GlyphPoint(c_grid_x, scan_line_y, true));
    points.push_back(GlyphPoint(glyph.x_max + grid_size_, scan_line_y, true));
    gui->fillRect(glyph.x_min +ix * grid_size_,
                  glyph.y_min +(iy + 1) * grid_size_,
                  grid_size_,
                  grid_size_,
                  "magenta");
    gui->drawPath(points, false, "magenta", false, 3.0);
  }

  for (size_t i = 0; i < resolved.size(); ++i) {
    std::vector<GlyphPoint> points = flattenPoints(resolved[i].points);

    for (size_t j = 0; j < points.size(); ++j) {
      GlyphPoint* prev = &points[j == 0 ? points.size() - 1 : j - 1];
      GlyphPoint* cur = &points[j];
      GlyphPoint* next = &points[j == points.size() - 1 ? 0 : j + 1];

      if (cur->on_curve) {
        if (prev->on_curve) {
          // Line
          if ((prev->x == c_grid_x && prev->y == c_grid_y) ||
              (cur->x == c_grid_x && cur->y == c_grid_y)) {
            // The line starts from the control point.
            return true;
          }

          if (prev->y != cur->y) {
            double t = (double)(scan_line_y - prev->y) / (double)(cur->y - prev->y);
            double x = cur->x * t + prev->x * (1.0 - t);
            if (x < c_grid_x)
              continue;
            if (0 < t && t <= 1.0) {
              int sign = cur->y > prev->y ? -1 : 1;
              cross_count += sign;
              if (debug_out) {
                gui->drawPath(std::vector<GlyphPoint>({*prev, *cur}), false,
                    "orange", false, 3.0);
                gui->drawPoint(prev->x, prev->y, 4.0, "red");
                gui->drawPoint(cur->x, cur->y, 4.0, "blue");
                gui->drawPoint(x, scan_line_y, 2.0, sign > 0 ? "red" : "blue");
              }
            }
          } else {
            // Parallel to X-axis
            // Never crosses with the scan line.
          }
        } else {
          // Skip already checked.
        }
      } else {
        // Curve
        if (!prev->on_curve || !next->on_curve)
          LOG(FATAL) << "Must not happen";

        if ((prev->x == c_grid_x && prev->y == c_grid_y) ||
            (next->x == c_grid_x && next->y == c_grid_y)) {
          // The line starts from the control point.
          return true;
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
        int D = by * by - 4 * ay * (cy - scan_line_y);
        if (D < 0)
          continue;

        double t0;
        double t1;
        if (ay != 0) {
          t0 = (- by + sqrt(D) ) / (2.0 * ay);
          t1 = (- by - sqrt(D) ) / (2.0 * ay);
        } else if (by != 0) {
          t0 = (double)(scan_line_y - cy) / (double)by;
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

        if (cross_x0 > c_grid_x &&  0 < t0 && t0 <= 1.0) {
          // drivertive
          // x'(t) = 2 * ax * t + bx
          // y'(t) = 2 * ay * t + by
          double dx_dt = 2.0 * ax * t0 + bx;
          double dy_dt = 2.0 * ay * t0 + by;
          int sign = dy_dt > 0 ? -1 : 1;
          cross_count += sign;
          if (debug_out) {
            gui->drawPath(std::vector<GlyphPoint>({*prev, *cur, *next}), false,
                "orange", false, 3.0);
            gui->drawPoint(prev->x, prev->y, 4.0, "red");
            gui->drawPoint(next->x, next->y, 4.0, "blue");
            gui->drawPoint(cross_x0, scan_line_y, 2.0, sign > 0 ? "red" : "blue");
          }
        }

        if (cross_x1 > c_grid_x && 0 < t1 && t1 <= 1.0) {
          double dx_dt = 2.0 * ax * t1 + bx;
          double dy_dt = 2.0 * ay * t1 + by;
          int sign = dy_dt > 0 ? -1 : 1;
          cross_count += sign;
          if (debug_out) {
            gui->drawPath(std::vector<GlyphPoint>({*prev, *cur, *next}), false,
                "orange", false, 3.0);
            gui->drawPoint(prev->x, prev->y, 4.0, "red");
            gui->drawPoint(next->x, next->y, 4.0, "blue");
            gui->drawPoint(cross_x1, scan_line_y, 2.0, sign > 0 ? "red" : "blue");
          }
        }
      }
    }
  }
  return cross_count == 1;
}

bool Rasterizer::isBitOnByRule2a(const SimpleGlyphData& glyph,
    const std::vector<Contour>& resolved,int ix, int iy, Gui* gui,
    bool debug_out) {
  int cross_count = 0;

  int c_grid_x = glyph.x_min + ix * grid_size_ + grid_size_ / 2;
  int c_grid_y = glyph.y_min + iy * grid_size_ + grid_size_ / 2;

  double scan_line_y = c_grid_y + 0.5;

  if (debug_out) {
    std::vector<GlyphPoint> points;
    points.push_back(GlyphPoint(c_grid_x, scan_line_y, true));
    points.push_back(GlyphPoint(c_grid_x + grid_size_, scan_line_y, true));
    gui->drawPath(points, false, "magenta", false, 3.0);
  }
  for (size_t i = 0; i < resolved.size(); ++i) {
    std::vector<GlyphPoint> points = flattenPoints(resolved[i].points);

    for (size_t j = 0; j < points.size(); ++j) {
      GlyphPoint* prev = &points[j == 0 ? points.size() - 1 : j - 1];
      GlyphPoint* cur = &points[j];
      GlyphPoint* next = &points[j == points.size() - 1 ? 0 : j + 1];

      if (cur->on_curve) {
        if (prev->on_curve) {
          // Line
          if (prev->y != cur->y) {
            double t = (double)(scan_line_y - prev->y) / (double)(cur->y - prev->y);
            double x = cur->x * t + prev->x * (1.0 - t);
            if (c_grid_x < x && x < c_grid_x + grid_size_ && 0 < t && t <= 1.0) {
              cross_count ++;
              if (debug_out) {
                gui->drawPath(std::vector<GlyphPoint>({*prev, *cur}), false,
                    "orange", false, 3.0);
                gui->drawPoint(x, scan_line_y, 2.0, "red");
              }
            }
          } else {
            // Parallel to X-axis
            // Never crosses with the scan line.
          }
        } else {
          // Skip already checked.
        }
      } else {
        // Curve
        if (!prev->on_curve || !next->on_curve)
          LOG(FATAL) << "Must not happen";

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
        int D = by * by - 4 * ay * (cy - scan_line_y);
        if (D < 0)
          continue;

        double t0;
        double t1;
        if (ay != 0) {
          t0 = (- by + sqrt(D) ) / (2.0 * ay);
          t1 = (- by - sqrt(D) ) / (2.0 * ay);
        } else if (by != 0) {
          t0 = (double)(scan_line_y - cy) / (double)by;
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

        if (c_grid_x < cross_x0 && cross_x0 < c_grid_x + grid_size_ &&  0 < t0 && t0 <= 1.0) {
          cross_count ++;
          if (debug_out) {
            gui->drawPath(std::vector<GlyphPoint>({*prev, *cur, *next}), false,
                "orange", false, 3.0);
            gui->drawPoint(cross_x0, scan_line_y, 2.0, "red");
          }
        }

        if (c_grid_x < cross_x1 && cross_x1 < c_grid_x + grid_size_ && 0 < t1 && t1 <= 1.0) {
          cross_count ++;
          if (debug_out) {
            gui->drawPath(std::vector<GlyphPoint>({*prev, *cur, *next}), false,
                "orange", false, 3.0);
            gui->drawPoint(cross_x1, scan_line_y, 2.0, "red");
          }
        }
      }
    }
  }
  return cross_count == 2;
}
bool Rasterizer::isBitOnByRule2b(const SimpleGlyphData& glyph,
    const std::vector<Contour>& resolved,int ix, int iy, Gui* gui,
    bool debug_out) {
  int cross_count = 0;

  int c_grid_x = glyph.x_min + ix * grid_size_ + grid_size_ / 2;
  int c_grid_y = glyph.y_min + iy * grid_size_ + grid_size_ / 2;

  double scan_line_x = c_grid_x + 0.5;

  if (debug_out) {
    std::vector<GlyphPoint> points;
    points.push_back(GlyphPoint(scan_line_x, c_grid_y, true));
    points.push_back(GlyphPoint(scan_line_x, c_grid_y + grid_size_, true));
    gui->drawPath(points, false, "magenta", false, 3.0);
  }
  for (size_t i = 0; i < resolved.size(); ++i) {
    std::vector<GlyphPoint> points = flattenPoints(resolved[i].points);

    for (size_t j = 0; j < points.size(); ++j) {
      GlyphPoint* prev = &points[j == 0 ? points.size() - 1 : j - 1];
      GlyphPoint* cur = &points[j];
      GlyphPoint* next = &points[j == points.size() - 1 ? 0 : j + 1];

      if (cur->on_curve) {
        if (prev->on_curve) {
          // Line
          if (prev->x != cur->x) {
            double t = (double)(scan_line_x - prev->x) / (double)(cur->x - prev->x);
            double y = cur->y * t + prev->y * (1.0 - t);
            if (c_grid_y < y && y < c_grid_y + grid_size_ && 0 < t && t <= 1.0) {
              cross_count ++;
              if (debug_out) {
                gui->drawPath(std::vector<GlyphPoint>({*prev, *cur}), false,
                    "orange", false, 3.0);
                gui->drawPoint(scan_line_x, y, 2.0, "red");
              }
            }
          } else {
            // Parallel to Y-axis
            // Never crosses with the scan line.
          }
        } else {
          // Skip already checked.
        }
      } else {
        // Curve
        if (!prev->on_curve || !next->on_curve)
          LOG(FATAL) << "Must not happen";

        // Curve (x0, y0) - (x1, y1) - (x2, y2)
        // (x1, y1) is the control point.
        // y(t) = ay t**2 + by * t + cy
        // Here,
        // ay = y2 - y1 * 2 + y0
        // by = 2 * y1 - 2 * y0
        // cy = y0
        int ax = next->x - 2 * cur->x + prev->x;
        int bx = 2 * cur->x - 2 * prev->x;
        int cx = prev->x;
        // Solve, y(t) == c_grid_y
        int D = bx * bx - 4 * ax * (cx - scan_line_x);
        if (D < 0)
          continue;

        double t0;
        double t1;
        if (ax != 0) {
          t0 = (- bx + sqrt(D) ) / (2.0 * ax);
          t1 = (- bx - sqrt(D) ) / (2.0 * ax);
        } else if (bx != 0) {
          t0 = (double)(scan_line_x - cx) / (double)bx;
          t1 = 1e+100;  // invalid value
        } else {
          LOG(FATAL) << "!?!?!?!?!?";
        }

        // x(t) =  ax t**2 + bx * t + cx
        // ax = x2 - 2 * x1 + x0
        // bx = 2 * x1 - 2 * x0
        // cx = x0
        double ay = next->y - 2 * cur->y + prev->y;
        double by = 2 * cur->y - 2 * prev->y;
        double cy = prev->y;

        double cross_y0 = ay * t0 * t0 + by * t0 + cy;
        double cross_y1 = ay * t1 * t1 + by * t1 + cy;

        if (c_grid_y < cross_y0 && cross_y0 < c_grid_y + grid_size_ &&  0 < t0 && t0 <= 1.0) {
          cross_count ++;
          if (debug_out) {
            gui->drawPath(std::vector<GlyphPoint>({*prev, *cur, *next}), false,
                "orange", false, 3.0);
            gui->drawPoint(scan_line_x, cross_y0, 2.0, "red");
          }
        }

        if (c_grid_y < cross_y1 && cross_y1 < c_grid_y + grid_size_ && 0 < t1 && t1 <= 1.0) {
          cross_count ++;
          if (debug_out) {
            gui->drawPath(std::vector<GlyphPoint>({*prev, *cur, *next}), false,
                "orange", false, 3.0);
            gui->drawPoint(scan_line_x, cross_y1, 2.0, "red");
          }
        }
      }
    }
  }
  return cross_count == 2;
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
      bool isOn = isBitOn(glyph, resolved, ix, iy, gui,false);
      if (!isOn)
        isOn = isBitOnByRule2a(glyph, resolved, ix, iy, gui, false);
      if (!isOn)
        isOn = isBitOnByRule2b(glyph, resolved, ix, iy, gui, false);
      (*out)[iy * x_grid_num + ix] = isOn;
    }
  }
}

