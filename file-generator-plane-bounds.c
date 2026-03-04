/*
gcc ./file-generator-plane-bounds.c -o file-generator-plane-bounds.out &&
./file-generator-plane-bounds.out
*/

#include <stdio.h>

int main() {
  int n = 256;
  FILE *f_out = fopen("plane-bounds.txt", "w");
  int data = 1;

  if (f_out == NULL) {
    perror("Error opening file...");
    return 1;
  }

  //   save matrix dim
  fprintf(f_out, "%d\n", n);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      fprintf(f_out, "%d", data);
      if (j < n - 1) {
        fprintf(f_out, " ");
      }
    }
    fprintf(f_out, "\n");
  }
  fclose(f_out);

  return 0;
}
