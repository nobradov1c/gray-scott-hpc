#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <math.h>
#include <stdio.h>

double clamp(double x, double a, double b) {
  if (x < a)
    return a;
  if (x > b)
    return b;
  return x;
}

/* Convert HSV (h in [0,1), s in [0,1], v in [0,1]) to RGB bytes */
void hsv_to_rgb_bytes(double h, double s, double v, unsigned char *r,
                      unsigned char *g, unsigned char *b) {
  h = h - floor(h); // wrap
  double hh = h * 6.0;
  int i = (int)hh;
  double f = hh - i;
  double p = v * (1.0 - s);
  double q = v * (1.0 - s * f);
  double t = v * (1.0 - s * (1.0 - f));
  double rf, gf, bf;
  switch (i) {
  case 0:
    rf = v;
    gf = t;
    bf = p;
    break;
  case 1:
    rf = q;
    gf = v;
    bf = p;
    break;
  case 2:
    rf = p;
    gf = v;
    bf = t;
    break;
  case 3:
    rf = p;
    gf = q;
    bf = v;
    break;
  case 4:
    rf = t;
    gf = p;
    bf = v;
    break;
  default:
    rf = v;
    gf = p;
    bf = q;
    break;
  }
  /* convert to bytes with clamping */
  *r = (unsigned char)(clamp(rf, 0.0, 1.0) * 255.0 + 0.5);
  *g = (unsigned char)(clamp(gf, 0.0, 1.0) * 255.0 + 0.5);
  *b = (unsigned char)(clamp(bf, 0.0, 1.0) * 255.0 + 0.5);
}

void write_png(const char *outdir, int step, int n, double *u, double *v) {
  int w = n;
  int h = n;
  unsigned char *img = malloc(3 * w * h);
  if (!img) {
    printf("Couldn't allocate memmory for the image.");
    return;
  }

  const double sat = 0.9;         /* strong saturation for vivid colors */
  const double gamma = 1.0 / 2.2; /* gamma to brighten midtones */
  const double contrast = 1.1;    /* small contrast boost */

  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      int index = i * n + j;
      double uu = u[index];
      double vv = v[index];

      uu = clamp(uu, 0, 1);
      vv = clamp(vv, 0, 1);

      //   map u to hue
      const double hue_base = 0.55; // teal
      const double hue_span = 0.35;
      double hval = hue_base + (uu - 0.5) * hue_span;
      //   hue is periodic [0, 1)
      //   extract only decimals as it's the same color
      hval = hval - floor(hval);

      //   map v to luminance with contrast + gamma
      vv = 0.5 + contrast * (vv - 0.5);
      //   if vv == -1, it will be clamped to 0
      // which is black color
      vv = clamp(vv, 0, 1);
      vv = pow(vv, gamma);

      unsigned char r, g, b;
      hsv_to_rgb_bytes(hval, sat, vv, &r, &g, &b);
      int p = (i * w + j) * 3;
      img[p + 0] = r;
      img[p + 1] = g;
      img[p + 2] = b;
    }
  }

  char name[512];
  snprintf(name, sizeof(name), "%s/%04d.png", outdir, step);
  int result = stbi_write_png(name, w, h, 3, img, w * 3);
  if (result == 0) {
    printf("Writing image failed, result = %i\n", result);
  }
  free(img);
}
