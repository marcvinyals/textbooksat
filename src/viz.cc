#include "viz.h"

#include <graphviz/gvc.h>

using namespace std;
using namespace cimg_library;

graphviz_viz::graphviz_viz() :
  tmpfile(tmpnam(nullptr)) {
  gvc = gvContext();
}

graphviz_viz::~graphviz_viz() {
  gvFreeLayout(gvc, g);
  agclose(g);
  gvFreeContext(gvc);
  unlink(tmpfile.c_str());
}

void graphviz_viz::render() {
  gvRenderFilename (gvc, g, "bmp", tmpfile.c_str());
  CImg<unsigned char> image(tmpfile.c_str());
  display.display(image);
  usleep(100000);
}
