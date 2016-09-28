#include "gui.h"

#include "glyph_utils.h"

#include <goocanvas.h>
#include <glog/logging.h>
#include <sstream>

Gui::Gui(int w, int h, int cx, int cy, float scale, int margin)
      : w_(w * scale + margin * 2), h_(h * scale + margin * 2),
      cx_(cx * scale), cy_(cy * scale),
      scale_(scale), margin_(margin) {
  GtkWidget* window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), w_, h_);
  gtk_widget_show (window);

  GtkWidget* canvas = goo_canvas_new ();
  gtk_widget_set_size_request (canvas, w_, h_);
  goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, w_, h_);
  gtk_widget_show (canvas);
  gtk_container_add (GTK_CONTAINER (window), canvas);

  root_ = goo_canvas_get_root_item (GOO_CANVAS (canvas));
}

void Gui::drawPath(const std::vector<Contour>& contours, bool with_contours,
    const std::string& color, bool close_path) {
  for (size_t i = 0; i < contours.size(); ++i) {
    std::vector<GlyphPoint> points = flattenPoints(contours[i].points);
    drawPath(points, with_contours, color, close_path);
  }
}

void Gui::drawPath(const std::vector<GlyphPoint>& points, bool with_contours,
    const std::string& color, bool close_path) {
  std::stringstream ss;
  for (size_t j = 0; j < points.size(); ++j) {
    const GlyphPoint* cur = &points[j];

    if (with_contours && !cur->interpolated)
      drawPoint(cur->x, cur->y, 2.0, cur->on_curve ? "red" : "blue");

    size_t prev_idx = j == 0 ? points.size() - 1 : j - 1;
    const GlyphPoint* prev = &points[prev_idx];
    if (cur->on_curve) {
      if (j == 0)
        ss << "M " << toX(cur->x) << " " << toY(cur->y) << " ";
      else if (prev->on_curve)
        ss << "L " << toX(cur->x) << " " << toY(cur->y) << " ";
      else
        ss << "Q " << toX(prev->x) << " " << toY(prev->y)
          << " " << toX(cur->x) << " " << toY(cur->y) << " ";
      if (close_path && j == points.size() - 1)
        ss << "Z";
    } else {

      if (!prev->on_curve)
        LOG(FATAL) << "Should not happen";
      if (j == points.size() - 1) {
        const GlyphPoint* next = &points[0];
        ss << "Q " << toX(cur->x) << " " << toY(cur->y) << " "
          << toX(next->x) << " " << toY(next->y) << " ";
      }
    }
  }
  drawPath(ss.str(), color);
}

void Gui::drawPath(const std::string& command, const std::string& color) {
  goo_canvas_path_new(root_, command.c_str(), "stroke-color", color.c_str(), NULL);
}

void Gui::drawPoint(int x, int y, float width, const std::string& color) {
  goo_canvas_ellipse_new(root_, toX(x), toY(y), 5, 5, "stroke-color", color.c_str(),
      "fill-color", color.c_str(), "line-width", width, NULL);
}

void Gui::drawLine(int from_x, int from_y, int to_x, int to_y,
    const std::string& color) {
  std::stringstream ss;
  ss << "M " << toX(from_x) << " " << toY(from_y) << " ";
  ss << "L " << toX(to_x) << " " << toY(to_y) << " ";
  goo_canvas_path_new(root_, ss.str().c_str(), "stroke-color", color.c_str(),
      NULL);
}

void Gui::fillRect(int x, int y, int w, int h, const std::string& color) {
  goo_canvas_rect_new(root_, toX(x), toY(y), w * scale_, h * scale_, "fill-color", color.c_str(), NULL);
}
