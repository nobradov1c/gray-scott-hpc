// gcc ./file-generator.c -o file-generator.out && ./file-generator.out

#include <stdio.h>

int main() {
  int n = 256;
  FILE *f_out = fopen("default-plane.txt", "w");
  int data = 0;

  if (f_out == NULL) {
    perror("Error opening file...");
    return 1;
  }

  //   save matrix dim
  fprintf(f_out, "%d %d\n", n, n);
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
