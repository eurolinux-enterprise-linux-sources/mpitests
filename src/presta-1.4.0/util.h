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


    util.h

*/

#ifndef _UTIL_H
#define _UTIL_H

#include <limits.h>

#define MESS_START_DEF 32
#define MESS_STOP_DEF 8388608
#define MESS_FACTOR_DEF 2
#define ALLOC_DEF 'b'
#define USE_BARRIER_DEF 0
#define PRINT_PAIRS_DEF 0
#define NEAREST_RANK_DEF 0
#define MIN_MESS_SIZE 0
#define MAX_MESS_SIZE INT_MAX
#define MB_SIZE 1000000
#define DEFAULT_ITERATIONS 10

typedef struct
{

  int debugFlag;
  int eiters;
  int iters;
  int messFactor;
  int messStart;
  int messStop;
  int printHostInfo;
  int printPairs;
  int printRankTime;
  int procsPerNode;
  int useBarrier;
  int useNearestRank;
  char *procFile;
  char *messFile;
  char allocPattern;
  int *procList;
  int procListSize;
  int *messList;
  int messListSize;
  int *iterList;
  char *testList;
  int sumLocalBW;

} ARGSTRUCT;

extern int rank, wsize;
extern char *targetFile;
extern int *targetArray;
extern int targetListSize;
extern char *targetDirectory;
extern ARGSTRUCT argStruct;
extern int majorVersion;
extern int minorVersion;
extern int patchVersion;
extern char procSrcTitle[256];


double getWtimeOh (void);
void populateData (char *, int);
void generic_barrier (MPI_Comm);
void printReportFooter (double mintime, int procsPerNode,
			int useNearestRanks);
void printCommTargets (int procsPerNode, int useNearestRank);
void listRankLocations (void);
int createActiveComm (int procs, int procsPerNode,
		      char allocPattern, int useNearestRank,
		      MPI_Comm * activeComm);
int isActiveProc (MPI_Comm * currCom);
int processArgs (int argc, char **argv);
void printActivePairs (int procs, int procsPerNode,
		       char allocPattern, int useNearestRank);
int getTargetRank (int rank, int procsPerNode, int useNearestRank);
void getPair (int idx, int procsPerNode,
	      char allocPattern, int useNearestRank, int *rank1, int *rank2);
int validateProcCount (int count, int minCount, int maxCount);
int validateMessageSize (int messSize, int minSize, int maxSize);
int validateTarget (int count, int minCount, int maxCount);
int getProcessList (int minVal, int maxVal,
		    int *listSize, char *listFile, int **list);
int getMessageList (int minVal, int maxVal,
		    int *listSize, char *listFile, int **list);
int getList (int (*validateFunc) (int, int, int), int minVal, int maxVal,
	     int *listSize, char *listFile, int **list);
int get2entryList (int (*validateFunc) (int, int, int), int minVal,
		   int maxVal, int *listSize, char *listFile, int **list1,
		   int **list2);
int createSeqIntArray (int start, int stop, int factor, int **array,
		       int *arraySize);
void prestaDebug (char *fmt, ...);
void prestaRankDebug (int targRank, char *fmt, ...);
void prestaRankPrint (int targRank, char *fmt, ...);
void prestaWarn (char *fmt, ...);
void prestaAbort (char *fmt, ...);
int getNextTargetFile (void);
int numcmp (const void *num1, const void *num2);
int validateRank (int vrank, int minRank, int maxRank);
int getTargetList (void);
void printGlobGroup (MPI_Comm currComm);
char *getTimeStr (void);
#endif
