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


    glob.c
    
*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>
#include "util.h"


static char *outputFormat =
  "  %-24s  %9d  %15d  %5d  %12.3f  %12.3f  %12.3f\n";
static char *outputCharFormat = "  %-24s  %9s  %15s  %5s  %12s  %12s  %12s\n";
static double minTime = 99999.0;
char *executableName = "glob";
int zeroArray = 0;
int rotateRoot = 0;

typedef enum TESTTYPE_TYPE
{ REDUCE_DBL_SUM_RX, REDUCE_DBL_SUM_R0, REDUCE_FLT_SUM_R0, REDUCE_FLT_SUM_RX,
  BCAST, REDUCEBCAST, ALLREDUCE, BARRIER, TYPETOT
} TESTTYPE;

typedef double (*TESTFUNC) (int, int, int, MPI_Comm *);

typedef struct
{
  char *name;
  char *desc;
  double rankResult;
  double maxResult;
  int iters;
  double sendFactor;
  int *messList;
  int messListSize;
  double maxBW;
  int maxBWMessSize;
  TESTFUNC testFunc;
  MPI_Datatype dataType;
  MPI_Op op;
  TESTTYPE test;
  int changeRoot;

} TESTPARAMS;

TESTPARAMS *currentParams;
TESTPARAMS *testParams;


void runTest (TESTPARAMS * testParams);
int generateResults (TESTPARAMS * testParams, int procs, int messSize,
		     int iters, MPI_Comm activeComm);
double runUnicomTest (int procs, int bufsize, int iters,
		      MPI_Comm * activeComm);
double runNonblockUnicomTest (int procs, int bufsize, int iters,
			      MPI_Comm * activeComm);
double runNonblockBicomTest (int procs, int bufsize, int iters,
			     MPI_Comm * activeComm);
double runBicomTest (int procs, int bufsize, int iters,
		     MPI_Comm * activeComm);
double runLatencyTest (int procs, int bufsize, int iters,
		       MPI_Comm * activeComm);
void printUse (void);
void printParameters ();
void printReportHeader (void);
int setupTestListParams ();
int initAllTestTypeParams (TESTPARAMS ** testParams);
void freeBuffers (TESTPARAMS ** testParams);
int getNextTargetFile ();
extern int getTargetList ();


int
main (int argc, char *argv[])
{
  int *messList = NULL;
  int testIdx, doTestLoop;


  MPI_Init (&argc, &argv);

  /* Set global wsize and rank values */
  MPI_Comm_size (MPI_COMM_WORLD, &wsize);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  if (wsize % 2 != 0 || wsize < 2)
    {
      prestaRankPrint (0, "\n  The process count must be even and > 1.\n\n");
      MPI_Finalize ();
      exit (1);
    }

  if (!initAllTestTypeParams (&testParams))
    return (-1);

  argStruct.testList = "Reduce:Double-SUM-R0";

  if (!processArgs (argc, argv))
    {
      if (rank == 0)
	printUse ();

      MPI_Finalize ();
      exit (1);
    }

  /* If using a source directory of process rank target files,
   * get the next appropriate file.
   */
  if (targetDirectory != NULL && getNextTargetFile () == 0)
    {
      prestaAbort ("Failed to open target file in target directory %s\n",
		   targetDirectory);
    }

  doTestLoop = 1;
  while (doTestLoop)
    {
      if (!setupTestListParams () || !initAllTestTypeParams (&testParams))
	{
	  if (rank == 0)
	    printUse ();

	  MPI_Finalize ();
	  exit (1);
	}

      if (rank == 0)
	printReportHeader ();

      for (testIdx = 0; testIdx < TYPETOT; testIdx++)
	{
	  prestaRankDebug (0, "running test index %d\n", testIdx);
	  if (argStruct.testList == NULL
	      || (argStruct.testList != NULL
		  && strstr (argStruct.testList,
			     testParams[testIdx].name) != NULL))
	    runTest (&testParams[testIdx]);
	}

      if (rank == 0)
	{
	  printf ("\n");

	  printParameters ();
	}

      argStruct.sumLocalBW = -1;	/* Do not print Bandwidth calculation information */
      printReportFooter (minTime, argStruct.procsPerNode,
			 argStruct.useNearestRank);

      prestaRankPrint (0,
		       "\n#######################################################################################################\n");

      if (targetDirectory == NULL || getNextTargetFile () == 0)
	{
	  doTestLoop = 0;
	}
    }

  freeBuffers (&testParams);
  free (messList);

  MPI_Finalize ();

  exit (0);
}


void
printReportHeader (void)
{
  char *now;
  int nameSize;
  char myProcName[MPI_MAX_PROCESSOR_NAME];

  now = getTimeStr ();
  MPI_Get_processor_name (myProcName, &nameSize);

  printf
    ("\n#######################################################################################################\n");
  printf ("\n  glob MPI Collective Communication Benchmark\n");
  printf ("  Version %d.%d.%d\n", majorVersion, minorVersion, patchVersion);
  printf ("  Run at %s, with rank 0 on %s\n", now, myProcName);

  if (strlen (procSrcTitle) > 0)
    printf ("  Using Task Pair file %s\n", procSrcTitle);

  printf
    ("\n#######################################################################################################\n");
  printf ("\n");
  printf (outputCharFormat, "Test", "Processes", "Op Size (bytes)",
	  "Ops", "Min (us)", "Avrg (us)", "Max (us)");
  free (now);
}


void
runTest (TESTPARAMS * testParams)
{
  int procIdx, procs, messIdx;
  MPI_Comm activeComm = MPI_COMM_NULL;
  int *messList, messListSize;
  int iters;

  messList = testParams->messList;
  messListSize = testParams->messListSize;

  testParams->maxBW = 0.0;
  testParams->maxBWMessSize = 0;

  currentParams = testParams;

  for (procIdx = 0; procIdx < argStruct.procListSize; procIdx++)
    {
      procs = argStruct.procList[procIdx];

      /*  Create Communicator of all active processes  */
      createActiveComm (procs, argStruct.procsPerNode, 'l',	/* Use fixed linear allocation, i.e. 0, 1, 2, etc. */
			argStruct.useNearestRank, &activeComm);

      prestaDebug ("rank %d returned from createActiveCom\n", rank);

      if (argStruct.printPairs)
	printGlobGroup (activeComm);

      for (messIdx = 0; messIdx < messListSize; messIdx++)
	{
	  if (argStruct.iterList != NULL && argStruct.iterList[messIdx] != 0)
	    iters = argStruct.iterList[messIdx];
	  else
	    iters = argStruct.iters;

	  /*  Run test and save current result  */
	  testParams->rankResult =
	    testParams->testFunc (procs, messList[messIdx], iters,
				  &activeComm);

	  if (testParams->rankResult < minTime)
	    minTime = testParams->rankResult;

	  if (!generateResults
	      (testParams, procs, messList[messIdx], iters, activeComm))
	    prestaAbort ("Failed to generate test results.");
	}
      if (testParams->test != BARRIER)
	prestaRankPrint (0, "\n");
    }

  if (activeComm != MPI_COMM_NULL)
    MPI_Comm_free (&activeComm);
}


int
generateResults (TESTPARAMS * testParams, int procs, int messSize,
		 int iters, MPI_Comm activeComm)
{
  double minTime, maxTime, sumTime, aggCommSize, currBW;

  if (isActiveProc (&activeComm))
    {

      prestaDebug ("rank %d rankResult is %f\n", rank,
		   testParams->rankResult);
      MPI_Reduce (&testParams->rankResult, &minTime, 1, MPI_DOUBLE, MPI_MIN,
		  0, activeComm);
      MPI_Reduce (&testParams->rankResult, &maxTime, 1, MPI_DOUBLE, MPI_MAX,
		  0, activeComm);
      MPI_Reduce (&testParams->rankResult, &sumTime, 1, MPI_DOUBLE, MPI_SUM,
		  0, activeComm);

      if (maxTime > testParams->maxResult)
	testParams->maxResult = maxTime;

      aggCommSize =
	(double) iters *testParams->sendFactor *
	(double) procs *(double) messSize;
      currBW = (aggCommSize / (double) maxTime) / MB_SIZE;

      prestaRankDebug (0, "iters is %d\n", iters);
      prestaRankDebug (0, "procs is %d\n", procs);
      prestaRankDebug (0, "messSize is %d\n", messSize);
      prestaRankDebug (0, "aggCommSize is %f\n", aggCommSize);
      prestaRankDebug (0, "maxTime is %f\n", maxTime);

      prestaRankPrint (0, outputFormat, testParams->name, procs, messSize,
		       iters, (minTime / iters) * 1000000,
		       ((sumTime / procs) / iters) * 1000000,
		       (maxTime / iters) * 1000000);

      if (currBW > testParams->maxBW)
	{
	  testParams->maxBW = currBW;
	  testParams->maxBWMessSize = messSize;
	}

      if (argStruct.printRankTime)
	{
	  if (maxTime > 0.0)
	    printf ("  Rank %4d time : %f\n", rank, maxTime);
	}
    }
  MPI_Barrier (MPI_COMM_WORLD);

  return 1;
}


/*
 * allocateBuffers
 * 
 * If pointers to buffer pointers are not NULL, the appropriate memory is 
 * allocated.  An allreduce is done to determine if all processes succeeding in 
 * allocating the appropriate memory.
 */
int
allocateBuffers (char **sendBuf, char **recvBuf, int sendSize, int recvSize,
		 MPI_Comm comm)
{
  int retval = 1, gretval;

  if (sendBuf != NULL)
    {
      *sendBuf = (char *) malloc (sendSize);
      if (*sendBuf == NULL)
	retval = 0;
    }

  if (recvBuf != NULL)
    {
      *recvBuf = (char *) malloc (recvSize);
      if (sendBuf == NULL)
	retval = 0;
    }

  MPI_Allreduce (&retval, &gretval, 1, MPI_INT, MPI_MIN, comm);

  if (gretval == 1)
    {
      if (sendBuf != NULL)
	memset (*sendBuf, 0, sendSize);
      if (recvBuf != NULL)
	memset (*recvBuf, 0, recvSize);
    }
  else
    {
      if (sendBuf != NULL && *sendBuf != NULL)
	free (*sendBuf);
      if (recvBuf != NULL && *recvBuf != NULL)
	free (*recvBuf);
    }

  return gretval;
}


double
runReduceTest (int procs, int bufsize, int iters, MPI_Comm * activeComm)
{
  int i, root;
  double diff = 0.0;
  double start = 0.0;
  char *sendBuf, *recvBuf;
  int dtSize, count;
  TESTPARAMS *tp;

  tp = currentParams;

  prestaDebug ("in runReduceTest\n");

  if (isActiveProc (activeComm)
      && allocateBuffers (&sendBuf, &recvBuf, bufsize, bufsize, *activeComm))
    {
      prestaDebug ("active Process in runReduceTest\n");

      MPI_Type_size (tp->dataType, &dtSize);
      count = bufsize / dtSize;
      /*  Ensure communication paths have been initialized  */
      MPI_Reduce (sendBuf, recvBuf, count, tp->dataType, tp->op, 0,
		  *activeComm);

      generic_barrier (*activeComm);
      generic_barrier (*activeComm);

      start = MPI_Wtime ();

      for (i = 0; i < iters; i++)
	{
	  if (tp->changeRoot == 1)
	    root = i % procs;
	  else
	    root = 0;
	  MPI_Reduce (sendBuf, recvBuf, count, tp->dataType, tp->op,
		      root, *activeComm);
	}

      if (argStruct.useBarrier)
	generic_barrier (*activeComm);

      diff = MPI_Wtime () - start;

      free (sendBuf);
      free (recvBuf);
    }

  MPI_Barrier (MPI_COMM_WORLD);

  return diff;
}


double
runBcastTest (int procs, int bufsize, int iters, MPI_Comm * activeComm)
{
  int i;
  double diff = 0.0;
  double start = 0.0;
  char *sendBuf;
  int dtSize;
  TESTPARAMS *tp;

  tp = currentParams;

  prestaDebug ("in runBcastTest\n");

  if (isActiveProc (activeComm)
      && allocateBuffers (&sendBuf, NULL, bufsize, 0, *activeComm))
    {
      prestaDebug ("active Process in runBcastTest\n");

      MPI_Type_size (tp->dataType, &dtSize);
      /*  Ensure communication paths have been initialized  */
      MPI_Bcast (sendBuf, bufsize / dtSize, tp->dataType, 0, *activeComm);

      generic_barrier (*activeComm);
      generic_barrier (*activeComm);

      start = MPI_Wtime ();

      for (i = 0; i < iters; i++)
	MPI_Bcast (sendBuf, bufsize / dtSize, tp->dataType, 0, *activeComm);

      if (argStruct.useBarrier)
	generic_barrier (*activeComm);

      diff = MPI_Wtime () - start;

      free (sendBuf);
    }

  MPI_Barrier (MPI_COMM_WORLD);

  return diff;
}


double
runReduceBcastTest (int procs, int bufsize, int iters, MPI_Comm * activeComm)
{
  int i;
  double diff = 0.0;
  double start = 0.0;
  char *sendBuf = NULL, *recvBuf = NULL;
  int dtSize;
  TESTPARAMS *tp;

  tp = currentParams;

  prestaDebug ("in runReduceBcastTest\n");

  if (isActiveProc (activeComm)
      && allocateBuffers (&sendBuf, &recvBuf, bufsize, bufsize, *activeComm))
    {
      prestaDebug ("active Process in runReduceBcastTest\n");

      MPI_Type_size (tp->dataType, &dtSize);
      /*  Ensure communication paths have been initialized  */
      MPI_Reduce (sendBuf, recvBuf, bufsize / dtSize, tp->dataType, tp->op, 0,
		  *activeComm);
      MPI_Bcast (sendBuf, bufsize / dtSize, tp->dataType, 0, *activeComm);

      generic_barrier (*activeComm);
      generic_barrier (*activeComm);

      start = MPI_Wtime ();

      for (i = 0; i < iters; i++)
	{
	  MPI_Reduce (sendBuf, recvBuf, bufsize / dtSize, tp->dataType,
		      tp->op, 0, *activeComm);
	  MPI_Bcast (sendBuf, bufsize / dtSize, tp->dataType, 0, *activeComm);
	}

      if (argStruct.useBarrier)
	generic_barrier (*activeComm);

      diff = MPI_Wtime () - start;

      free (sendBuf);
      free (recvBuf);
    }

  MPI_Barrier (MPI_COMM_WORLD);

  return diff;
}


double
runAllreduceTest (int procs, int bufsize, int iters, MPI_Comm * activeComm)
{
  int i;
  double diff = 0.0;
  double start = 0.0;
  char *sendBuf = NULL, *recvBuf = NULL;
  int dtSize;
  TESTPARAMS *tp;

  tp = currentParams;

  prestaDebug ("in runAllreduceTest\n");

  if (isActiveProc (activeComm)
      && allocateBuffers (&sendBuf, &recvBuf, bufsize, bufsize, *activeComm))
    {
      prestaDebug ("active Process in runAllreduceTest\n");

      MPI_Type_size (tp->dataType, &dtSize);
      /*  Ensure communication paths have been initialized  */
      MPI_Allreduce (sendBuf, recvBuf, bufsize / dtSize, tp->dataType, tp->op,
		     *activeComm);

      generic_barrier (*activeComm);
      generic_barrier (*activeComm);

      start = MPI_Wtime ();

      for (i = 0; i < iters; i++)
	MPI_Allreduce (sendBuf, recvBuf, bufsize / dtSize, tp->dataType,
		       tp->op, *activeComm);

      if (argStruct.useBarrier)
	generic_barrier (*activeComm);

      diff = MPI_Wtime () - start;

      free (sendBuf);
      free (recvBuf);
    }

  MPI_Barrier (MPI_COMM_WORLD);

  return diff;
}


double
runBarrierTest (int procs, int bufsize, int iters, MPI_Comm * activeComm)
{
  int i;
  double diff = 0.0;
  double start = 0.0;

  prestaDebug ("in runBarrierTest\n");

  if (isActiveProc (activeComm))
    {
      prestaDebug ("active Process in runBarrierTest\n");

      /*  Ensure communication paths have been initialized  */
      MPI_Barrier (*activeComm);

      generic_barrier (*activeComm);
      generic_barrier (*activeComm);

      start = MPI_Wtime ();

      for (i = 0; i < iters; i++)
	MPI_Barrier (*activeComm);

      if (argStruct.useBarrier)
	generic_barrier (*activeComm);

      diff = MPI_Wtime () - start;

    }

  MPI_Barrier (MPI_COMM_WORLD);

  return diff;
}


void
printUse (void)
{
  printf ("\n%s : MPI collective operation benchmark\n\n",
	  executableName);
  printf ("  syntax: %s [OPTION]...\n\n", executableName);
  printf ("    -b [message start size]                        default=%d\n",
	  MESS_START_DEF);
  printf ("    -e [message stop  size]                        default=%d\n",
	  MESS_STOP_DEF);
  printf ("    -f [process count source file]\n");
  printf ("    -m [message size source file]\n");
  printf ("    -o [number of operations between measurements]\n");
  printf ("    -w '[test list]'\n");
  printf ("    -h : print use information\n");
  printf ("    -l : print hostname information\n");
  printf ("    -n : do not use barrier within measurement     default=%s\n",
	  USE_BARRIER_DEF ? "barrier used" : "barrier not used");
  printf ("    -q : print test names\n");
  printf ("    -r : partner processes with nearby rank        default=%s\n",
	  NEAREST_RANK_DEF ? "true" : "false");
  printf ("    -v : print individual rank times               default=%s\n",
	  NEAREST_RANK_DEF ? "true" : "false");
  printf ("\n");
}


void
printParameters ()
{
  printf ("\nTest Parameters\n");
  printf
    ("\n#######################################################################################################\n");

  if (argStruct.procFile)
    printf ("Process count file                   : %s\n",
	    argStruct.procFile);

  if (argStruct.procsPerNode > 0)
    printf ("Processes per SMP                    : %8d\n",
	    argStruct.procsPerNode);

  if (targetFile != NULL)
    printf ("Process group sourcefile             : %s\n", targetFile);

  if (!argStruct.useBarrier)
    printf ("\nBarrier not included in measurement.\n");

  printf ("\n");
}


int
setupTestListParams ()
{
  if (wsize % 2 != 0)
    wsize--;

  if (argStruct.procFile != NULL)
    {
      if (!getList (validateProcCount, 2, wsize, &argStruct.procListSize,
		    argStruct.procFile, &argStruct.procList))
	{
	  prestaAbort ("Failed to open process count source file %s\n",
		       argStruct.procFile);
	  return 0;
	}
    }
  else
    {
      if (argStruct.allocPattern == 'c')
	{
	  if (!createSeqIntArray (2, wsize, wsize / argStruct.procsPerNode,
				  &argStruct.procList,
				  &argStruct.procListSize))
	    {
	      return 0;
	    }
	}
      else
	{
	  if (!createSeqIntArray (2, wsize, argStruct.messFactor,
				  &argStruct.procList,
				  &argStruct.procListSize))
	    {
	      return 0;
	    }
	}
    }

  if (argStruct.messFile != NULL)
    {
      if (!get2entryList (validateMessageSize, MIN_MESS_SIZE, MAX_MESS_SIZE,
			  &argStruct.messListSize, argStruct.messFile,
			  &argStruct.messList, &argStruct.iterList))
	{
	  prestaAbort ("Failed to open message size source file %s\n",
		       argStruct.messFile);
	  return 0;
	}
    }
  else
    {
      if (!createSeqIntArray (argStruct.messStart, argStruct.messStop,
			      argStruct.messFactor, &argStruct.messList,
			      &argStruct.messListSize))
	{
	  return 0;
	}
    }

  if (targetFile != NULL && !getTargetList ())
    {
      prestaAbort ("Failed to open target list source file %s\n", targetFile);
      return 0;
    }

  return 1;
}


int
initAllTestTypeParams (TESTPARAMS ** testParams)
{
  TESTPARAMS *ptp;
  int *latenList, idx;

  latenList = (int *) malloc (sizeof (int));

  *testParams = (TESTPARAMS *) malloc (sizeof (TESTPARAMS) * TYPETOT);

  if (latenList == NULL || *testParams == NULL)
    {
      fprintf (stderr, "Failed to allocate test parameters\n");
      return 0;
    }


  /*
   * To insert a new test, copy the appropriate TESTPARAMS initializations
   * and add a testname/index to the test enum
   */


  ptp = &(*testParams)[REDUCE_DBL_SUM_RX];

  ptp->name = "Reduce:Double-SUM-RX";
  ptp->desc = "MPI_Reduce, MPI_DOUBLE, MPI_SUM, Root cycles";
  ptp->iters = argStruct.iters;
  ptp->sendFactor = .5;
  ptp->messList = argStruct.messList;
  ptp->messListSize = argStruct.messListSize;
  ptp->testFunc = runReduceTest;
  ptp->dataType = MPI_DOUBLE;
  ptp->op = MPI_SUM;
  ptp->changeRoot = 1;

  ptp = &(*testParams)[REDUCE_DBL_SUM_R0];

  ptp->name = "Reduce:Double-SUM-R0";
  ptp->desc = "MPI_Reduce, MPI_DOUBLE, MPI_SUM, Root is rank 0";
  ptp->iters = argStruct.iters;
  ptp->sendFactor = .5;
  ptp->messList = argStruct.messList;
  ptp->messListSize = argStruct.messListSize;
  ptp->testFunc = runReduceTest;
  ptp->dataType = MPI_DOUBLE;
  ptp->op = MPI_SUM;
  ptp->changeRoot = 0;

  ptp = &(*testParams)[REDUCE_FLT_SUM_R0];

  ptp->name = "Reduce:Float-SUM-R0";
  ptp->desc = "MPI_Reduce, MPI_FLOAT, MPI_SUM, Root is rank 0";
  ptp->iters = argStruct.iters;
  ptp->sendFactor = .5;
  ptp->messList = argStruct.messList;
  ptp->messListSize = argStruct.messListSize;
  ptp->testFunc = runReduceTest;
  ptp->dataType = MPI_FLOAT;
  ptp->op = MPI_SUM;
  ptp->changeRoot = 0;

  ptp = &(*testParams)[REDUCE_FLT_SUM_RX];

  ptp->name = "Reduce:Float-SUM-RX";
  ptp->desc = "MPI_Reduce, MPI_FLOAT, MPI_SUM, Root cycles";
  ptp->iters = argStruct.iters;
  ptp->sendFactor = .5;
  ptp->messList = argStruct.messList;
  ptp->messListSize = argStruct.messListSize;
  ptp->testFunc = runReduceTest;
  ptp->dataType = MPI_FLOAT;
  ptp->op = MPI_SUM;
  ptp->changeRoot = 1;

  ptp = &(*testParams)[BCAST];

  ptp->name = "Broadcast:Double";
  ptp->desc = "MPI_Bcast, MPI_DOUBLE, Root is rank 0";
  ptp->iters = argStruct.iters;
  ptp->sendFactor = .5;
  ptp->messList = argStruct.messList;
  ptp->messListSize = argStruct.messListSize;
  ptp->testFunc = runBcastTest;
  ptp->dataType = MPI_DOUBLE;

  ptp = &(*testParams)[REDUCEBCAST];

  ptp->name = "Reduce-Bcast:Double-MIN";
  ptp->desc = "MPI_Reduce/MPI_Bcast, MPI_DOUBLE, MPI_MIN, Root is rank 0";
  ptp->iters = argStruct.iters;
  ptp->sendFactor = .5;
  ptp->messList = argStruct.messList;
  ptp->messListSize = argStruct.messListSize;
  ptp->testFunc = runReduceBcastTest;
  ptp->dataType = MPI_DOUBLE;
  ptp->op = MPI_MIN;

  ptp = &(*testParams)[ALLREDUCE];

  ptp->name = "Allreduce:Double-MIN";
  ptp->desc = "MPI_Allreduce, MPI_DOUBLE, MPI_MIN";
  ptp->iters = argStruct.iters;
  ptp->sendFactor = .5;
  ptp->messList = argStruct.messList;
  ptp->messListSize = argStruct.messListSize;
  ptp->testFunc = runAllreduceTest;
  ptp->dataType = MPI_DOUBLE;
  ptp->op = MPI_MIN;

  ptp = &(*testParams)[BARRIER];

  ptp->name = "Barrier";
  ptp->desc = "MPI_Barrier";
  ptp->iters = argStruct.iters;
  ptp->sendFactor = .5;
  ptp->messList = &zeroArray;
  ptp->messListSize = 1;
  ptp->testFunc = runBarrierTest;
  ptp->test = BARRIER;

  for (idx = 0; idx < TYPETOT; idx++)
    {
      ptp = &(*testParams)[idx];

      ptp->maxBW = 0;
      ptp->maxBWMessSize = 0;

      ptp->rankResult = 0.0;
      ptp->maxResult = 0.0;
    }

  return 1;
}


void
printTestNames (void)
{
  int testIdx;

  prestaRankPrint (0, "\nTest Names\n");

  for (testIdx = 0; testIdx < TYPETOT; testIdx++)
    prestaRankPrint (0, "  %-25s : %s\n", (&testParams[testIdx])->name,
		     (&testParams[testIdx])->desc);

  prestaRankPrint (0, "\n");
}


void
freeBuffers (TESTPARAMS ** testParams)
{
  free (argStruct.procList);
  free (*testParams);
}
