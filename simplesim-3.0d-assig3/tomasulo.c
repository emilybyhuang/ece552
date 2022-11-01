
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"
#include "decode.def"

#include "instr.h"

/* PARAMETERS OF THE TOMASULO'S ALGORITHM */

#define INSTR_QUEUE_SIZE         16

#define RESERV_INT_SIZE    5
#define RESERV_FP_SIZE     3
#define FU_INT_SIZE        3
#define FU_FP_SIZE         1

#define FU_INT_LATENCY     5
#define FU_FP_LATENCY      7

/* IDENTIFYING INSTRUCTIONS */

//unconditional branch, jump or call
#define IS_UNCOND_CTRL(op) (MD_OP_FLAGS(op) & F_CALL || \
                         MD_OP_FLAGS(op) & F_UNCOND)

//conditional branch instruction
#define IS_COND_CTRL(op) (MD_OP_FLAGS(op) & F_COND)

//floating-point computation
#define IS_FCOMP(op) (MD_OP_FLAGS(op) & F_FCOMP)

//integer computation
#define IS_ICOMP(op) (MD_OP_FLAGS(op) & F_ICOMP)

//load instruction
#define IS_LOAD(op)  (MD_OP_FLAGS(op) & F_LOAD)

//store instruction
#define IS_STORE(op) (MD_OP_FLAGS(op) & F_STORE)

//trap instruction
#define IS_TRAP(op) (MD_OP_FLAGS(op) & F_TRAP) 

#define USES_INT_FU(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_STORE(op))
#define USES_FP_FU(op) (IS_FCOMP(op))

#define WRITES_CDB(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_FCOMP(op))

/* FOR DEBUGGING */

//prints info about an instruction
#define PRINT_INST(out,instr,str,cycle)	\
  myfprintf(out, "%d: %s", cycle, str);		\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

#define PRINT_REG(out,reg,str,instr) \
  myfprintf(out, "reg#%d %s ", reg, str);	\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

/* VARIABLES */

//instruction queue for tomasulo
static instruction_t* instr_queue[INSTR_QUEUE_SIZE];
//number of instructions in the instruction queue
static int instr_queue_size = 0;

//reservation stations (each reservation station entry contains a pointer to an instruction)
static instruction_t* reservINT[RESERV_INT_SIZE];
static instruction_t* reservFP[RESERV_FP_SIZE];

//functional units
static instruction_t* fuINT[FU_INT_SIZE];
static instruction_t* fuFP[FU_FP_SIZE];

//common data bus
static instruction_t* commonDataBus = NULL;

//The map table keeps track of which instruction produces the value for each register
static instruction_t* map_table[MD_TOTAL_REGS];

//the index of the last instruction fetched
static int fetch_index = 0;

struct Node {
	instruction_t *data;
	struct node *next;
};
typedef struct Node Node;

struct Queue {
	int count;
	node *head;
	node *tail;
};
typedef struct Queue Queue;

void initializeQueue(Queue *q){
	q -> count = 0;
	q -> head = NULL;
	q -> tail = NULL;
}

bool isQueueFull(Queue *q){
	return (q -> count == INSTR_QUEUE_SIZE);
}

bool queuePush(instruction_t *data, Queue *q){
	if(isQueueFull(q))return false;
	Node * node = malloc(sizeof(Node));
	if(node == NULL) return false;
	node -> data = data;
	node -> next = NULL;
	if(q -> head == NULL){
		q -> head = node;
	}
	q -> tail = node;	
	q -> count = q -> count + 1;
	return true;
}

bool queuePop(Queue *q){
	if(q -> count == 0)
		return false;
	node * nodeToDelete = q -> head;
	q -> head = q -> head -> next;
	q -> count = q -> count - 1;
	nodeToDelete -> next = NULL;
	free(nodeToDelete);
	return true; 
}

static queue *IFQ;
/* FUNCTIONAL UNITS */


/* RESERVATION STATIONS */


/* 
 * Description: 
 * 	Checks if simulation is done by finishing the very last instruction
 *      Remember that simulation is done only if the entire pipeline is empty
 * Inputs:
 * 	sim_insn: the total number of instructions simulated
 * Returns:
 * 	True: if simulation is finished
 */
static bool is_simulation_done(counter_t sim_insn) {

  /* ECE552: YOUR CODE GOES HERE */

  // Check if all data structures are empty

  return true; //ECE552: you can change this as needed; we've added this so the code provided to you compiles
}

/* 
 * Description: 
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void CDB_To_retire(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

}


/* 
 * Description: 
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void execute_To_CDB(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

}

/* 
 * Description: 
 * 	Moves instruction(s) from the issue to the execute stage (if possible). We prioritize old instructions
 *      (in program order) over new ones, if they both contend for the same functional unit.
 *      All RAW dependences need to have been resolved with stalls before an instruction enters execute.
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void issue_To_execute(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
}

/* 
 * Description: 
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void dispatch_To_issue(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
   //OoO issue
   //check from head to tail: the first available dispatched && has RS available
   md_opcode_op op; //need to write...
   //.....
   // .....
   if (IS_ICOMP(op)) {
      //INT
      if (isReservFull(reservINT, RESERV_INT_SIZE)) {
         //stall
      } else {
         //allocate RS entry, set Map table (according to output reg) 

         D_success = true;
      }
   } else if (IS_FCOMP(op)) {
      //FP
      if (isReservFull(reservFP, RESERV_FP_SIZE)) {
         //stall
      } else {
         //allocate RS entry

         D_success = true;
      }
   } else if (IS_UNCOND_CTRL) {
      //unconditional branches
         D_success = true;        
   } else if (IS_COND_CTRL(op)) {
      // confitional branches
         D_success = true; 
   } else if (IS_LOAD(op)) {
      //load
      if (isReservFull(reservINT, RESERV_INT_SIZE)) {
         //stall
      } else {
         //allocate RS entry, set Map table (according to output reg) 

         D_success = true;
      }
   } else if (IS_STORE(op)) {
      //store
      if (isReservFull(reservINT, RESERV_INT_SIZE)) {
         //stall
      } else {
         //allocate RS entry

         D_success = true;
      }
   }


}

/* 
 * Description: 
 * 	Grabs an instruction from the instruction trace (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	None
 */
void fetch(instruction_trace_t* trace) {
  
  /* ECE552: YOUR CODE GOES HERE */
  if (fetch_index >= INSTR_TRACE_SIZE) {
     return;
  }
  if (isQueueFull(IFQ) == false) {
    instruction_t *instr = NULL;
    instr = get_instr(trace, fetch_index);
    if (!IS_TRAP(instr->op)) {
       // do not push TRAP insn
       queuePush(trace->table[fetch_index], IFQ); 
    }
    fetch_index ++;
  }

}

void isReservFuFull(instruction_t *table, int size) {

}


/* 
 * Description: 
 * 	Calls fetch and dispatches an instruction at the same cycle (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void fetch_To_dispatch(instruction_trace_t* trace, int current_cycle) {
  /* ECE552: YOUR CODE GOES HERE */

   bool D_success = false;
   //head instruction:
   //note: assume deep copy now so it's a static value
   // todo: not head, supposed to be the insn (havn't enter D stage) after head
   instruction_t* curr_insn = IFQ->tail->data;
   md_opcode curr_op = curr_insn->op;
   if (D_success)
      curr_insn->tom_dispatch_cycle = current_cycle;

}

/* 
 * Description: 
 * 	Performs a cycle-by-cycle simulation of the 4-stage pipeline
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	The total number of cycles it takes to execute the instructions.
 * Extra Notes:
 * 	sim_num_insn: the number of instructions in the trace
 */
counter_t runTomasulo(instruction_trace_t* trace)
{
  fetch_index = 1;
  //initialize instruction queue
  int i;
  for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
    instr_queue[i] = NULL;
  }

  //initialize reservation stations
  for (i = 0; i < RESERV_INT_SIZE; i++) {
      reservINT[i] = NULL;
  }

  for(i = 0; i < RESERV_FP_SIZE; i++) {
      reservFP[i] = NULL;
  }

  //initialize functional units
  for (i = 0; i < FU_INT_SIZE; i++) {
    fuINT[i] = NULL;
  }

  for (i = 0; i < FU_FP_SIZE; i++) {
    fuFP[i] = NULL;
  }

  //initialize map_table to no producers
  int reg;
  for (reg = 0; reg < MD_TOTAL_REGS; reg++) {
    map_table[reg] = NULL;
  }
  
  /* ECE552: my code begin*/
  initializeQueue(IFQ);
  /* ECE552: my code end*/

  int cycle = 1;
  while (true) {

     /* ECE552: YOUR CODE GOES HERE */

     cycle++;

     if (is_simulation_done(sim_num_insn))
        break;
  }
  
  return cycle;
}
