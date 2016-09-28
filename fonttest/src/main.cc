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
#include "prep.h"
#include "fpgm.h"
#include "rasterizer.h"

int main (int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gtk_init (&argc, &argv);

  std::string fname = argv[1];
  std::string last_four = fname.substr(fname.size() - 4);
  int index = 0;
  int char_arg_index = 2;
  if (last_four == ".ttc") {
    // font collection
    index = atoi(argv[2]);
    char_arg_index = 3;
  } else if (last_four != ".ttf") {
    LOG(ERROR) << "Not supported file: " << fname;
  }

  TrueType ttf(fname, index);
  int px = argc == (char_arg_index + 2) ? atoi(argv[char_arg_index + 1]) : 12;

  uint32_t ucs4 = 0;
  uint8_t leading = argv[char_arg_index][0];
  size_t utf8_len;
  if (leading < 0x80) {
    utf8_len = 1;
    ucs4 = leading;
  } else if ((leading & 0xe0) == 0xc0) {
    utf8_len = 2;
    ucs4 = leading & 0x1f;
  } else if ((leading & 0xf0) == 0xe0) {
    utf8_len = 3;
    ucs4 = leading & 0x0f;
  } else if ((leading & 0xf8) == 0xf0) {
    utf8_len = 4;
    ucs4 = leading & 0x07;
  }

  for (int i = 1; i < utf8_len; ++i) {
    ucs4 <<= 6;
    ucs4 += argv[char_arg_index][i] & 0x3f;
  }

  std::unique_ptr<CmapSubTable> cmap(ttf.getCmap());
  uint32_t glyphId = cmap->findGlyphId(ucs4, 0);
  LOG(ERROR) << std::hex << "GlyphId[U+" << ucs4 << "] = 0x" << glyphId;

  std::unique_ptr<LocaSubTable> loca(ttf.getLoca());
  std::unique_ptr<GlyfSubTable> glyf(ttf.getGlyf());
  LOG(ERROR) << "Offset : " << loca->findGlyfOffset(glyphId);
  std::unique_ptr<SimpleGlyphData> simpleGlyph(
      static_cast<SimpleGlyphData*>(
          glyf->getGlyfData(loca->findGlyfOffset(glyphId), loca.get()).release()));

  std::unique_ptr<HeadSubTable> head(ttf.getHead());

  uint32_t cx = -simpleGlyph->x_min;
  uint32_t cy = -simpleGlyph->y_min;
  uint32_t w = simpleGlyph->x_max - simpleGlyph->x_min;
  uint32_t h = simpleGlyph->y_max - simpleGlyph->y_min;
  Gui gui(w, h, cx, cy, 0.3, 100);

  Rasterizer rasterizer(px, head->unit_per_em(), ttf);

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

  gui.drawPath(simpleGlyph->contours, true, "blue", true);

  gtk_main();

  return 0;
}
