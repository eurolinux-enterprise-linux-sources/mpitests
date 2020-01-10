News in 3.2
===========

Compared to its predecessors, IMB 3.2 has one other default setting:

Run time control

   IMB dynamically computes a repetition count for each experiment so that
   the sum of the resulting repetitions (roughly) does not exceed a given
   run time. The run time for each sample (= 1 message size with repetitions)
   is set in => IMB_settings.h, IMB_settings_io.h (current value: 10 secs,
   preprocessor variable SECS_PER_SAMPLE)

   To override this behavior, use the flag -time

   

Calling sequence (command line will be repeated in Output table!):


IMB-MPI1    [-h{elp}]
            [-npmin     <NPmin>]
            [-multi     <MultiMode>]
            [-off_cache <cache_size[,cache_line_size]>
            [-iter      <msgspersample[,overall_vol[,msgs_nonaggr]]>
            [-time      <max_runtime per sample>]
            [-mem       <max. per process memory for overall message buffers>]
            [-msglen    <Lengths_file>]
            [-map       <PxQ>]
            [-input     <filename>]
            [benchmark1 [,benchmark2 [,...]]]

where 

- h ( or help) just provides basic help 
  (if active, all other arguments are ignored)

- npmin

  the argumaent after npmin is NPmin, 
  the minimum number of processes to run on
  (then if IMB is started on NP processes, the process numbers 
   NPmin, 2*NPmin, ... ,2^k * NPmin < NP, NP are used)
   >>>
   to run on just NP processes, run IMB on NP and select -npmin NP
   <<<
  default: 
  NPmin=2

- off_cache 
 
  the argument after off_cache can be either 1 single number (cache_size),  
  or 2 comma separated numbers (cache_size,cache_line_size), or just -1 
   
  By default, without this flag, the communications buffer is  
  the same within all repetitions of one message size sample;   
  most likely, cache reusage is yielded and thus throughput results   
  that might be non realistic.    
 
  With -off_cache, it is attempted to avoid cache reusage.    
  cache_size is a float for an upper bound of the size of the last level cache in MBytes 
  cache_line_size is assumed to be the size (Bytes) of a last level cache line  
  (can be an upper estimate).  
  The sent/recv'd data are stored in buffers of size ~ 2 x MAX( cache_size, message_size );  
  when repetitively using messages of a particular size, their addresses are advanced within those  
  buffers so that a single message is at least 2 cache lines after the end of the previous message.  
  Only when those buffers have been marched through (eventually), they will re-used from the beginning.  
   
  A cache_size and a cache_line_size are assumed as statically defined    
  in  => IMB_mem_info.h; these are used when -off_cache -1 is entered  
   
  remark: -off_cache is effective for IMB-MPI1, IMB-EXT, but not IMB-IO  
   
  examples  
   -off_cache -1 (use defaults of IMB_mem_info.h);  
   -off_cache 2.5 (2.5 MB last level cache, default line size);  
   -off_cache 16,128 (16 MB last level cache, line size 128);  
   
  NOTE: the off_cache mode might also be influenced by eventual internal  
        caching with the MPI library. This could make the interpretation 
        intricate.  
   
  default: 
  no cache control, data likely to come out of cache most of the time 

- iter 

  the argument after -iter can be 1 single, 2 comma separated, or 3 comma separated 
  integer numbers, which override the defaults 
  MSGSPERSAMPLE, OVERALL_VOL, MSGS_NONAGGR of =>IMB_settings.h 
  examples 
   -iter 2000        (override MSGSPERSAMPLE by value 2000) 
   -iter 1000,100    (override OVERALL_VOL by 100) 
   -iter 1000,40,150 (override MSGS_NONAGGR by 150) 
 
  
  default: 
  iteration control through parameters MSGSPERSAMPLE,OVERALL_VOL,MSGS_NONAGGR => IMB_settings.h 
  
  NOTE: !! New in versions from IMB 3.2 on !!  
        the iter selection is overridden by a dynamic selection that is a new default in 
        IMB 3.2: when a maximum run time (per sample) is expected to be exceeded, the 
        iteration number will be cut down; see -time flag  

- time

  the argument after -time is a float, specifying that 
  a benchmark will run at most that many seconds per message size 
  the combination with the -iter flag or its defaults is so that always 
  the maximum number of repetitions is chosen that fulfills all restrictions 
  example 
   -time 0.150       (a benchmark will (roughly) run at most 150 milli seconds per message size, iff
                      the default (or -iter selected) number of repetitions would take longer than that) 
  
  remark: per sample, the rough number of repetitions to fulfill the -time request 
          is estimated in preparatory runs that use ~ 1 second overhead 
  
  default: 
  A fixed time limit SECS_PER_SAMPLE =>IMB_settings.h; currently set to 10  
  (new default in IMB_3.2) 

- mem

  the argument after -mem is a float, specifying that 
  at most that many GBytes are allocated per process for the message buffers 
  if the size is exceeded, a warning will be output, stating how much memory 
  would have been necessary, but the overall run is not interrupted 
  example 
   -mem 0.2         (restrict memory for message buffers to 200 MBytes per process) 
  
  default: 
  the memory is restricted by MAX_MEM_USAGE => IMB_mem_info.h 

- map

  the argument after -map is PxQ, P,Q are integer numbers with P*Q <= NP
  enter PxQ with the 2 numbers separated by letter "x" and no blancs
  the basic communicator is set up as P by Q process grid

  if, e.g., one runs on N nodes of X processors each, and inserts
  P=X, Q=N, then the numbering of processes is "inter node first"
  running PingPong with P=X, Q=2 would measure inter-node performance
  (assuming MPI default would apply 'normal' mapping, i.e. fill nodes
  first priority) 

  default: 
  Q=1

- multi

  the argument after -multi is MultiMode (0 or 1)

  if -multi is selected, running the N process version of a benchmark
  on NP overall, means running on (NP/N) simultaneous groups of N each.

  MultiMode only controls default (0) or extensive (1) output charts.
  0: only lowest performance groups is output
  1: all groups are output

  default: 
  multi off

- msglen

  the argument after -msglen is a lengths_file, an ASCII file, containing any set of nonnegative
  message lengths, 1 per line

  default: 
  no lengths_file, lengths defined by settings.h, settings_io.h
  
- input

  the argument after -input is a filename is any text file containing, line by line, benchmark names
  facilitates running particular benchmarks as compared to using the
  command line.

  default: 
  no input file exists
  
- benchmarkX is (in arbitrary lower/upper case spelling)

PingPong
PingPing
Sendrecv
Exchange
Bcast
Allgather
Allgatherv
Gather
Gatherv
Scatter
Scatterv
Alltoall
Alltoallv
Reduce
Reduce_scatter
Allreduce
Barrier
