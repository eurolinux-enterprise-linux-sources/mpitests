CC		= ${MPIHOME}/bin/mpicc
MPI1_BENCHMARKS	= osu_bcast osu_bibw osu_bw osu_latency osu_mbw_mr
MPI2_BENCHMARKS	= osu_acc_latency osu_get_bw osu_get_latency osu_latency_mt\
		  osu_put_bibw osu_put_bw osu_put_latency
ALL_BENCHMARKS	= ${MPI1_BENCHMARKS} ${MPI2_BENCHMARKS}

osu_bcast: LDFLAGS = -lm

# By default only MPI-1 Benchmarks are built
mpi1:	${MPI1_BENCHMARKS}
mpi2:	${ALL_BENCHMARKS}
all:	${ALL_BENCHMARKS}

install:
	mkdir -p ${PREFIX}/osu_benchmarks &&\
	  for benchmark in ${ALL_BENCHMARKS}; do\
	  if [ -e $$benchmark ]; then\
	  cp $$benchmark ${PREFIX}/osu_benchmarks/;\
	  fi; done

clean:
	rm -f ${ALL_BENCHMARKS} ${ALL_BENCHMARKS:%=%.o}

${ALL_BENCHMARKS:%=%.o}: osu.h

.DELETE_ON_ERROR:
.PHONY: mpi1 mpi2 all install clean

