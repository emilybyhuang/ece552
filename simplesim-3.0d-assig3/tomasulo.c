
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

#define NUM_INPUT_REGS     3
#define NUM_OUTPUT_REGS    2

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
// static instruction_t* instr_queue[INSTR_QUEUE_SIZE];
//number of instructions in the instruction queue
// static int instr_queue_size = 0;

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
   // Check all of the reservation stations and execute one from INT and FP
   // if all of their operands are ready



   

}

bool getFreeReservationEntry(instruction_t *reservationStationTable[], int size, instruction_t *insertPlace){
   // traverse reservationStationTable and check if it's null
   for(int i = 0; i < size; i++){
      // as long as if one empty
      if(reservationStationTable[i] == NULL)
         insertPlace = reservationStationTable[i];
         return false;
   }
   return true;
}

bool operandsReady(instruction_t *entry){
   for(int i = 0; i < NUM_INPUT_REGS; i++){
      if(entry -> Q[i] != NULL)
         return false;
   }
   return true;
}

// issue the first that has been dispatched with all operands ready in reservation station
instruction_t *t getOldestRsToIssue(instruction_t *rsTable[], int rsTableSize, int current_cycle){
   int oldest_cycle = current_cycle;
   instruction_t *oldestRSEntry = NULL; 
   for(int i = 0; i < rsTableSize; i++){
      if(rsTable[i] == NULL)
         continue;
      // if all operands ready and dispatch cycle is before current cycle: candidate for issue
      if(operandsReady(rsTable[i]) && 
         rsTable[i] -> tom_dispatch_cycle < oldest_cycle){
         oldest_cycle = tom_dispatch_cycle;
         oldestRSEntry = rsTable[i];
      }  
   }
   return oldestRSEntry;
}

/* 
 * Description: 
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */

// ENTER ISSUE STAGE
// Main purpose: check if issuable: if source operands are ready(Q is all NULL)
// can issue
void dispatch_To_issue(int current_cycle) {

   /* ECE552: YOUR CODE GOES HERE */
   // If no RAW: issue

   // check reservINT: get the oldest one
   instruction_t *reservIntEntryToIssue = getOldestRsToIssue(reservINT, RESERV_INT_SIZE, current_cycle);
   // NULL means that there's nothing ready to issue
   if(reservIntEntryToIssue){
      reservIntEntryToIssue.tom_issue_cycle = current_cycle;
   }

   // check reservFP: get the oldest one
   instruction_t *reservFpEntryToIssue = getOldestRsToIssue(reservFP, RESERV_FP_SIZE, current_cycle);
   // NULL means there's nothing ready to issue
   if(reservFpEntryToIssue){
      reservFpEntryToIssue.tom_issue_cycle = current_cycle;
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

// ENTER FETCH STAGE
// Main purpose:  fetch instr and push into IFQ queue
void fetch(instruction_trace_t* trace) {
  
   /* ECE552: YOUR CODE GOES HERE */
   if (fetch_index >= INSTR_TRACE_SIZE || isQueueFull(IFQ)) {
      return;
   }

   instruction_t *instr = NULL;
   instr = get_instr(trace, fetch_index);
   fetch_index++;
   // if it is a trap instruction: get another instr
   while (IS_TRAP(instr->op)) {
      fetch_index++;
      instr = get_instr(trace, fetch_index);
   }

   // this must be a non trap instruction: 
   // it's valid to be pushed into instruction queue
   bool instrInsertionSuccess = queuePush(instr, IFQ);

   // that means that this instruction couldn't be inserted
   // (either instr queue full or ran out of memory: try again)
   if(!instrInsertionSuccess){
      fetch_index--;
   }
   
}

/*
Description:
   Update the input reg in reservatin station with
   1. corrresponding tag(instr) if not ready
   2. NULL if ready
*/ 
void update_rs_entry(instruction_t* currInstr){
   for(int i = 0; i < NUM_INPUT_REGS; i++){
      // NOTE: map_table[currInstr -> r_in[i]] might be NULL
      // if NULL: that means it's ready for issue
      // else: record the value for RAW
      if(currInstr -> r_in[i] != DNA){
         currInstr -> Q[i] = map_table[currInstr->r_in[i]];
      }
   }   
}

/*
/*
Description:
   Update the output reg in map table with it's corresponding instruction
*/
void update_maptable(instruction_t* currInstr){
   for(int i = 0; i < NUM_OUTPUT_REGS; i++){
      if(currInstr -> r_out[i] != DNA)
         map_table[currInstr -> r_out] = currInstr;
   }
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
// ENTER DISPATCH STAGE
// Main purpose: 
void fetch_To_dispatch(instruction_trace_t* trace, int current_cycle) {
   /* ECE552: YOUR CODE GOES HERE */
   // case where no instruction in instruction queue: can't dispatch
   if (IFQ -> head == IFQ -> tail){
      return;
   }
 

   /*
   A fetched instruction can be dispatched (complete D) immediately if a reservation
   station entry will be available in the next cycle. Structural hazards on reservation
   stations are resolved here with stalls.
   */
   bool D_success = false;
   instruction_t* currInstr = IFQ->head->data;
   // check for structural hazard

   md_opcode currOp = currInstr->op;

   //OoO issue
   //check from head to tail: the first available dispatched && has RS available


   if(IS_UNCOND_CTRL(currOp) || IS_COND_CTRL(currOp)){
      D_success = true;
      currInstr->tom_dispatch_cycle = current_cycle;
   }else if (USES_INT_FU(currOp)) {
      //INT
      instruction_t *insertPlace = NULL;
      // returns true if avail entry and entryToInsert is where to insert
      // returns false if no entry avail
      D_success = getFreeReservationEntry(reservINT, RESERV_INT_SIZE, insertPlace);
      if(D_success){
         insertPlace = currInstr;
         currInstr->tom_dispatch_cycle = current_cycle;
         update_rs_entry(currInstr);
         update_maptable(currInstr);
      }
   } else if (USES_FP_FU(currOp)) {
      //FP
      instruction_t *insertPlace = NULL;
      // returns true if avail entry and entryToInsert is where to insert
      // returns false if no entry avail
      D_success = getFreeReservationEntry(reservFP, RESERV_FP_SIZE, insertPlace);
      if(D_success){
         insertPlace = currInstr;
         currInstr->tom_dispatch_cycle = current_cycle;
         update_rs_entry(currInstr);
         update_maptable(currInstr);
      } 
   }
   
   // remove instruction from queue
   if(D_success)
      queuePop(IFQ);
   // if !D_success: stall so don't pop from queue cuz wanna retry when reservation
   // station needed is ready so don't update rs entry nor map table
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
  fetch_index = 1;
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
