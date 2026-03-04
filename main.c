// mpicc ./main.c -o main.out && mpirun -n 4 ./main.out default-plane.txt

#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  int process_rank, process_count;
  MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &process_count);

  int m = 0, n = 0;
  double *matrix = NULL;

  if (argc != 2) {
    if (process_rank == 0) {
      printf("Usage: %s <input_plane_file>\n", argv[0]);
    }
    MPI_Finalize();
    return 0;
  }

  if (process_rank == 0) {
    FILE *f_in = fopen(argv[1], "r");
    if (f_in == NULL) {
      perror("Error opening file...");
    } else {
      /* Expected file format:
         m n
         a[0][0] a[0][1] ... a[0][n-1]
         ...
         a[m-1][0] ...  a[m-1][n-1]
      */
      if (fscanf(f_in, "%d %d", &m, &n) != 2) {
        printf("Failed reading m and n from file.\n");
      }

      matrix = calloc(m * n, sizeof(double));
      for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
          fscanf(f_in, "%lf", &matrix[i * n + j]);
        }
      }
      fclose(f_in);
    }
  }
  MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (m == 0 || n == 0) {
    if (process_rank == 0) {
      printf("Failed parsing plane input file...\n");
    }
    MPI_Finalize();
    return 0;
  }

  //   if (process_rank == 0) {
  //     printf("%.2f %.2f %.2f\n", matrix[0], matrix[1], matrix[n * m - 1]);
  //   }

  if (process_rank == 0) {
    free(matrix);
  }
  MPI_Finalize();
  return 0;
}
