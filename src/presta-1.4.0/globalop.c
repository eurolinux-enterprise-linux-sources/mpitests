/*

    This work was produced at the University of California, Lawrence Livermore
    National Laboratory (UC LLNL) under contract no. W-7405-ENG-48 (Contract
    48) between the U.S. Department of Energy (DOE) and The Regents of the
    University of California (University) for the operation of UC LLNL. The
    rights of the Federal Government are reserved under Contract 48 subject to
    the restrictions agreed upon by the DOE and University as allowed under DOE
    Acquisition Letter 97-1.

    DISCLAIMER

    This work was prepared as an account of work sponsored by an agency of the
    United States Government. Neither the United States Government nor the
    University of California nor any of their employees, makes any warranty,
    express or implied, or assumes any liability or responsibility for the
    accuracy, completeness, or usefulness of any information, apparatus,
    product, or process disclosed, or represents that its use would not
    infringe privately-owned rights.  Reference herein to any specific
    commercial products, process, or service by trade name, trademark,
    manufacturer or otherwise does not necessarily constitute or imply its
    endorsement, recommendation, or favoring by the United States Government or
    the University of California. The views and opinions of authors expressed
    herein do not necessarily state or reflect those of the United States
    Government or the University of California, and shall not be used for
    advertising or product endorsement purposes.

    NOTIFICATION OF COMMERCIAL USE

    Commercialization of this product is prohibited without notifying the
    Department of Energy (DOE) or Lawrence Livermore National Laboratory
    (LLNL).

    UCRL-CODE-2001-028


    globalop.c

    Syntax:
  
      globalop 
      
    Description:

      This test times MPI operation loops with and without calculation for
      MPI_Barrier, MPI_Reduce, MPI_Bcast, MPI_Allreduce, and 
      MPI_Reduce/MPI_Bcast.
      
*/


#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N		(1 << 12)


void crunch(double * blap, double * blip, double * whrrr)
{
  int i;
  double trouble;
  double nextblap = *blap;
  double nextblip = *blip;

  for (i = 0; i < 400; i++) 
  {
    if ( nextblap < 0 )
      nextblap *= -1;

    if ( nextblip < 0 )
      nextblip *= -1;

    if ( nextblap < 1 )
      nextblap += 1;

    if ( nextblip < 1 )
      nextblip += 1;

    trouble = log(nextblap) / sqrt(nextblip);
    nextblip = trouble - nextblap;
    nextblap = trouble * nextblip;
  }

  *whrrr = trouble;
}


void set(int arrsize, double *num)
{
  int	i;

  srand((unsigned int) num[i % arrsize]);

  for (i = 0; i < arrsize; i++) num[i] = (double)rand();
}


main(int argc, char **argv)
{
  int	myid;
  int	ntasks;
  double	num;
  double	max;
  double	start_time, end_time;
  double	elapsed_time, sum_elapsed_time;
  double	run_start_time, run_end_time, run_elapsed_time, sum_run_time;
  double	work_time, reduce_time, allreduce_time, bcast_time,
    reduce_bcast_time, split_rb_time, barrier_time, timer_time;
  double	min_reduce_time, max_reduce_time;
  double	min_bcast_time, max_bcast_time;
  double	min_allreduce_time, max_allreduce_time;
  double	min_reduce_bcast_time, max_reduce_bcast_time;
  double	min_split_rb_time, max_split_rb_time;
  double	x[N], y[N], z[N];
  int	i, k;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  MPI_Comm_size(MPI_COMM_WORLD, &ntasks);

  run_start_time = MPI_Wtime();

  if (myid == 0) {
    printf("Testing N=%d, %d tasks\n", N, ntasks);
  }

  set(N, x);
  set(N, y);

  for (k = 0; k < 3; k++) {

    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {

      crunch(&x[i], &y[i], &z[i]);
      num = z[i];
      max = -1;
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    sum_elapsed_time = 0;

    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      work_time = sum_elapsed_time / ntasks;

      printf("Average work time per task is %7.4f\n",
             work_time);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {

      crunch(&x[i], &y[i], &z[i]);
      num = z[i];
      max = -1;

      MPI_Barrier(MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      barrier_time = sum_elapsed_time / ntasks;
      printf("Average barrier + work time per task is %7.2f\n",
             barrier_time);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {

      MPI_Barrier(MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      barrier_time = sum_elapsed_time / ntasks;
      printf("Average barrier time per task is %7.2f\n",
             barrier_time);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {

      crunch(&x[i], &y[i], &z[i]);
      num = z[i];
      max = -1;

      MPI_Reduce(&num, &max, 1, MPI_DOUBLE, MPI_MAX, 0,
                 MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &min_reduce_time, 1, MPI_DOUBLE,
               MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &max_reduce_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      reduce_time = sum_elapsed_time / ntasks;
      printf("Average reduce + work time per task is %7.2f\n",
             reduce_time);
      printf("Minimum reduce + work time was %7.2f\n",
             min_reduce_time);
      printf("Maximum reduce + work time was %7.2f\n",
             max_reduce_time);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {
      MPI_Reduce(&num, &max, 1, MPI_DOUBLE, MPI_MAX, 0,
                 MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &min_reduce_time, 1, MPI_DOUBLE,
               MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &max_reduce_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      reduce_time = sum_elapsed_time / ntasks;
      printf("Average reduce time per task is %7.2f\n",
             reduce_time);
      printf("Minimum reduce time was %7.2f\n",
             min_reduce_time);
      printf("Maximum reduce time was %7.2f\n",
             max_reduce_time);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {

      crunch(&x[i], &y[i], &z[i]);
      num = z[i];
      max = -1;

      MPI_Bcast(&num, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &min_bcast_time, 1, MPI_DOUBLE,
               MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &max_bcast_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      bcast_time = sum_elapsed_time / ntasks;
      printf("Average bcast + work time per task is %7.2f\n",
             bcast_time);
      printf("Minimum bcast + work time was %7.2f\n",
             min_bcast_time);
      printf("Maximum bcast + work time was %7.2f\n",
             max_bcast_time);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {
      MPI_Bcast(&num, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &min_bcast_time, 1, MPI_DOUBLE,
               MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &max_bcast_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      bcast_time = sum_elapsed_time / ntasks;
      printf("Average bcast time per task is %7.2f\n",
             bcast_time);
      printf("Minimum bcast time was %7.2f\n",
             min_bcast_time);
      printf("Maximum bcast time was %7.2f\n",
             max_bcast_time);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {

      crunch(&x[i], &y[i], &z[i]);
      num = z[i];
      max = -1;

      MPI_Allreduce(&num, &max, 1, MPI_DOUBLE, MPI_MAX,
                    MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &min_allreduce_time, 1, MPI_DOUBLE,
               MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &max_allreduce_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      allreduce_time = sum_elapsed_time / ntasks;
      printf("Average allreduce + work time per task is %7.2f\n",
             allreduce_time);
      printf("Minimum allreduce + work time was %7.2f\n",
             min_allreduce_time);
      printf("Maximum allreduce + work time was %7.2f\n",
             max_allreduce_time);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {
      MPI_Allreduce(&num, &max, 1, MPI_DOUBLE, MPI_MAX,
                    MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &min_allreduce_time, 1, MPI_DOUBLE,
               MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &max_allreduce_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      allreduce_time = sum_elapsed_time / ntasks;
      printf("Average allreduce time per task is %7.2f\n",
             allreduce_time);
      printf("Minimum allreduce time was %7.2f\n",
             min_allreduce_time);
      printf("Maximum allreduce time was %7.2f\n",
             max_allreduce_time);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {
      MPI_Reduce(&num, &max, 1, MPI_DOUBLE, MPI_MAX, 0,
                 MPI_COMM_WORLD);
      MPI_Bcast(&max, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &min_reduce_bcast_time, 1, MPI_DOUBLE,
               MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &max_reduce_bcast_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      reduce_bcast_time = sum_elapsed_time / ntasks;
      printf("Average reduce-bcast time per task is %7.2f\n",
             reduce_bcast_time);
      printf("Minimum reduce-bcast time was %7.2f\n",
             min_reduce_bcast_time);
      printf("Maximum reduce-bcast time was %7.2f\n",
             max_reduce_bcast_time);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    for (i = 0; i < N; i++) {
      MPI_Reduce(&num, &max, 1, MPI_DOUBLE, MPI_MAX, 0,
                 MPI_COMM_WORLD);

      crunch(&x[i], &y[i], &z[i]);
      num = z[i];
      max = -1;

      MPI_Bcast(&max, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();

    elapsed_time = end_time - start_time;

    sum_elapsed_time = 0;

    MPI_Reduce(&elapsed_time, &min_split_rb_time, 1, MPI_DOUBLE,
               MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &max_split_rb_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&elapsed_time, &sum_elapsed_time, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
      split_rb_time = sum_elapsed_time / ntasks;
      printf("Average reduce-work-bcast time per task is %7.2f\n",
             split_rb_time);
      printf("Minimum reduce-work-bcast time was %7.2f\n",
             min_split_rb_time);
      printf("Maximum reduce-work-bcast time was %7.2f\n",
             max_split_rb_time);
    }

    if (myid == 0) {
      printf("\n");
    }

  } /* loop over k */

  run_end_time = MPI_Wtime();

  run_elapsed_time = run_end_time - run_start_time;

  sum_run_time = 0;

  MPI_Reduce(&run_elapsed_time, &sum_run_time, 1, MPI_DOUBLE,
             MPI_SUM, 0, MPI_COMM_WORLD);


  MPI_Finalize();

  if (myid == 0) {
    run_elapsed_time = sum_run_time / ntasks;

    printf("Average elapsed run time was %7.2f\n",
           run_elapsed_time);

    printf("All done\n");
  }

  exit (0);
}
