MPICC ?= mpicc
CFLAGS = -O3 -march=native -ffast-math
LDLIBS = -lm
INPUT ?= inputs/case4
MATRIX_DIMM ?= 512
MPI_PROCS ?= 8

.PHONY: clean run-sequential run-mpi gen-plane-bounds gen-u-v-seed

inputs/file-generator-plane-bounds.out: inputs/file-generator-plane-bounds.c
	$(CC) $(CFLAGS) inputs/file-generator-plane-bounds.c -o inputs/file-generator-plane-bounds.out

gen-plane-bounds: inputs/file-generator-plane-bounds.out
	./inputs/file-generator-plane-bounds.out $(MATRIX_DIMM)

inputs/file-generator-u-v.out: inputs/file-generator-u-v.c
	$(CC) $(CFLAGS) inputs/file-generator-u-v.c -o inputs/file-generator-u-v.out

gen-u-v-seed: inputs/file-generator-u-v.out
	./inputs/file-generator-u-v.out

gray_scott.out: gray_scott.c img_util.c img_util.h
	$(CC) $(CFLAGS) gray_scott.c img_util.c $(LDLIBS) -o gray_scott.out

run-sequential: gray_scott.out
	./gray_scott.out $(INPUT)

gray-scott-open-mpi.out: gray-scott-open-mpi.c img_util.c img_util.h
	$(MPICC) $(CFLAGS) gray-scott-open-mpi.c img_util.c $(LDLIBS) -o gray-scott-open-mpi.out

run-mpi: gray-scott-open-mpi.out
	mpirun -n $(MPI_PROCS) ./gray-scott-open-mpi.out $(INPUT)

clean:
	rm -f gray_scott.out gray-scott-open-mpi.out inputs/file-generator-plane-bounds.out inputs/file-generator-u-v.out
