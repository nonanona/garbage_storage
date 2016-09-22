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
#include "cvt.h"
#include "rasterizer.h"

int main (int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gtk_init (&argc, &argv);

  TrueType ttf(argv[1]);
  uint32_t c = argv[2][0];
  int px = argc == 4 ? atoi(argv[3]) : 12;


  std::unique_ptr<CmapSubTable> cmap(ttf.getCmap());
  std::unique_ptr<LocaSubTable> loca(ttf.getLoca());
  std::unique_ptr<GlyfSubTable> glyf(ttf.getGlyf());
  std::unique_ptr<CvtSubTable> cvt(ttf.getCvt());

  uint32_t glyphId = cmap->findGlyphId(c, 0);
  std::unique_ptr<SimpleGlyphData> simpleGlyph(
      static_cast<SimpleGlyphData*>(
          glyf->getGlyfData(loca->findGlyfOffset(glyphId)).release()));

  std::unique_ptr<HeadSubTable> head(ttf.getHead());

  uint32_t cx = -simpleGlyph->x_min;
  uint32_t cy = -simpleGlyph->y_min;
  uint32_t w = simpleGlyph->x_max - simpleGlyph->x_min;
  uint32_t h = simpleGlyph->y_max - simpleGlyph->y_min;
  Gui gui(w, h, cx, cy, 0.3, 100);

  Rasterizer rasterizer(px, head->unit_per_em(), cvt.get());

  int x_grid_num;
  std::vector<char> pixels;

  rasterizer.rasterize(*simpleGlyph.get(), &pixels, &x_grid_num);

  int y_grid_num = pixels.size() / x_grid_num;
  int grid = rasterizer.grid_size();

  for (int i = 0; i < x_grid_num + 1; ++i) {
    gui.drawLine(
        simpleGlyph->x_min + i * grid,
        simpleGlyph->y_min,
        simpleGlyph->x_min + i * grid,
        simpleGlyph->y_min + y_grid_num * grid, 
        "gray");
  }

  for (int j = 0; j < y_grid_num + 1; ++j) {
    gui.drawLine(
        simpleGlyph->x_min,
        simpleGlyph->y_min + j * grid,
        simpleGlyph->x_min + x_grid_num * grid,
        simpleGlyph->y_min + j * grid,
        "gray");
  }

  for (int ix = 0; ix < x_grid_num; ++ix) {
    for (int iy = 0; iy < y_grid_num; ++iy) {
      if (pixels[iy * x_grid_num + ix] == 0)
        continue;
      gui.fillRect(
          simpleGlyph->x_min + ix * grid,
          simpleGlyph->y_min + (iy + 1) * grid,
          grid,
          grid,
          "black");
    }
  }

  gui.drawPath(simpleGlyph->contours, false, "blue", true);

  gtk_main();

  return 0;
}
