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


    util.c

    Provides functions common to benchmarks.

*/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <time.h>
#include <mpi.h>
#include "util.h"

#define GENERIC_BARRIER_TAG 1
#define FILE_DELIM " "

char *targetFile = NULL;
int *targetList = NULL;
int targetListSize = 0;
char *targetDirectory = NULL;
static DIR *dp = NULL;

int majorVersion = 1;
int minorVersion = 4;
int patchVersion = 0;

ARGSTRUCT argStruct;

/*  Global variables for MPI_COMM_WORLD rank and size  */
int rank, wsize;

char procSrcTitle[256] = "";

extern void printTestNames (void);


/*
    If the Wtick granularity for Wtime is coarser than the
    overhead of calling MPI_Wtime, then approximate the MPI_Wtime
    overhead by counting calls within one tick.
*/

double
getWtimeOh ()
{
  double start, stop, diff;
  int count;

  count = 1;
  start = MPI_Wtime ();
  stop = MPI_Wtime ();
  diff = stop - start;

  if (diff == 0 || MPI_Wtime () == stop)
    {
      stop = MPI_Wtime ();
      while ((start = MPI_Wtime ()) == stop);
      while ((stop = MPI_Wtime ()) == start)
	count++;
      return (stop - start) / count;
    }

  return stop - start;
}


/* This barrier implementation is taken from the Sphinx MPI benchmarks
   and is based directly on the MPICH barrier implementation...        */

void
generic_barrier (MPI_Comm curcomm)
{
  int rank, size, N2_prev, surfeit;
  int d, dst, src, temp;
  MPI_Status status;

  /* Intialize communicator size */
  (void) MPI_Comm_size (curcomm, &size);

  /* If there's only one member, this is trivial */
  if (size > 1)
    {

      MPI_Comm_rank (curcomm, &rank);

      for (temp = size, N2_prev = 1;
	   temp > 1; temp >>= 1, N2_prev <<= 1) /* NULL */ ;

      surfeit = size - N2_prev;

      /* Perform a combine-like operation */
      if (rank < N2_prev)
	{
	  if (rank < surfeit)
	    {

	      /* get the fanin letter from the upper "half" process: */
	      dst = N2_prev + rank;

	      MPI_Recv ((void *) 0, 0, MPI_INT, dst,
			GENERIC_BARRIER_TAG, curcomm, &status);
	    }

	  /* combine on embedded N2_prev power-of-two processes */
	  for (d = 1; d < N2_prev; d <<= 1)
	    {
	      dst = (rank ^ d);

	      MPI_Sendrecv ((void *) 0, 0, MPI_INT, dst,
			    GENERIC_BARRIER_TAG, (void *) 0, 0, MPI_INT,
			    dst, GENERIC_BARRIER_TAG, curcomm, &status);
	    }

	  /* fanout data to nodes above N2_prev... */
	  if (rank < surfeit)
	    {
	      dst = N2_prev + rank;
	      MPI_Send ((void *) 0, 0, MPI_INT, dst, GENERIC_BARRIER_TAG,
			curcomm);
	    }
	}
      else
	{
	  /* fanin data to power of 2 subset */
	  src = rank - N2_prev;
	  MPI_Sendrecv ((void *) 0, 0, MPI_INT, src, GENERIC_BARRIER_TAG,
			(void *) 0, 0, MPI_INT, src, GENERIC_BARRIER_TAG,
			curcomm, &status);
	}
    }
  return;
}


void
printCommTargets (int procsPerNode, int useNearestRank)
{
  /*  The rationale behind this implementation is to keep memory 
   *  requirements down to only use 3x MPI_MAX_PROCESSOR_NAME rather than 
   *  wsize*MPI_MAX_PROCESSOR_NAME  
   */
  char myProcName[MPI_MAX_PROCESSOR_NAME];
  char processProcName[MPI_MAX_PROCESSOR_NAME];
  char targetProcName[MPI_MAX_PROCESSOR_NAME];
  int nameSize, currtarg, mytarg, i;
  MPI_Status status;

  MPI_Get_processor_name (myProcName, &nameSize);

  if (rank != 0)
    {
      currtarg = getTargetRank (rank, procsPerNode, useNearestRank);

      /* Send target and hostname twice.  As process 0 loops through
       * ranks, it recieves the current rank hostname as well as the target
       * rank information.
       */
      MPI_Send (&currtarg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      MPI_Send (myProcName, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
		0, 0, MPI_COMM_WORLD);
      if (currtarg >= 0)
	{

	  MPI_Send (&currtarg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	  MPI_Send (myProcName, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
		    0, 0, MPI_COMM_WORLD);
	}
    }
  else
    {
      putchar ('\n');

      mytarg = getTargetRank (0, procsPerNode, useNearestRank);

      /*  Receive the target process target rank  */
      MPI_Recv (&currtarg, 1, MPI_INT, mytarg, 0, MPI_COMM_WORLD, &status);
      /*  Receive the current process hostname  */
      if (currtarg >= 0)
	{
	  MPI_Recv (targetProcName, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, mytarg,
		    0, MPI_COMM_WORLD, &status);
	}
      printf ("Rank id %4d (%s) communicated with rank id %4d (%s)\n",
	      0, myProcName, mytarg, targetProcName);

      for (i = 1; i < wsize; i++)
	{
	  /*  Receive the current process target rank  */
	  MPI_Recv (&currtarg, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
	  /*  Receive the current process hostname  */
	  MPI_Recv (processProcName, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, i,
		    0, MPI_COMM_WORLD, &status);

	  if (currtarg != 0)
	    {
	      if (currtarg >= 0)
		{
		  /*  Receive the target process target rank  */
		  MPI_Recv (&mytarg, 1, MPI_INT, currtarg, 0, MPI_COMM_WORLD,
			    &status);
		  /*  Receive the current process hostname  */
		  MPI_Recv (targetProcName, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
			    currtarg, 0, MPI_COMM_WORLD, &status);
		}
	    }
	  else
	    strcpy (targetProcName, myProcName);

	  if (currtarg < 0)
	    strcpy (targetProcName, "Not Used");

	  printf ("Rank id %4d (%s) communicated with rank id %4d (%s)\n",
		  i, processProcName, currtarg, targetProcName);
	}
      putchar ('\n');
    }
}



void
printGlobGroup (MPI_Comm currComm)
{
  char myProcName[MPI_MAX_PROCESSOR_NAME];
  char processProcName[MPI_MAX_PROCESSOR_NAME];
  int nameSize, i, commSize;
  MPI_Status status;

  MPI_Get_processor_name (myProcName, &nameSize);

  if (rank != 0)
    {
      if (isActiveProc (&currComm))
	MPI_Send (myProcName, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
		  0, 0, currComm);
    }
  else
    {
      putchar ('\n');

      printf ("Active Process Ranks\n");
      printf ("Rank id %4d (%s)\n", 0, myProcName);

      MPI_Comm_size (currComm, &commSize);
      for (i = 1; i < commSize; i++)
	{
	  /*  Receive the current process hostname  */
	  MPI_Recv (processProcName, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, i,
		    0, currComm, &status);

	  printf ("Rank id %4d (%s) \n", i, processProcName);
	}
      putchar ('\n');
    }
}



void
listRankLocations ()
{
  /* The rationale behind this implementation is to keep memory 
   * requirements down to only use 2x MPI_MAX_PROCESSOR_NAME rather than 
   * wsize*MPI_MAX_PROCESSOR_NAME  
   * 
   * This is used for the global operations where we don't have the concept of
   * process pairs.
   */
  char procNameBuf[MPI_MAX_PROCESSOR_NAME];
  int nameSize, i;
  MPI_Status status;

  MPI_Get_processor_name (procNameBuf, &nameSize);

  if (rank != 0)
    {
      MPI_Send (procNameBuf, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
		0, 0, MPI_COMM_WORLD);
    }
  else
    {
      putchar ('\n');

      printf ("Rank id %6d ran on %s\n", 0, procNameBuf);

      for (i = 1; i < wsize; i++)
	{
	  /*  Receive the current process hostname  */
	  MPI_Recv (procNameBuf, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, i, 0,
		    MPI_COMM_WORLD, &status);

	  printf ("Rank id %6d ran on %s\n", i, procNameBuf);
	}
      putchar ('\n');
    }
}


void
printReportFooter (double mintime, int procsPerNode, int useNearestRank)
{
  double wtick, wtoverhead;

  if (argStruct.printHostInfo)
    printCommTargets (procsPerNode, useNearestRank);

  if (rank == 0)
    {
      if (argStruct.sumLocalBW == 1)
	printf ("Bandwidth calculated as sum of process bandwidths.\n");
      else if (argStruct.sumLocalBW == 0)
	printf
	  ("Bandwidth calculated from total volume / longest task communication.\n");

      printf ("\n");

      wtick = MPI_Wtick ();
      wtoverhead = getWtimeOh ();

      printf ("MPI_Wtick returns           %11.9f\n", wtick);
      printf ("MPI_Wtime overhead          %11.9f\n", wtoverhead);
      if (argStruct.sumLocalBW == 0)
	printf ("Ticks for minimum sample    %11.0f\n", mintime / wtick);


    }
}


void
getPair (int idx, int procsPerNode,
	 char allocPattern, int useNearestRank, int *rank1, int *rank2)
{
  if (allocPattern == 'c' && procsPerNode > 0 && wsize > 2 * procsPerNode)
    {

      if (useNearestRank)
	{
	  int calcval;

	  calcval = idx * procsPerNode * 2;
	  *rank1 = calcval % wsize + calcval / wsize;
	}
      else
	{
	  int calcval;

	  calcval = idx * procsPerNode;
	  *rank1 = calcval % (wsize / 2) + calcval / (wsize / 2);
	}

    }
  else
    {
      if (useNearestRank)
	{
	  if (procsPerNode < 1)
	    {
	      prestaWarn
		("processes per node is not defined, reverting to default process allocation\n");
	      *rank1 = idx;
	    }
	  else
	    *rank1 = idx + (idx / procsPerNode * procsPerNode);
	}
      else
	*rank1 = idx;
    }

  *rank2 = getTargetRank (*rank1, procsPerNode, useNearestRank);
}


int
getTargetRank (int rank, int procsPerNode, int useNearestRank)
{
  int target = -1;

  if (targetList != NULL)
    {
      target = targetList[rank];
    }
  else if (useNearestRank && procsPerNode > 0)
    {
      if (rank % (procsPerNode * 2) >= procsPerNode)
	target = rank - procsPerNode;
      else
	target = rank + procsPerNode;
    }
  else
    {
      if (rank < wsize / 2)
	target = rank + wsize / 2;
      else
	target = rank - wsize / 2;
    }

  return target;
}


int
numcmp (const void *num1, const void *num2)
{
  return *(int *) num1 - *(int *) num2;
}


int
createActiveComm (int procs, int procsPerNode,
		  char allocPattern, int useNearestRank,
		  MPI_Comm * activeComm)
{
  int *activeIds;
  int i;
  MPI_Group worldGroup, activeGroup;
  char *procFlags = NULL;
  int taskID1, taskID2;
  int idCnt = 0;

  MPI_Comm_group (MPI_COMM_WORLD, &worldGroup);

  /*  Create Communicator of all active processes  */
  activeIds = (int *) malloc (procs * sizeof (int));
  procFlags = (char *) malloc (wsize * sizeof (char));

  memset (procFlags, 0, wsize * sizeof (char));

  i = 0;
  while (idCnt < procs && i < procs)
    {
      if (allocPattern == 'l')	/* linear pattern used for collective tests  */
	{
	  activeIds[i] = i;
	  i++;
	  idCnt++;
	}
      else
	{
	  getPair (i, procsPerNode, allocPattern, useNearestRank,
		   &taskID1, &taskID2);


	  prestaDebug ("i:%d  wsize:%d  procsPerNode:%d\n", i, wsize,
		       procsPerNode);
	  prestaDebug ("taskID1: %d   taskID2:  %d\n", taskID1, taskID2);

	  if (taskID1 > -1 && taskID2 > -1)
	    {
	      if (procFlags[taskID1] == 0)
		{
		  activeIds[idCnt++] = taskID1;
		  procFlags[taskID1] = 1;
		}

	      if (procFlags[taskID2] == 0)
		{
		  activeIds[idCnt++] = taskID2;
		  procFlags[taskID2] = 1;
		}
	    }
	  i++;
	}
    }

  if (idCnt % 2 != 0)
    prestaAbort ("Total number of processes is invalid!\n");

  qsort (activeIds, idCnt, sizeof (int), numcmp);

  MPI_Group_incl (worldGroup, idCnt, activeIds, &activeGroup);
  MPI_Comm_create (MPI_COMM_WORLD, activeGroup, activeComm);

  free (activeIds);
  free (procFlags);

  MPI_Group_free (&worldGroup);
  MPI_Group_free (&activeGroup);

  return idCnt;
}


void
printActivePairs (int procs, int procsPerNode, char allocPattern,
		  int useNearestRank)
{
  int i, cp1, cp2;

  if (rank == 0)
    {
      if (allocPattern == 'c' && procsPerNode > 0 && wsize > procsPerNode * 2)
	printf ("\nCurrent pairs, cyclic allocation\n");
      else
	printf ("\nCurrent pairs, block allocation\n");

      for (i = 0; i < procs / 2; i++)
	{
	  getPair (i, procsPerNode, allocPattern, useNearestRank, &cp1, &cp2);

	  printf ("  %5d:%5d\n", cp1, cp2);
	}
      printf ("\n");
    }
}


int
isActiveProc (MPI_Comm * currComm)
{
  MPI_Group tempGroup;
  int grank;

  if (currComm == NULL)
    prestaAbort ("currComm is NULL in isActiveProc");

  if (*currComm == MPI_COMM_NULL)
    return 0;

  MPI_Comm_group (*currComm, &tempGroup);

  MPI_Group_rank (tempGroup, &grank);

  MPI_Group_free (&tempGroup);

  if (grank == MPI_UNDEFINED)
    return 0;
  else
    return 1;
}


int
processArgs (int argc, char **argv)
{
  int flag;
  int dogetopt = 1;

  argStruct.iters = 0;
  argStruct.iterList = NULL;
  argStruct.eiters = 0;
  argStruct.debugFlag = 0;
  argStruct.messStart = MESS_START_DEF;
  argStruct.messStop = MESS_STOP_DEF;
  argStruct.messFactor = MESS_FACTOR_DEF;
  argStruct.procFile = NULL;
  argStruct.messFile = NULL;
  argStruct.printHostInfo = 0;
  argStruct.procsPerNode = 0;
  argStruct.allocPattern = ALLOC_DEF;
  argStruct.useBarrier = USE_BARRIER_DEF;
  argStruct.printPairs = PRINT_PAIRS_DEF;
  argStruct.useNearestRank = NEAREST_RANK_DEF;
  argStruct.printRankTime = 0;
  argStruct.sumLocalBW = 1;

  opterr = 0;
  while ((flag =
	  getopt (argc, argv, "b:c:d:e:f:ghilm:no:p:qrs:t:vw:xz")) != -1
	 && dogetopt == 1)
    {
      /*  Flags without arguments  */

      switch (flag)
	{
	case 'g':
	  argStruct.debugFlag = 1;
	  break;
	case 'h':
	  return (0);
	  break;
	case 'i':
	  argStruct.printPairs = 1;
	  break;
	case 'l':
	  argStruct.printHostInfo = 1;
	  break;
	case 'n':
	  argStruct.useBarrier = 1;
	  break;
	case 'q':
	  printTestNames ();
	  MPI_Finalize ();
	  exit (0);
	  break;
	case 'r':
	  argStruct.useNearestRank = 1;
	  break;
	case 'v':
	  argStruct.printRankTime = 1;
	  break;
	case 'x':
	  argStruct.sumLocalBW = 0;
	  break;

	  /*  Flags with arguments  */
	case 'b':
	  argStruct.messStart = atoi (optarg);
	  break;
	case 'c':
	  argStruct.eiters = atoi (optarg);
	  break;
	case 'd':
	  targetFile = strdup (optarg);
	  break;
	case 'e':
	  argStruct.messStop = atoi (optarg);
	  break;
	case 'f':
	  argStruct.procFile = strdup (optarg);
	  break;
	case 'm':
	  argStruct.messFile = strdup (optarg);
	  break;
	case 'o':
	  argStruct.iters = atoi (optarg);
	  break;
	case 'p':
	  argStruct.allocPattern = *optarg;
	  break;
	case 'w':
	  if (strlen (optarg) == 0)
	    {
	      printTestNames ();
	      exit (-1);
	    }
	  argStruct.testList = strdup (optarg);
	  break;
	case 's':
	  targetDirectory = strdup (optarg);
	  break;
	case 't':
	  argStruct.procsPerNode = atoi (optarg);
	  break;
	default:
	  if (rank == 0)
	    printf ("\nInvalid option -%c\n", optopt);
	  dogetopt = 0;
	  break;
	}
    }

  if (argStruct.iters == 0)
    {
      argStruct.iters = DEFAULT_ITERATIONS;
    }

  if (argStruct.allocPattern == 'c' && argStruct.procsPerNode == 0)
    {
      if (rank == 0)
	printf
	  ("\n  Cyclic allocation requires a process per node argument!\n\n");
      return (0);
    }

  if (argStruct.allocPattern == 'c' && wsize <= argStruct.procsPerNode * 2)
    {
      if (rank == 0)
	{
	  printf ("\n  Cyclic allocation can only be used for jobs\n");
	  printf ("    with MPI process count > 2*processes/node.\n\n");

	}
      return (0);
    }

  return (1);
}


int
validateProcCount (int count, int minCount, int maxCount)
{
  int retval = 0;

  if (count > 1 && count % 2 == 0 && count >= minCount && count <= maxCount)
    {
      retval = 1;
    }

  return retval;
}


int
validateRank (int vrank, int minRank, int maxRank)
{
  int retval = 0;

  if ((vrank >= minRank && vrank <= maxRank) || vrank == -1)
    {
      retval = 1;
    }

  return retval;
}


int
validateMessageSize (int messSize, int minSize, int maxSize)
{
  int retval = 0;

  if (messSize >= minSize && messSize <= maxSize)
    {
      retval = 1;
    }

  return retval;
}


int
getProcessList (int minVal, int maxVal,
		int *listSize, char *listFile, int **list)
{
  return getList (validateProcCount, minVal, maxVal, listSize, listFile,
		  list);
}


int
getTargetList (void)
{
  return getList (validateRank, 0, wsize, &targetListSize, targetFile,
		  &targetList);
}


int
getMessageList (int minVal, int maxVal,
		int *listSize, char *listFile, int **list)
{
  return getList (validateMessageSize, minVal, maxVal,
		  listSize, listFile, list);
}


/*
 * getList - process rank 0 reads a file into an array and broadcasts the entire
 *           array to all processes.
 * 
 * in :
 *      validateFunc   : function to validate input values
 *      minVal, maxVal : values to pass the validate function
 *            listFile : the name of the file to read
 * out :
 *      listSize, list : pointers to the resulting list and list size
 */

int
getList (int (*validateFunc) (int, int, int), int minVal, int maxVal,
	 int *listSize, char *listFile, int **list)
{
  FILE *fp = NULL;
  char lineBuf[1024];
  int currVal, retval = 1, arval;

  *listSize = 0;

  if (rank == 0)
    {
      if (listFile != NULL)
	{
	  if ((fp = fopen (listFile, "r")) != NULL)
	    {
	      *list = (int *) malloc (1024 * sizeof (int));
	      if (*list == NULL)
		{
		  fclose (fp);
		  retval = 0;
		}

	      while (retval == 1 && fgets (lineBuf, 1024, fp) != NULL)
		{
		  if (lineBuf[0] == '#')
		    {
		      if (strstr (lineBuf, "Title") != NULL)
			strcpy (procSrcTitle, &lineBuf[24]);
		      continue;
		    }

		  currVal = atoi (lineBuf);
		  prestaDebug ("currVal is %d\n", currVal);
		  if (validateFunc (currVal, minVal, maxVal))
		    {
		      (*list)[*listSize] = currVal;
		      prestaDebug ("list[%d] is %d\n", *listSize,
				   (*list)[*listSize]);
		      (*listSize)++;
		      if (*listSize % 1024 == 0)
			{
			  *list =
			    (int *) realloc (*list,
					     (*listSize / 1024 +
					      1) * 1024 * sizeof (int));

			  if (*list == NULL)
			    {
			      fclose (fp);
			      retval = 0;
			    }
			}
		    }
		}

	      fclose (fp);
	    }
	  else
	    {
	      retval = 0;
	    }
	}
      else
	retval = 0;
    }

  MPI_Bcast (listSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
  if (rank != 0)
    {
      *list = (int *) malloc (*listSize * sizeof (int));
      if (*list == NULL)
	retval = 0;
    }

  MPI_Allreduce (&retval, &arval, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
  retval = arval;

  if (retval != 0)
    MPI_Bcast (*list, *listSize, MPI_INT, 0, MPI_COMM_WORLD);

  return retval;
}


int
get2entryList (int (*validateFunc) (int, int, int), int minVal,
	       int maxVal, int *listSize, char *listFile, int **list1,
	       int **list2)
{
  FILE *fp = NULL;
  char lineBuf[1024];
  int currVal;
  char *pVal;

  *listSize = 0;

  if (listFile != NULL)
    {
      if ((fp = fopen (listFile, "r")) != NULL)
	{
	  *list1 = (int *) malloc (1024 * sizeof (int));
	  if (*list1 == NULL)
	    {
	      fclose (fp);
	      return 0;
	    }

	  *list2 = (int *) malloc (1024 * sizeof (int));
	  if (*list2 == NULL)
	    {
	      fclose (fp);
	      return 0;
	    }

	  while (fgets (lineBuf, 1024, fp) != NULL)
	    {
	      if (lineBuf[0] == '#')
		continue;

	      pVal = strtok (lineBuf, FILE_DELIM);
	      if (pVal)
		currVal = atoi (pVal);
	      else
		currVal = 0;

	      if (validateFunc (currVal, minVal, maxVal))
		{
		  (*list1)[*listSize] = currVal;
		  if (*listSize % 1024 == 0)
		    {
		      *list1 =
			(int *) realloc (*list1,
					 (*listSize / 1024 +
					  1) * 1024 * sizeof (int));

		      if (*list1 == NULL)
			{
			  fclose (fp);
			  return 0;
			}
		    }
		}

	      pVal = strtok (NULL, FILE_DELIM);
	      if (pVal)
		currVal = atoi (pVal);
	      else
		currVal = 0;

	      (*list2)[*listSize] = currVal;

	      if (*listSize % 1024 == 0)
		{
		  *list2 =
		    (int *) realloc (*list2,
				     (*listSize / 1024 +
				      1) * 1024 * sizeof (int));

		  if (*list1 == NULL)
		    {
		      fclose (fp);
		      return 0;
		    }
		}
	      (*listSize)++;
	    }

	  fclose (fp);
	}
      else
	{
	  return 0;
	}
    }
  else
    return 0;

  return 1;
}


int
createSeqIntArray (int start, int stop, int factor, int **array,
		   int *arraySize)
{
  int curSize, seqCount, idx;

  *array = NULL;

  for (curSize = start, seqCount = 0; curSize <= stop; curSize *= factor)
    {
      seqCount++;

      if (curSize == 0)
	curSize = 1;
    }

  *arraySize = seqCount;
  *array = (int *) malloc (seqCount * sizeof (int));

  if (array == NULL)
    {
      fprintf (stderr, "Failed to allocate array.\n");

      return 0;
    }

  for (curSize = start, idx = 0; curSize <= stop; curSize *= factor, idx++)
    {
      (*array)[idx] = curSize;

      if (curSize == 0)
	curSize = 1;
    }

  return 1;
}


void
prestaDebug (char *fmt, ...)
{
  va_list args;
  FILE *fp = stderr;
  extern char *executableName;

  if (argStruct.debugFlag == 1)
    {
      va_start (args, fmt);
      fprintf (fp, "DEBUG [%d] %s: ", rank, executableName);
      vfprintf (fp, fmt, args);
      va_end (args);
      fflush (fp);
    }
}

void
prestaRankDebug (int targRank, char *fmt, ...)
{
  va_list args;
  FILE *fp = stderr;
  extern char *executableName;

  if ((targRank < 0 || rank == targRank) && argStruct.debugFlag == 1)
    {
      va_start (args, fmt);
      fprintf (fp, "DEBUG : %s: ", executableName);
      vfprintf (fp, fmt, args);
      va_end (args);
      fflush (fp);
    }
}


void
prestaRankPrint (int targRank, char *fmt, ...)
{
  va_list args;
  FILE *fp = stdout;

  if ((targRank < 0 || rank == targRank))
    {
      va_start (args, fmt);
      vfprintf (fp, fmt, args);
      va_end (args);
      fflush (fp);
    }
}


void
prestaWarn (char *fmt, ...)
{
  va_list args;
  FILE *fp = stderr;
  extern char *executableName;

  va_start (args, fmt);
  fprintf (fp, "Warning : %s: ", executableName);
  vfprintf (fp, fmt, args);
  va_end (args);
  fflush (fp);
}


void
prestaAbort (char *fmt, ...)
{
  va_list args;
  FILE *fp = stderr;
  extern char *executableName;

  va_start (args, fmt);
  fprintf (fp, "\n");
  fprintf (fp, "Failure : %s: ", executableName);
  vfprintf (fp, fmt, args);
  va_end (args);
  fprintf (fp, "\n");
  fflush (fp);

  /* Not sure why this Barrier is here.  If a single process aborts, then
   * this would seem to keep any non-aborting processes from leaving the 
   * Barrier.

   MPI_Barrier ( MPI_COMM_WORLD );
   */
  MPI_Abort (MPI_COMM_WORLD, 0);
}


/*
 * getNextTargetFile - check the target directory for another target file
 * 
 * Uses global variables:
 *  targetDirectory
 *  targetFile
 */

int
getNextTargetFile ()
{
  static int openedDirectory = 0;
  struct dirent *dent;
  struct stat sbuf;
  int tlen;

  assert (targetDirectory != NULL);

  if (!openedDirectory)
    {
      if ((dp = opendir (targetDirectory)) == NULL)
	{
	  return 0;
	}
      openedDirectory = 1;
      prestaRankDebug (0, "opened directory %s\n", targetDirectory);
    }

  while ((dent = readdir (dp)) != NULL
	 && stat (dent->d_name, &sbuf) == 0 && !S_ISREG (sbuf.st_rdev))
    {
      prestaRankDebug (0, "found file %s\n", dent->d_name);
    }

  if (dent != NULL)
    {
      tlen = strlen (targetDirectory) + strlen (dent->d_name) + 2;
      if (targetFile)
	free (targetFile);
      targetFile = malloc (tlen * sizeof (char));
      strcpy (targetFile, targetDirectory);
      strcat (targetFile, "/");
      strcat (targetFile, dent->d_name);
      prestaRankDebug (0, "found target file %s\n", targetFile);
    }

  if (dent != NULL)
    return 1;
  else
    closedir (dp);

  return 0;
}


char *
getTimeStr (void)
{
  time_t ct;
  static char nowstr[128];
  const struct tm *nowstruct;
  char *fmtstr = "%m/%d/%y  %T";

  time (&ct);
  nowstruct = localtime (&ct);
  strftime (nowstr, 128, fmtstr, nowstruct);
  return strdup (nowstr);
}
