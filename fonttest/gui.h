#pragma once

#include <goocanvas.h>
#include <string>
#include "glyf.h"

class Gui {
 public:
  Gui(int w, int h) : Gui(w, h, 0, 0) {}
  Gui(int w, int h, int cx, int cy) : Gui(w, h, cx, cy, 1.0, 0) {}
  Gui(int w, int h, int cx, int cy, float scale, int margin);

  void drawPath(const std::string& command, const std::string& color);
  void drawPath(const std::vector<Contour>& contours, bool with_contours,
      const std::string& color, bool close_path);
  void drawPath(const std::vector<GlyphPoint>& contours, bool with_contours,
      const std::string& color, bool close_path);
  void drawPoint(int x, int y, float width, const std::string& color);

  void drawLine(int from_x, int from_y, int to_x, int to_y, const std::string& color);

  std::vector<GlyphPoint> flattenPoints(const std::vector<GlyphPoint>& points);

 private:
  int w_;
  int h_;
  int cx_;
  int cy_;
  int margin_;
  float scale_;

  int toX(int x) { return scale_ * x + cx_ + margin_; }
  int toY(int y) { return h_ - scale_ * y - cy_ - margin_; }

  GooCanvasItem* root_;
};
