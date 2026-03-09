#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "img_util.h"

#define U_V_SEED_FILE "u-v-seed.txt"
#define PARAMS_FILE "params.txt"

int64_t unix_timestamp_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
    return -1;
  return (int64_t)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int get_index(int n, int i, int j) { return i * n + j; }

int process_prev(int process_count, int i) {
  return (i > 0) ? i - 1 : process_count - 1;
}

int process_next(int process_count, int i) {
  return (i == process_count - 1) ? 0 : i + 1;
}

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
  MPI_Init(&argc, &argv);

  int my_rank;
  int process_count;
  MPI_Comm_size(MPI_COMM_WORLD, &process_count);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if (argc != 2) {
    if (my_rank == 0) {
      printf("Usage: %s <directory with input files>\n", argv[0]);
    }

    MPI_Finalize();
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
  int time_start;
  int time_end;

  if (my_rank == 0) {
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
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int result = 0;
    result = fscanf(f_params_file, "%lf", &du);
    if (result <= 0) {
      printf("Failed to read DU from %s\n", params_file);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    result = fscanf(f_params_file, "%lf", &dv);
    if (result <= 0) {
      printf("Failed to read DV from %s\n", params_file);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    result = fscanf(f_params_file, "%lf", &f);
    if (result <= 0) {
      printf("Failed to read F from %s\n", params_file);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    result = fscanf(f_params_file, "%lf", &k);
    if (result <= 0) {
      printf("Failed to read K from %s\n", params_file);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    result = fscanf(f_params_file, "%lf", &dt);
    if (result <= 0) {
      printf("Failed to read DT from %s\n", params_file);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    result = fscanf(f_params_file, "%i", &steps);
    if (result <= 0) {
      printf("Failed to read steps from %s\n", params_file);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    result = fscanf(f_params_file, "%i", &snapshot_interval);
    if (result <= 0) {
      printf("Failed to read snapshot interval from %s\n", params_file);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (snapshot_interval <= 0) {
      printf("Invalid value for snapshot_interval = %i\n", snapshot_interval);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    fclose(f_params_file);

    FILE *f_u_v_seed_file = fopen(u_v_seed_file, "r");
    if (f_u_v_seed_file == NULL) {
      printf("Failed to open file: %s\n", u_v_seed_file);
      perror("");
      MPI_Abort(MPI_COMM_WORLD, 1);
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

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(outdir, sizeof(outdir), "out_%04d%02d%02d_%02d%02d%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
             tm.tm_min, tm.tm_sec);
    mkdir(outdir, 0755);
  }

  MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&du, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&dv, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&f, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&k, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&dt, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&snapshot_interval, 1, MPI_INT, 0, MPI_COMM_WORLD);

  //   printf("process rank %i, has du = %.6f\n", my_rank, du);

  int rows_per_process = n / process_count;
  int rem = n % process_count;
  int local_rows = rows_per_process + (my_rank < rem ? 1 : 0);
  //   printf("process rank %i, has %i local_rows\n", my_rank, local_rows);
  double *local_u = calloc((local_rows + 2) * n, sizeof(double));
  double *local_u_new = calloc((local_rows + 2) * n, sizeof(double));
  double *local_v = calloc((local_rows + 2) * n, sizeof(double));
  double *local_v_new = calloc((local_rows + 2) * n, sizeof(double));
  int *sendcounts = NULL, *displs = NULL;
  if (my_rank == 0) {
    sendcounts = malloc(process_count * sizeof(int));
    displs = malloc(process_count * sizeof(int));
    for (int i = 0; i < process_count; i++) {
      sendcounts[i] = (rows_per_process + (i < rem ? 1 : 0)) * n;
      displs[i] = (i == 0) ? 0 : displs[i - 1] + sendcounts[i - 1];
    }
  }
  MPI_Scatterv(u, sendcounts, displs, MPI_DOUBLE, local_u + n, local_rows * n,
               MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Scatterv(v, sendcounts, displs, MPI_DOUBLE, local_v + n, local_rows * n,
               MPI_DOUBLE, 0, MPI_COMM_WORLD);

  // sending first and last rows to neighbors with wrapping
  //   u matrix
  MPI_Sendrecv(local_u + n, n, MPI_DOUBLE, process_prev(process_count, my_rank),
               0, local_u + (local_rows + 1) * n, n, MPI_DOUBLE,
               process_next(process_count, my_rank), 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

  MPI_Sendrecv(local_u + local_rows * n, n, MPI_DOUBLE,
               process_next(process_count, my_rank), 0, local_u, n, MPI_DOUBLE,
               process_prev(process_count, my_rank), 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

  //    v matrix
  MPI_Sendrecv(local_v + n, n, MPI_DOUBLE, process_prev(process_count, my_rank),
               0, local_v + (local_rows + 1) * n, n, MPI_DOUBLE,
               process_next(process_count, my_rank), 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

  MPI_Sendrecv(local_v + local_rows * n, n, MPI_DOUBLE,
               process_next(process_count, my_rank), 0, local_v, n, MPI_DOUBLE,
               process_prev(process_count, my_rank), 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

  //   if (my_rank == 3) {
  //     print_matrix(local_u, local_rows + 2, n);
  //   }

  if (my_rank == 0) {
    write_png(outdir, 0, n, u, v);
    time_start = unix_timestamp_ms();
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // calculation, indexes with wrapping
  for (int step = 1; step <= steps; step++) {
    for (int i = 1; i <= local_rows; i++) {
      int ia = i - 1; // row above
      int ib = i + 1; // row below
      for (int j = 0; j < n; j++) {
        int jl = (j == 0) ? n - 1 : j - 1; // left column
        int jr = (j == n - 1) ? 0 : j + 1; // right column
        int curr_index = get_index(n, i, j);

        // if we are out of plane bounds
        if (local_u[curr_index] == -1) {
          local_u_new[curr_index] = -1;
          local_v_new[curr_index] = -1;

          continue;
        }

        // lap variants
        // lap I: flow out (spill) onto "-1", no borders
        // double lap_u = local_u[get_index(n, ia, j)] +
        //                local_u[get_index(n, ib, j)] +
        //                local_u[(get_index(n, i, jl))] +
        //                local_u[get_index(n, i, jr)] - 4 *
        //                local_u[curr_index];
        // double lap_v = local_v[get_index(n, ia, j)] +
        //                local_v[get_index(n, ib, j)] +
        //                local_v[(get_index(n, i, jl))] +
        //                local_v[get_index(n, i, jr)] - 4 *
        //                local_v[curr_index];

        // lap II: mirror border (no spilling)
        // no spilling onto "-1"
        double current = local_u[curr_index];
        double lap_u =
            get_neighbor_walled(current, local_u[get_index(n, ia, j)]) +
            get_neighbor_walled(current, local_u[get_index(n, ib, j)]) +
            get_neighbor_walled(current, local_u[(get_index(n, i, jl))]) +
            get_neighbor_walled(current, local_u[get_index(n, i, jr)]) -
            4 * local_u[curr_index];
        current = local_v[curr_index];
        double lap_v =
            get_neighbor_walled(current, local_v[get_index(n, ia, j)]) +
            get_neighbor_walled(current, local_v[get_index(n, ib, j)]) +
            get_neighbor_walled(current, local_v[(get_index(n, i, jl))]) +
            get_neighbor_walled(current, local_v[get_index(n, i, jr)]) -
            4 * local_v[curr_index];

        double uvv =
            local_u[curr_index] * local_v[curr_index] * local_v[curr_index];
        double curr_du = du * lap_u - uvv + f * (1.0 - local_u[curr_index]);
        double curr_dv = dv * lap_v + uvv - (f + k) * local_v[curr_index];

        local_u_new[curr_index] = local_u[curr_index] + dt * curr_du;
        local_v_new[curr_index] = local_v[curr_index] + dt * curr_dv;

        //   clamp [0, 1]
        if (local_u_new[curr_index] < 0)
          local_u_new[curr_index] = 0;
        if (local_u_new[curr_index] > 1)
          local_u_new[curr_index] = 1;
        if (local_v_new[curr_index] < 0)
          local_v_new[curr_index] = 0;
        if (local_v_new[curr_index] > 1)
          local_v_new[curr_index] = 1;
      }
    }

    double *tmp = local_u;
    local_u = local_u_new;
    local_u_new = tmp;
    tmp = local_v;
    local_v = local_v_new;
    local_v_new = tmp;

    // if (step % snapshot_interval == 0) {
    //   MPI_Gatherv(local_u + n, local_rows * n, MPI_DOUBLE, u, sendcounts,
    //               displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    //   MPI_Gatherv(local_v + n, local_rows * n, MPI_DOUBLE, v, sendcounts,
    //               displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    //   if (my_rank == 0) {
    //     write_png(outdir, step, n, u, v);
    //   }
    // }

    if (step < steps) {
      // sending first and last rows to neighbors with wrapping
      //   u matrix
      MPI_Sendrecv(local_u + n, n, MPI_DOUBLE,
                   process_prev(process_count, my_rank), 0,
                   local_u + (local_rows + 1) * n, n, MPI_DOUBLE,
                   process_next(process_count, my_rank), 0, MPI_COMM_WORLD,
                   MPI_STATUS_IGNORE);

      MPI_Sendrecv(local_u + local_rows * n, n, MPI_DOUBLE,
                   process_next(process_count, my_rank), 0, local_u, n,
                   MPI_DOUBLE, process_prev(process_count, my_rank), 0,
                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      //    v matrix
      MPI_Sendrecv(local_v + n, n, MPI_DOUBLE,
                   process_prev(process_count, my_rank), 0,
                   local_v + (local_rows + 1) * n, n, MPI_DOUBLE,
                   process_next(process_count, my_rank), 0, MPI_COMM_WORLD,
                   MPI_STATUS_IGNORE);

      MPI_Sendrecv(local_v + local_rows * n, n, MPI_DOUBLE,
                   process_next(process_count, my_rank), 0, local_v, n,
                   MPI_DOUBLE, process_prev(process_count, my_rank), 0,
                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
  }

  //   gathering final results
  //   gather local v and u
  //   sendcounts and displs are already configured correctly
  MPI_Gatherv(local_u + n, local_rows * n, MPI_DOUBLE, u, sendcounts, displs,
              MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Gatherv(local_v + n, local_rows * n, MPI_DOUBLE, v, sendcounts, displs,
              MPI_DOUBLE, 0, MPI_COMM_WORLD);
  if (my_rank == 0) {
    time_end = unix_timestamp_ms();
    int total_time_ms = time_end - time_start;
    printf("Calc took %i ms\n", total_time_ms);

    write_png(outdir, steps, n, u, v);
  }

  free(local_u);
  free(local_u_new);
  free(local_v);
  free(local_v_new);
  if (my_rank == 0) {
    // print_matrix(u, n, n);
    free(u);
    free(v);
    free(sendcounts);
    free(displs);
  }
  MPI_Finalize();
  return 0;
}
