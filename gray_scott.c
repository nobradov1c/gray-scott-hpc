#include "img_util.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define U_V_SEED_FILE "u-v-seed.txt"
#define PARAMS_FILE "params.txt"

int get_index(int n, int i, int j) { return i * n + j; }
void print_matrix(double *mat, int m, int n) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      printf("%.6f ", mat[get_index(n, i, j)]);
    }
    printf("\n");
  }
}

double get_neighbor_walled(double curr, double neighbor) {
  // if neighbor is "-1", we are out of bounds,
  // use value equal to curr for mirror-like behaviour
  // for no spilling out of bounds
  if (neighbor == -1) {
    return curr;
  } else {
    return neighbor;
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <directory with input files>\n", argv[0]);
    return 0;
  }

  int n = 0;
  double du;
  double dv;
  double f;
  double k;
  double dt;
  int steps;
  int snapshot_interval;
  double *u = NULL;
  double *v = NULL;
  char outdir[256] = "";
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  snprintf(outdir, sizeof(outdir), "out_%04d%02d%02d_%02d%02d%02d",
           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
           tm.tm_sec);
  mkdir(outdir, 0755);

  const char *separator = "/";
  char u_v_seed_file[64], params_file[64];
  snprintf(u_v_seed_file, sizeof(u_v_seed_file), "%s%s%s", argv[1], separator,
           U_V_SEED_FILE);
  snprintf(params_file, sizeof(params_file), "%s%s%s", argv[1], separator,
           PARAMS_FILE);

  FILE *f_params_file = fopen(params_file, "r");
  if (f_params_file == NULL) {
    printf("Failed to open file: %s\n", params_file);
    perror("");
    return 1;
  }

  int result = 0;
  result = fscanf(f_params_file, "%lf", &du);
  if (result <= 0) {
    printf("Failed to read DU from %s\n", params_file);
    return 1;
  }

  result = fscanf(f_params_file, "%lf", &dv);
  if (result <= 0) {
    printf("Failed to read DV from %s\n", params_file);
    return 1;
  }

  result = fscanf(f_params_file, "%lf", &f);
  if (result <= 0) {
    printf("Failed to read F from %s\n", params_file);
    return 1;
  }

  result = fscanf(f_params_file, "%lf", &k);
  if (result <= 0) {
    printf("Failed to read K from %s\n", params_file);
    return 1;
  }

  result = fscanf(f_params_file, "%lf", &dt);
  if (result <= 0) {
    printf("Failed to read DT from %s\n", params_file);
    return 1;
  }

  result = fscanf(f_params_file, "%i", &steps);
  if (result <= 0) {
    printf("Failed to read steps from %s\n", params_file);
    return 1;
  }

  result = fscanf(f_params_file, "%i", &snapshot_interval);
  if (result <= 0) {
    printf("Failed to read snapshot interval from %s\n", params_file);
    return 1;
  }
  if (snapshot_interval <= 0) {
    printf("Invalid value for snapshot_interval = %i\n", snapshot_interval);
    return 1;
  }
  fclose(f_params_file);

  FILE *f_u_v_seed_file = fopen(u_v_seed_file, "r");
  if (f_u_v_seed_file == NULL) {
    printf("Failed to open file: %s\n", u_v_seed_file);
    perror("");
    return 1;
  }

  fscanf(f_u_v_seed_file, "%i", &n);
  u = calloc(n * n, sizeof(double));
  v = calloc(n * n, sizeof(double));

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      fscanf(f_u_v_seed_file, "%lf", &u[get_index(n, i, j)]);
    }
  }

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      fscanf(f_u_v_seed_file, "%lf", &v[get_index(n, i, j)]);
    }
  }

  fclose(f_u_v_seed_file);

  double *u_new = calloc(n * n, sizeof(double));
  double *v_new = calloc(n * n, sizeof(double));

  //   save initial state
  // write_png(outdir, 0, n, u, v);

  // calculation, indexes with wrapping
  for (int step = 1; step <= steps; step++) {
    for (int i = 0; i < n; i++) {
      int ia = (i == 0) ? n - 1 : i - 1; // row above
      int ib = (i == n - 1) ? 0 : i + 1; // row below
      for (int j = 0; j < n; j++) {
        int jl = (j == 0) ? n - 1 : j - 1; // left column
        int jr = (j == n - 1) ? 0 : j + 1; // right column
        int curr_index = get_index(n, i, j);

        if (u[curr_index] == -1) {
          u_new[curr_index] = -1;
          v_new[curr_index] = -1;

          continue;
        }

        // lap II: mirror border (no spilling)
        // no spilling onto "-1"
        double current = u[curr_index];
        double lap_u = get_neighbor_walled(current, u[get_index(n, ia, j)]) +
                       get_neighbor_walled(current, u[get_index(n, ib, j)]) +
                       get_neighbor_walled(current, u[(get_index(n, i, jl))]) +
                       get_neighbor_walled(current, u[get_index(n, i, jr)]) -
                       4 * u[curr_index];
        current = v[curr_index];
        double lap_v = get_neighbor_walled(current, v[get_index(n, ia, j)]) +
                       get_neighbor_walled(current, v[get_index(n, ib, j)]) +
                       get_neighbor_walled(current, v[(get_index(n, i, jl))]) +
                       get_neighbor_walled(current, v[get_index(n, i, jr)]) -
                       4 * v[curr_index];

        double uvv = u[curr_index] * v[curr_index] * v[curr_index];
        double curr_du = du * lap_u - uvv + f * (1.0 - u[curr_index]);
        double curr_dv = dv * lap_v + uvv - (f + k) * v[curr_index];

        u_new[curr_index] = u[curr_index] + dt * curr_du;
        v_new[curr_index] = v[curr_index] + dt * curr_dv;

        //   clamp [0, 1]
        if (u_new[curr_index] < 0)
          u_new[curr_index] = 0;
        if (u_new[curr_index] > 1)
          u_new[curr_index] = 1;
        if (v_new[curr_index] < 0)
          v_new[curr_index] = 0;
        if (v_new[curr_index] > 1)
          v_new[curr_index] = 1;
      }
    }

    double *tmp = u;
    u = u_new;
    u_new = tmp;
    tmp = v;
    v = v_new;
    v_new = tmp;

    if (step % snapshot_interval == 0) {
      // write_png(outdir, step, n, u, v);
    }
  }

  if (steps % snapshot_interval != 0) {
    // write_png(outdir, steps, n, u, v);
  }

  free(u);
  free(u_new);
  free(v);
  free(v_new);
  return 0;
}
