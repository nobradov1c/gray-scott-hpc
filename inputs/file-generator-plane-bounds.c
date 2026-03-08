/*
gcc ./file-generator-plane-bounds.c -o file-generator-plane-bounds.out &&
./file-generator-plane-bounds.out
*/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  int n = 0;
  if (argc != 2) {
    printf("Usage: %s <matrix-dim>\n", argv[0]);
    return 1;
  }
  n = atoi(argv[1]);
  if (n <= 0) {
    printf("Invalid matrix dim n = %i\n", n);
    return 1;
  }

  int mask = 1;
  int inactive = 0;

  FILE *f_out = fopen("plane-bounds.txt", "w");
  if (f_out == NULL) {
    perror("Error opening file...");
    return 1;
  }

  //   save matrix dim
  fprintf(f_out, "%d\n", n);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (i <= j) {
        fprintf(f_out, "%d", mask);
      } else {
        fprintf(f_out, "%d", mask);
      }
      if (j < n - 1) {
        fprintf(f_out, " ");
      }
    }
    fprintf(f_out, "\n");
  }
  fclose(f_out);

  return 0;
}
