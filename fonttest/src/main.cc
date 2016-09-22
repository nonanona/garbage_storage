#include <goocanvas.h>

#include <glog/logging.h>
#include <memory>
#include <math.h>

#include "truetype.h"
#include "cmap.h"
#include "loca.h"
#include "glyf.h"
#include "image.h"
#include "gui.h"
#include "head.h"

int main (int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  gtk_init (&argc, &argv);

  TrueType ttf("DroidSans.ttf");
  std::unique_ptr<CmapSubTable> cmap(ttf.getCmap());

  uint32_t c = argv[1][0];

  uint32_t glyphId = cmap->findGlyphId(c, 0);
  LOG(ERROR) << "Glyph [" << (char)c << "] = " << glyphId;
  std::unique_ptr<LocaSubTable> loca(ttf.getLoca());

  std::unique_ptr<GlyfSubTable> glyf(ttf.getGlyf());
  LOG(ERROR) << "0x" << std::hex << loca->findGlyfOffset(glyphId);
  std::unique_ptr<SimpleGlyphData> simpleGlyph(
      static_cast<SimpleGlyphData*>(
          glyf->getGlyfData(loca->findGlyfOffset(glyphId)).release()));

  std::unique_ptr<HeadSubTable> head(ttf.getHead());

  uint32_t cx = -simpleGlyph->x_min;
  uint32_t cy = -simpleGlyph->y_min;
  uint32_t w = simpleGlyph->x_max - simpleGlyph->x_min;
  uint32_t h = simpleGlyph->y_max - simpleGlyph->y_min;

  Image image(w, h, cx, cy);

  for (size_t i = 0; i < simpleGlyph->contours.size(); ++i) {
    GlyphPoint* from_pt = &simpleGlyph->contours[i].points[
        simpleGlyph->contours[i].points.size() - 1];
    for (size_t j = 0; j < simpleGlyph->contours[i].points.size(); ++j) {
        GlyphPoint* to_pt = &simpleGlyph->contours[i].points[j];
        image.drawLine(from_pt->x, from_pt->y, to_pt->x, to_pt->y);

        from_pt = to_pt;
    }
  }

  image.saveToBMP("test.bmp");

  Gui gui(w, h, cx, cy, 0.3, 100);
  gui.drawPath(simpleGlyph->contours, false, "blue", true);

  const int pixels = 24;
  int grid = head->unit_per_em() / pixels;

  int x_grid_num = w / grid + 1;
  int y_grid_num = h / grid + 1;


  for (int i = 0; i < x_grid_num + 1; ++i) {
    gui.drawLine(
        simpleGlyph->x_min + i * grid,
        simpleGlyph->y_min,
        simpleGlyph->x_min + i * grid,
        simpleGlyph->y_min + y_grid_num * grid, 
        "green");
  }

  for (int j = 0; j < y_grid_num + 1; ++j) {
    gui.drawLine(
        simpleGlyph->x_min,
        simpleGlyph->y_min + j * grid,
        simpleGlyph->x_min + x_grid_num * grid,
        simpleGlyph->y_min + j * grid,
        "green");
  }


  for (int ix = 0; ix < x_grid_num; ++ix) {
    for (int iy = 0; iy < y_grid_num; ++iy) {
      int cross_count = 0;

      int c_grid_x = simpleGlyph->x_min + ix * grid + grid / 2;
      int c_grid_y = simpleGlyph->y_min + iy * grid + grid / 2;

      bool draw_on = (ix == 2 && iy == 9);

      for (size_t i = 0; i < simpleGlyph->contours.size(); ++i) {
        std::vector<GlyphPoint> points = gui.flattenPoints(
            simpleGlyph->contours[i].points);

        for (size_t j = 0; j < points.size(); ++j) {
          GlyphPoint* prev = &points[j == 0 ? points.size() - 1 : j - 1];
          GlyphPoint* cur = &points[j];
          GlyphPoint* next = &points[j == points.size() - 1 ? 0 : j + 1];

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
                if (draw_on) {
                gui.drawPoint(
                    prev->x * t + cur->x * (1.0 - t),
                    prev->y * t + cur->y * (1.0 - t),
                    8.0, "orange");
                }
                cross_count++;
              }
            } else {
              // Skip already checked.
            }
          } else {
            bool dump_path;
            if (i == 0 && j == 52) {
              std::vector<GlyphPoint> curve;
              curve.push_back(*prev);
              curve.push_back(*cur);
              curve.push_back(*next);
              gui.drawPath(curve, true, "red", false);
              dump_path = true;
            } else {
              dump_path = false;
            }
            if (!prev->on_curve || !next->on_curve) {
              LOG(FATAL) << "Must not happen";
            }

            if (prev->x > c_grid_x && prev->y == c_grid_y) {
              if (draw_on)
                gui.drawPoint(prev->x, prev->y, 8.0, "orange");
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
            int discriminant = by * by - 4 * ay * (cy - c_grid_y);
            if (discriminant < 0) {
              // Not crossing. go next.
              continue;
            }

            double t0;
            double t1;
            if (ay != 0) {
              t0 = (- by + sqrt(discriminant) ) / (2.0 * ay);
              t1 = (- by - sqrt(discriminant) ) / (2.0 * ay);
            } else if (by != 0) {
              LOG(ERROR) << ay << ", " << by << ", " << cy << ": " << c_grid_y;
              t0 = (double)(c_grid_y - cy) / (double)by;
              t1 = 1e+100;
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

            double cross_y0 = ay * t0 * t0 + by * t0 + cy;
            double cross_y1 = ay * t1 * t1 + by * t1 + cy;

            if (cross_x0 > c_grid_x &&  0 < t0 && t0 < 1.0) {
              if (draw_on)
                gui.drawPoint(cross_x0, cross_y0, 8.0, "orange");
              cross_count++;
            }

            if (cross_x1 > c_grid_x && 0 < t1 && t1 < 1.0) {
              if (draw_on)
                gui.drawPoint(cross_x1, cross_y1, 8.0, "orange");
              cross_count++;
            }

          }
        }
      }

      gui.drawPoint(
          simpleGlyph->x_min + ix * grid + grid / 2,
          simpleGlyph->y_min + iy * grid + grid / 2,
          10.0, cross_count % 2 == 1 ? "red" : "pink");
    }
  }

  gtk_main();

  return 0;
}
