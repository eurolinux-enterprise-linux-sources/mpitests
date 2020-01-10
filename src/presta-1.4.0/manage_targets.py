#!/usr/bin/python -Wall

##########################################################################
#
# manage_targets.py  Chris Chambreau (chcham@llnl.gov)
#
# generate presta com sourcefiles for various communication patterns
#   taking unavailable nodes into consideration.
#
# Parameters:
#   tasks_per_node    :  Default MPI tasks per node
#   node_count        :  Total number of available nodes
#   task_count        :  Total number of tasks in job
#   com_pattern       :  Default communication pattern (Nearest Neighbor)
#   exclude_list      :  Exclude Node numbers
#   min_node          :  Node start id (such as up001)
#   max_node          :  Maximum node number (such as up064)
#
# Script Execution Procedure:
#
#  1)  Map ranks to specific available nodes (node_nap)
#  2)  Map all nodes to target communication pair nodes (node_pair)
#  3)  Iterate through ranks, assigning rank target or spin flag
#        if the corresponding target node is not available (ranklist)
#  4)  Iterate through ranklist, printing requested information
#
##########################################################################


import getopt, sys, random, time

spin_flag = -1
init_flag = -1
excl_flag = -1

debug_flag = False
rand_node_map = {}
rand_node_init_flag = False;


##########################################################################
#
#  Function : debug
#
#  Description : debug utility routine for verbose output
#
##########################################################################

def debug(*args):

  global debug_flag

  if ( debug_flag == True ) :
    fmt = 'DEBUG : '+args[0]
    args = args[1:]
    print fmt % args


##########################################################################
#
#  Function : is_valid_node
#
#  Description : return True/False if node number is valid
#
##########################################################################

def is_valid_node(n):

  global exclude_list, min_node, max_node

  if ( str(n) in exclude_list or n < min_node or n > max_node ) :
    return False
  else:
    return True


##########################################################################
#
#  Function : print_header
#
#  Description : print output header comments
#
##########################################################################

def print_header(name) :

  label_width = 18

  print '#'
  print '#  Ranklist for %s Communication Pattern' % (name)
  print '#'

  print '#  %-*s : %s' % (label_width, 'Title', name)
  print '#  %-*s : %s' % (label_width, 'Total Nodes', node_count)
  print '#  %-*s : %s' % (label_width, 'Total Processes',task_count)
  print '#  %-*s : %s' % (label_width, 'Tasks Per Node',tasks_per_node)
  print '#  %-*s : %s' % (label_width, 'First Node number', min_node)
  print '#  %-*s : %s' % (label_width, 'Last Node number', max_node)
  print '#  %-*s : %s' % (label_width, 'Excluded Nodes', exclude_list)
  print '#  %-*s : %s' % (label_width, 'Generated', time.strftime('%D %r', time.localtime()))
  print '#'



##########################################################################
#
#  Function : print_rank_list
#
#  Description : print node information from ranklist
#
##########################################################################

def print_rank_list():

  global ranklist, exclude_list

  debug('***  In print_rank_list\n')

  for rank in range(0, task_count) :

    #debug('rank %d, ranklist[rank] %d', rank, ranklist[rank])

    if ( ranklist[rank] == spin_flag ):
      print '%-7s    #  rank %6d  <->  %6s          node %6s <-> node %s  **DOWN**' \
        % (spin_flag, rank, 'NONE', node_map[rank], node_pair[node_map[rank]])

    else:
      targNode = node_map[(ranklist[rank])]

      print '%-7d    #  rank %6d  <->  %6d          node %6s <-> node %s' \
        % (ranklist[rank], rank, ranklist[rank], node_map[rank], targNode)
        

##########################################################################
#
#  Node Pair calculation routines
#
#  For a given node number, return the target node number
#
##########################################################################
      
def nn_node_pair(cnode):

    if ( cnode % 2 == 1 ) :
      tnode = cnode + 1
    else:
      tnode = cnode - 1

    return tnode


def nw_ne_node_pair(cnode):

    tnode = (cnode + max_node/2) % max_node
    if ( tnode == 0 ) :
      tnode = max_node

    return tnode


def nw_se_node_pair(cnode):

    #  if cnode in NW or NE Quadrants
    if ( cnode <= max_node/4 \
      or ( cnode > max_node/2 and cnode <= 3*(max_node/4) ) ) :
      
      tnode = (cnode + (max_node/2) + (max_node/4)) % max_node
    else:
      tnode = (cnode + (max_node/4)) % max_node

    if ( tnode == 0 ) :
      tnode = max_node

    return tnode


def nw_sw_node_pair(cnode):

    #  if cnode in NW or NE Quadrants, targnode += max_node/4
    if ( cnode <= max_node/4 
         or ( cnode >= max_node/2 and cnode < max_node/2 + max_node/4)  ) :
      tnode = cnode + max_node/4
    else:
      tnode = cnode - max_node/4

    return tnode


def rand_node_pair(cnode):

  global rand_node_init_flag, node_count

  #  Do random node bookkeeping setup
  if ( rand_node_init_flag == False ) :
    rand_node_init_flag = True

    #  Assign all excluded nodes as partners for efficient use of nodes
    for idx in range(len(exclude_list)/2) :
      debug('idx is %d, exclude_list[idx] is %d, exclude_list[-idx-1] is %d', idx, int(exclude_list[idx]), int(exclude_list[-idx-1]))
      debug('len rand_node_dict is %d', len(rand_node_dict))

      rand_node_dict[int(exclude_list[idx])] = int(exclude_list[-idx-1])
      rand_node_dict[int(exclude_list[-idx-1])] = int(exclude_list[idx])

  if ( rand_node_dict[cnode] == init_flag ) :

    tnode = random.randint(min_node, max_node)
    debug('random tnode is %d, cnode is %d', tnode, cnode)
    node_entry = rand_node_dict[tnode]

    while ( node_entry != init_flag or tnode == cnode ) : 
      tnode = random.randint(min_node, max_node)
      debug('random tnode is %d, cnode is %d', tnode, cnode)
      node_entry = rand_node_dict[tnode]

    rand_node_dict[tnode] = cnode
    rand_node_dict[cnode] = tnode
   
  else:
    tnode = rand_node_dict[cnode]

  return tnode


##########################################################################
#
#  Function : set_ranklist
#
#  Description : set ranklist list that maps rank to target rank
#
##########################################################################

def set_ranklist() :
  
  global rank_map, node_pair, node_map, ranklist

  rank_targ_map = {}

  #  Generate map rank_targ_map that maps a rank to its target rank
  for rank in range(0, task_count) :
    tnode = node_map[rank]
    tnode_list = rank_map[node_pair[tnode]]
    idx = rank % tasks_per_node
    debug('tnode is %d, tnode_list is %s, idx is %d', tnode, tnode_list, idx)

    if ( len(tnode_list) < 1 ) :
      targ_rank = spin_flag
    else:
      targ_rank = tnode_list[idx]

    ranklist[rank] = targ_rank
  
  if ( debug_flag == True ) :
    print rank_targ_map


##########################################################################
#
#  Function : set_rand_proc_ranklist
#
#  Description : set random ranklist list that maps rank to target rank
#
##########################################################################

def set_rand_proc_ranklist():

  global ranklist

  ranklist = [init_flag] * task_count

  for idx in range(0, task_count) :

    if ( ranklist[idx] == init_flag ) :

      rank_entry = 0
      while ( ranklist[rank_entry] != init_flag or rank_entry == idx or \
        node_map[idx] == node_map[rank_entry] ) : 
        rank_entry = random.randint(0, task_count-1)
	debug('Will pass with rank_entry %d and idx %d?', rank_entry, idx)
  
      ranklist[idx] = rank_entry
      ranklist[rank_entry] = idx
      debug('set ranklist[idx] to %d, set ranklist[rank_entry] to %d', ranklist[idx], ranklist[rank_entry])



##########################################################################
#
#  Function : map_node_pairs
#
#  Description : Populate node_pair dictionary which identifies the 
#    communcation target node for a given node number for this communication 
#    configuration.  Assign enough nodes to handle task count in the
#    event of excluded nodes.
#
##########################################################################

def map_node_pairs(pair_func):
  
  global start_node, max_node, exclude_list, task_count, node_pair
  
  node_pair = {}
  debug('***  In map_node_pairs\n')

  # current node number
  cnode = min_node
  tasks = 0
  
  #  Iterate through nodes and assign target node
  while ( cnode <= max_node and tasks < task_count ) :


    #  Determine target node
    tnode = pair_func(cnode)

    debug('cnode is %d, tasks is %d, task_count is %d, tnode is %d', cnode, tasks, task_count, tnode)

    node_pair[cnode] = tnode

    if ( not str(cnode) in exclude_list ) :
      tasks += tasks_per_node
      
    cnode = cnode + 1


##########################################################################
#
#  Function : map_ranks_to_nodes
#
#  Description : Populate node_map list, which, using a process rank index
#    returns the node that the task is (supposed to be) running on.
#
##########################################################################

def map_ranks_to_nodes():
  
  global min_node, node_map, rank_map, task_count
  
  debug('***  In map_ranks_to_nodes\n')

  node_map = {}
  rank_map = {}
  for idx in range(0, max_node+1) :
    rank_map[idx] = []

  #  Using the start node number, iterate over the number of tasks that
  #    will be running, assigning node_map values, unless the current node
  #    is in the node exclusion list.

  cnode = min_node  # current node
  cton = 0               # current process count mapped on the current node

  for i in range(0, task_count) :

    # if exceeded tasks per node with current mapping, move on the to next node
    if ( cton >= tasks_per_node ) :
      cnode = cnode + 1
      cton = 0

    # skip nodes in exclusion list 
    while ( str(cnode) in exclude_list ) :
      cnode = cnode + 1

    #  Set node_map relationship:  rank i, should be running on node cnode
    node_map[i] = cnode

    #  Set rank_map relationship:  rank i, should be running on node cnode
    debug('adding rank %d to node %d , list is %s', i, cnode, rank_map[cnode])
    (rank_map[cnode]).append(i)

    cton = cton + 1

  if ( debug_flag == True ) :
    for i in range(0, len(node_map)) :
      print 'Node Map %5d : %5d' % (i, node_map[i])
    for i in range(0, len(rank_map)) :
      print 'Rank Map %5d : %5s' % (i, rank_map[i])
  

def list_patterns():

  print '\nCommunication Pattern Flags\n'

  for cidx in range(0, len(com_pattern_flags)) :
     print '  %-*s : %s' % ( 9, com_pattern_flags[cidx], com_pattern_names[cidx] )
    
  print


def usage():

  global all_node_configs, arch, all_test_dirs, tarfile

  opt_info = [ \
     [ 'b', 'Lowest node number  e.g. 1   default is %d' %(min_node)], \
     [ 'e', 'Highest node number e.g. 512 default is %d' % (max_node)], \
     [ 'h', 'Help message'], \
     [ 'l', 'List communication patterns'], \
     [ 'p', 'Communication pattern        default is %s' % (com_patterns)], \
     [ 't', 'Tasks per node               default is %d' % (tasks_per_node)], \
     [ 'x', 'Exclude list by node number, quoted and space-delimited e.g. "3 5 9"'], \
     ]
             
  print "\n  Usage: manage_targets.py [options]\n"
  for a, i in opt_info :
    print '  -%-5s : %s' % (a,i)
  print
  sys.exit(0)


##########################################################################
#
#  Main
#
##########################################################################


##########################################################################
#  Default Variables Values
##########################################################################

#  Node start id (such as up001)
min_node = 1

#  Exclude Node numbers
exclude_list = [ ]

#  Maximum node number (such as up064)
#  This is the highest node number in the system and must be a factor of 4
max_node = 16

#  Default MPI tasks per node
tasks_per_node = 8

#  Default communication pattern (Nearest Neighbor)
com_patterns = [ 'nn' ]

list_patterns_flag = False


##########################################################################
#  Check/Set command-line options
##########################################################################

try:
    opts, args = getopt.getopt(sys.argv[1:], "b:e:ghlp:t:x:", \
      [])

except getopt.GetoptError:
    usage()
    sys.exit(2)


for o, a in opts:
    if o == "-b":
        min_node = int(a)
    if o == "-e":
        max_node = int(a)
    if o == "-g":
        debug_flag = True
    if o == "-h":
        usage()
    if o == "-l":
        list_patterns_flag = True
    if o == "-p":
        com_patterns = a.split()
    if o == "-t":
        tasks_per_node = int(a)
    if o == "-x":
        exclude_list = a.split()

#  Total number of available nodes
node_count = max_node - min_node + 1 - len(exclude_list)

#  Total number of tasks to be run over
task_count = node_count * tasks_per_node

  

##########################################################################
#  Global information objects
##########################################################################
node_map = {}
node_pair = {}
rankvals = [init_flag] * task_count
ranklist = [init_flag] * (task_count+len(exclude_list)*tasks_per_node);
rand_node_dict = [init_flag] * (max_node+1)

debug('task_count is %d', task_count)
debug('exclude list is %s', exclude_list)
debug('node_count is %d', node_count)
debug('tasks_per_node is %d', tasks_per_node)
debug('sizeof ranklist is %d', len(ranklist))


#  Communication Pattern Definitions
com_pattern_flags = ['nn', 'nw_ne', 'nw_se', 'nw_sw', 'rand_node', 'rand_proc' ]
com_pattern_names = [ \
  'Nearest Neighbor', \
  'Northwest-to-Northeast Quadrants', \
  'Northwest-to-Southeast Quadrants', \
  'Northwest-to-Southwest Quadrants', \
  'Random-Between-Nodes', \
  'Random-Between-Processes' \
  ]

com_node_funcs = [ \
  nn_node_pair, \
  nw_ne_node_pair, \
  nw_se_node_pair, \
  nw_sw_node_pair, \
  rand_node_pair
  ]


##########################################################################
#  Script Actions
##########################################################################

if list_patterns_flag == True :
  list_patterns()
  sys.exit(0)

#  Get process node location information
map_ranks_to_nodes()


#  Iterate through communication pattern configurations
for cidx in range(0, len(com_pattern_flags)) :

  cflag = com_pattern_flags[cidx]

  #  If a match is found, generate communication pattern rank information
  if ( cflag in com_patterns ) :

    debug('current pattern %s', cflag)

    print_header(com_pattern_names[cidx])
    if ( cflag == 'rand_proc' ) :
      set_rand_proc_ranklist()
    else:
      map_node_pairs(com_node_funcs[cidx])
      set_ranklist()

    print_rank_list()

