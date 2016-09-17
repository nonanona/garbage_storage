#include <goocanvas.h>

#include <glog/logging.h>
#include <memory>

#include "truetype.h"
#include "cmap.h"
#include "loca.h"
#include "glyf.h"
#include "image.h"

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

  GtkWidget* window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), w, h);
  gtk_widget_show (window);

  GtkWidget* canvas = goo_canvas_new ();
  gtk_widget_set_size_request (canvas, w, h);
  goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, w, h);
  gtk_widget_show (canvas);
  gtk_container_add (GTK_CONTAINER (window), canvas);

  GooCanvasItem* root = goo_canvas_get_root_item (GOO_CANVAS (canvas));

  for (size_t i = 0; i < simpleGlyph->contours.size(); ++i) {
    std::vector<GlyphPoint> points;
    for (size_t j = 0; j < simpleGlyph->contours[i].points.size(); ++j) {
      GlyphPoint* cur = &simpleGlyph->contours[i].points[j];

      size_t prev_idx = j == 0 ? simpleGlyph->contours[i].points.size() - 1 : j - 1;
      GlyphPoint* prev = &simpleGlyph->contours[i].points[prev_idx];

      if (!prev->on_curve && !cur->on_curve) {
        points.push_back(GlyphPoint((cur->x + prev->x) / 2, (cur->y + prev->y) / 2, true));
        goo_canvas_ellipse_new(
            root,
            (cur->x + prev->x) / 2 + cx, h - ((cur->y + prev->y) / 2 + cy),
            10, 10,
            "stroke-color", "green",
            "line-width", 5.0, NULL);
      }
      points.push_back(*cur);

      goo_canvas_ellipse_new(root, cur->x + cx, h - (cur->y + cy),
                             10, 10,
                             "stroke-color", cur->on_curve ? "red" : "blue",
                             "line-width", 5.0, NULL);
    }


    std::stringstream ss;
    for (size_t j = 0; j < points.size(); ++j) {
      GlyphPoint* cur = &points[j];

      size_t prev_idx = j == 0 ? points.size() - 1 : j - 1;
      GlyphPoint* prev = &points[prev_idx];

      if (cur->on_curve) {
        if (j == 0)
          ss << "M " << cur->x + cx << " " << h - (cur->y + cy) << " ";
        else if (prev->on_curve)
          ss << "L " << cur->x + cx << " " << h - (cur->y + cy) << " ";
        else
          ss << "Q " << prev->x + cx << " " << h - (prev->y + cy)
              << " " << cur->x + cx << " " << h - (cur->y + cy) << " ";
        if (j == points.size() - 1)
          ss << "Z";
      } else {
        if (!prev->on_curve)
          LOG(FATAL) << "Should not happen";
        if (j == points.size() - 1) {
          GlyphPoint* next = &points[0];
          ss << "Q " << cur->x + cx << " " << h - (cur->y + cy) << " "
              << next->x + cx << " " << h - (next->y + cy) << " ";
        }
      }
    }
    goo_canvas_path_new(root, ss.str().c_str(), "stroke-color", "blue", NULL);
    LOG(ERROR) << ss.str();
  }
  gtk_main ();

  return 0;
}
