/*
gcc ./linear.c -o linear.out && ./linear.out plane-bounds.txt u-v-seed.txt
*/
#include "stdio.h"
#include "stdlib.h"
#include <stdio.h>

int main(int argc, char **argv) {
  int n = 0;
  double *matrix = NULL;

  if (argc != 3) {
    printf("Usage: %s <plane-bounds-file> <u-v-seed-file>\n", argv[0]);
  }

  FILE *f_plane_bounds_in = fopen(argv[1], "r");
  if (f_plane_bounds_in == NULL) {
    printf("Error opening file %s...\n", argv[1]);
    perror("error");

    return 0;
  } else {
    /* Expected file format:
       n
       a[0][0] a[0][1] ... a[0][n-1]
       ...
       a[n-1][0] ...  a[n-1][n-1]
    */
    if (fscanf(f_plane_bounds_in, "%d", &n) != 1) {
      printf("Failed reading matrix dim from file.\n");
    }

    matrix = calloc(n * n, sizeof(double));
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        fscanf(f_plane_bounds_in, "%lf", &matrix[i * n + j]);
      }
    }
    fclose(f_plane_bounds_in);
  }

  if (n == 0) {
    printf("Failed parsing plane input file...\n");
    free(matrix);
    return 0;
  }

  int u_v_n = 0;
  double *u;
  double *v;
  FILE *f_u_v_seed_in = fopen(argv[2], "r");
  if (f_u_v_seed_in == NULL) {
    printf("Error opening file %s...\n", argv[2]);
    perror("error");

    free(matrix);
    return 0;
  } else {
    /* Expected file format:
       n
       u[0][0] u[0][1] ... u[0][n-1]
       ...
       u[n-1][0] ...  u[n-1][n-1]

       v[0][0] v[0][1] ... v[0][n-1]
       ...
       v[n-1][0] ...  v[n-1][n-1]
    */
    if (fscanf(f_u_v_seed_in, "%d", &u_v_n) != 1) {
      printf("Failed reading matrix dim from file.\n");
    }

    if (n != u_v_n) {
      printf("Dims not matching %i != %i\n", n, u_v_n);
      free(matrix);
      return 0;
    }

    u = calloc(n * n, sizeof(double));
    v = calloc(n * n, sizeof(double));
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        fscanf(f_u_v_seed_in, "%lf", &u[i * n + j]);
      }
    }

    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        fscanf(f_u_v_seed_in, "%lf", &v[i * n + j]);
      }
    }
    fclose(f_u_v_seed_in);
  }

  // printf("%.6f %.6f %.6f\n", v[0], v[1], v[n * n - 1]);

  free(matrix);
  return 0;
}
