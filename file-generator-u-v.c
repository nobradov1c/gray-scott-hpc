/*
gcc ./file-generator-u-v.c -o file-generator-u-v.out && ./file-generator-u-v.out
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* rng_next using standard rand(); returns [0,1). */
static double rng_next(void) {
  return (double)rand() / ((double)RAND_MAX + 1.0);
}

/* init_uv -- simplified signature keeps original behavior,
   uses hardcoded seed for deterministic random initialization. */
int init_uv(double *u, double *v, int n, double seed_density,
            double seed_strength, unsigned long long rng_seed_unused,
            const char *mode) {
  /* Hardcoded deterministic seed */
  srand(42);

  int i, j, s, cx, cy;

  /* 1. Base state */
  for (i = 0; i < n * n; i++) {
    u[i] = 1.0;
    v[i] = 0.0;
  }

  if (strcmp(mode, "random") == 0) {
    /* 2a. Random seeds */
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
        if (rng_next() < seed_density) {
          v[i * n + j] = seed_strength;
          u[i * n + j] = 1.0 - 0.5 * seed_strength;
        }
      }
    }
  } else if (strcmp(mode, "center") == 0) {
    /* 2b. Center blob */
    s = (6 > n / 20) ? 6 : n / 20;
    cx = n / 2;
    cy = n / 2;
    for (i = cx - s; i < cx + s; i++) {
      for (j = cy - s; j < cy + s; j++) {
        if (i >= 0 && i < n && j >= 0 && j < n) {
          u[i * n + j] = 0.50;
          v[i * n + j] = seed_strength;
        }
      }
    }
  } else {
    return -1; /* unknown mode */
  }

  /* 3. Add small noise to break symmetry */
  for (i = 0; i < n * n; i++) {
    u[i] += 0.02 * (rng_next() - 0.5);
    v[i] += 0.02 * (rng_next() - 0.5);
  }

  /* 4. Clamp to [0, 1] */
  for (i = 0; i < n * n; i++) {
    if (u[i] < 0.0)
      u[i] = 0.0;
    else if (u[i] > 1.0)
      u[i] = 1.0;
    if (v[i] < 0.0)
      v[i] = 0.0;
    else if (v[i] > 1.0)
      v[i] = 1.0;
  }

  return 0;
}

int main() {
  int n = 256;
  double *u = (double *)malloc((size_t)(n * n) * sizeof(double));
  double *v = (double *)malloc((size_t)(n * n) * sizeof(double));
  if (!u || !v) {
    free(u);
    free(v);
    return 1;
  }

  if (init_uv(u, v, n, 0.02, 1.0, 0ULL, "random") != 0) {
    free(u);
    free(v);
    return 1;
  }

  FILE *f_out = fopen("u-v-seed.txt", "w");
  if (f_out == NULL) {
    perror("Error opening file...");
    free(u);
    free(v);
    return 1;
  }

  /* save matrix dim */
  fprintf(f_out, "%d\n", n);

  /* write u field */
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      fprintf(f_out, "%.6f", u[i * n + j]);
      if (j < n - 1)
        fprintf(f_out, " ");
    }
    fprintf(f_out, "\n");
  }

  /* blank separator line */
  fprintf(f_out, "\n");

  /* write v field */
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      fprintf(f_out, "%.6f", v[i * n + j]);
      if (j < n - 1)
        fprintf(f_out, " ");
    }
    fprintf(f_out, "\n");
  }

  fclose(f_out);
  free(u);
  free(v);
  return 0;
}
