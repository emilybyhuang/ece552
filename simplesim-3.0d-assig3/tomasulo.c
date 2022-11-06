
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

// We decided to use a linked list for the instruction fetch queue: no need for these now
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
	struct Node *next;
};
typedef struct Node Node;

struct Queue {
	int count;
	Node *head;
	Node *tail;
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
	if(isQueueFull(q)){
      printf("Queue is full\n\n\n");
      return false;
   }
	Node * node = malloc(sizeof(Node));
   // can't allocate cuz out of memory
	if(node == NULL){
      printf("Can't malloc\n\n\n");
      return false;
   }
	node -> data = data;
	node -> next = NULL;
   if (q->count == 0) {
      q->head = node;
      q->tail = node;
   } else {
      q->tail->next = node;
      q->tail = node;
   }
	q -> count = q -> count + 1;
	return true;
}

bool queuePop(Queue *q){
	if(q -> count == 0)
		return false;
	Node * nodeToDelete = q -> head;
	q -> head = q -> head -> next;
	q -> count = q -> count - 1;
	if (q->count == 0) {
      q->head = q->tail = NULL;
   }
	free(nodeToDelete);
	return true; 
}

static Queue *IFQ;


/* FUNCTIONAL UNITS */
void printFU(int current_cycle){
   printf("-----printing fu tables @ cycle: %d------\n", current_cycle);
   for(int i = 0; i < FU_INT_SIZE; i++){
      if(fuINT[i] != NULL)
         printf("fuInt index: %d contains instr index: %d\n", i, fuINT[i] -> index);
   }
   for(int i = 0; i < FU_FP_SIZE; i++){
      if(fuFP[i] != NULL)
         printf("fuFp index: %d contains instr index: %d\n", i, fuFP[i] -> index);
   }
}

instruction_t *getFuAvail(instruction_t *fu[], int numFU){
   for(int i = 0; i < numFU; i++){
      if(fu[i] == NULL)
         return fu[i];
   }
   return NULL;
}

/* MAP TABLE */
/*
Description:
   Update the output reg in map table with it's corresponding instruction
*/
void updateMaptable(instruction_t* currInstr){
   for(int i = 0; i < NUM_OUTPUT_REGS; i++){
      if(currInstr -> r_out[i] != DNA)
         map_table[currInstr -> r_out[i]] = currInstr;
   }
}

/* RESERVATION STATIONS */
void setRSTagsToNullForRAW(instruction_t *instrToSearch){
   // check all int rs first
   for(int i = 0; i < RESERV_INT_SIZE; i++){
      // this reservation station entry is not in use
      if(reservINT[i] == NULL)
         continue;
      for(int j = 0; j < NUM_INPUT_REGS; j++){
         if(reservINT[i] -> Q[j] == instrToSearch){
            reservINT[i] -> Q[j] = NULL;
         }
      }
   }

   // check all fp rs now
   for(int i = 0; i < RESERV_FP_SIZE; i++){
      // this reservation station entry is not in use
      if(reservFP[i] == NULL)
         continue;
      for(int j = 0; j < NUM_INPUT_REGS; j++){
         if(reservFP[i] -> Q[j] == instrToSearch){
            reservFP[i] -> Q[j] = NULL;
         }
      }
   }
}

int getFreeReservationEntry(instruction_t *reservationStationTable[], int size){
   // traverse reservationStationTable and check if it's null
   for(int i = 0; i < size; i++){
      // as long as if one empty
      if(reservationStationTable[i] == NULL)
         return i;
   }
   return -1;
}

/*
Description:
   Update the input reg in reservatin station with
   1. corresponding tag(instr) if not ready
   2. NULL if ready
*/ 
void updateRSEntry(instruction_t* currInstr){
   for(int i = 0; i < NUM_INPUT_REGS; i++){
      // NOTE: map_table[currInstr -> r_in[i]] might be NULL
      // if NULL: that means it's ready for issue
      // else: record the value for RAW
      if(currInstr -> r_in[i] != DNA){
         currInstr -> Q[i] = map_table[currInstr->r_in[i]];
      }
   }   
}

int getOldestRsToBroadcast(instruction_t *rsTable[], int rsTableSize, int current_cycle){
   int oldest_cycle = current_cycle;
   int rsIndex = INT_MAX;
   // printReservationTable(current_cycle);
   for(int i = 0; i < rsTableSize; i++){
      // not in use
      if(rsTable[i] == NULL)
         continue;

      if(rsTable[i] -> tom_execute_cycle != -1){
         enum md_opcode currOp = rsTable[i]->op;
         
         int latency = 0;
         if(USES_INT_FU(currOp)){
            latency = FU_INT_LATENCY;
         }
         if(USES_FP_FU(currOp)){
            latency = FU_FP_LATENCY;
         }
         int cycle_done = rsTable[i] -> tom_execute_cycle + latency - 1;

         // if already done executing
         if(cycle_done < current_cycle){
            // for the first one only
            if(rsIndex == INT_MAX)
               rsIndex = i;
            if(rsIndex != INT_MAX && rsTable[i] -> index < rsTable[rsIndex] -> index) 
               rsIndex = i;
         }  
      }
   }
   // return the older instruction in program
   return (rsIndex == INT_MAX) ? -1 : rsIndex;
}

void printReservationTable(int current_cycle){
   printf("-----printing rs tables at cycle: %d----\n", current_cycle);
   for(int i = 0; i < RESERV_INT_SIZE; i++){
      if(reservINT[i] != NULL){
         printf("reservINT index: %d, instr index in it: %d\n", i, reservINT[i] -> index);
         for(int j = 0; j < NUM_INPUT_REGS; j++){
            if(reservINT[i] -> Q[j] == NULL){
               printf("Q[%d] == NULL\n", j);
            }else{
               printf("Q[%d] points to instr %d\n", j, reservINT[i] -> Q[j] -> index);
            }
         }
      }
   }

   for(int i = 0; i < RESERV_FP_SIZE; i++){
      if(reservFP[i] != NULL)
         printf("reservFP index: %d, instr index in it: %d\n", i, reservFP[i] -> index);
   }
}

// For store that's done: need to clear FU and RS
void freeFuRSForStoreDone(int current_cycle){
   // check for store that's done
   for(int i = 0; i < FU_INT_SIZE; i++){
      if(fuINT[i] && IS_STORE(fuINT[i] -> op) && fuINT[i] -> tom_execute_cycle != -1){
         int latency = FU_INT_LATENCY;
         
         int cycle_done = fuINT[i]  -> tom_execute_cycle + latency - 1;
         if(cycle_done < current_cycle){
            fuINT[i] -> tom_cdb_cycle = 0;
            
            // free rs for this entry
            for(int j = 0; j < RESERV_INT_SIZE; j++){
               if(reservINT[j] && reservINT[j] -> index == fuINT[i] -> index)
                  reservINT[j] = NULL;
            }
            fuINT[i] = NULL;
         }
      }
   }
}

void printIFQ(Queue *q){
   Node *curr = q -> head;
   while(curr){
      printf("ifq index: %d\n", curr -> data ->index);
      curr = curr -> next;
   }
   return;
}

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
   // bool isFinished = false;

   // Check IFQ
   if(IFQ -> count != 0){
      // printf("ifq not empty\n");
      // printIFQ(IFQ);
      // printf("Queue size: %d\n", IFQ -> count);
      return false;
   }

   // Check RS
   for(int i = 0; i < RESERV_INT_SIZE; i++){
      if(reservINT[i] != NULL){
         //printf("reservint not empty\n");
         return false;
      }
   }

   for(int i = 0; i < RESERV_FP_SIZE; i++){
      if(reservFP[i] != NULL){
         //printf("reservfp not empty\n");
         return false;
      }
   }

   // Check CDB
   if(commonDataBus != NULL){
      //printf("cdb not empty\n");
      return false;
   }

   // Check FU
   for(int i = 0; i < FU_INT_SIZE; i++){
      if(fuINT[i] != NULL){
         //printf("fuint not empty\n");
         return false;
      }
   }

   for(int i = 0; i < FU_FP_SIZE; i++){
      if(fuFP[i] != NULL){
         //printf("fufp not empty\n");
         return false;
      }
   }

   return true; //ECE552: you can change this as needed; we've added this so the code provided to you compiles
}

instruction_t *getOlder(instruction_t *a, instruction_t *b){
   if(!a)
      return b;
   if(!b)
      return a;
   if(a -> index < b -> index)
      return a;
   else
      return b;
}

/* 
 * Description: 
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
 // CDB stage
void CDB_To_retire(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
   
   // printf("\n\nEnterrring retire\n");
   // printFU(current_cycle);
   // printReservationTable(current_cycle);

   // that means there's nothing to retire
   if(!commonDataBus)
      return;

   setRSTagsToNullForRAW(commonDataBus);

   // update map table s.t. the output reg is now null
   for(int i = 0; i < NUM_OUTPUT_REGS; i++){
      if(commonDataBus -> r_out[i] != DNA && map_table[commonDataBus -> r_out[i]] == commonDataBus){
         map_table[commonDataBus -> r_out[i]] = NULL;
      }
   }
   commonDataBus = NULL;   
}

// Checks if the Q for a RS entry is NULL
bool operandsReady(instruction_t *entry, int current_cycle){
   for(int i = 0; i < NUM_INPUT_REGS; i++){
      if(entry -> r_in[i] != DNA && entry -> Q[i] != NULL){
         // printf("Operand index: %d not ready\n\n", i);
         return false;
      }
   }
   return true;
}

// get oldest that has been issued and operands ready
instruction_t *getOldestRsToExecute(instruction_t *rsTable[], int rsTableSize, int current_cycle){
   int instrIndex = INT_MAX;
   int oldest_cycle = current_cycle;
   instruction_t *oldestRSEntry = NULL;
   for(int i = 0; i < rsTableSize; i++){
      if(rsTable[i] == NULL)
         continue;

      // already has something that's executing
	   if(rsTable[i] -> tom_execute_cycle != -1)
		   continue;  

      // if all operands ready and dispatch cycle is before current cycle: candidate for issue
      if(rsTable[i] -> tom_issue_cycle != -1 && operandsReady(rsTable[i], current_cycle)){
         if(rsTable[i] -> index < instrIndex){
            oldestRSEntry = rsTable[i];
            instrIndex = rsTable[i] -> index;
         }
      }  
   }
   return (instrIndex == INT_MAX)? NULL:oldestRSEntry;
}

/* 
 * Description: 
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
 // EXECUTE STAGE
 // TODO: Store just needs to remove rs, fu: no need to go to CDB
//        cdb_cycle needs to be 0
void execute_To_CDB(int current_cycle) {

   /* ECE552: YOUR CODE GOES HERE */
   // printf("\n\n\nEnterring CDB\n");
   // printReservationTable(current_cycle);
   // printFU(current_cycle);
   freeFuRSForStoreDone(current_cycle);

   instruction_t *reservEntryToBroadCast = NULL;

   int reservIntIndexToBroadCast = getOldestRsToBroadcast(reservINT, RESERV_INT_SIZE, current_cycle);
   int reservFpIndexToBroadCast = getOldestRsToBroadcast(reservFP, RESERV_FP_SIZE, current_cycle);

   // this is a pointer to the reservation station
   if(reservIntIndexToBroadCast == -1 && reservFpIndexToBroadCast == -1)
      reservEntryToBroadCast = NULL;
   else if(reservIntIndexToBroadCast == -1 && reservFpIndexToBroadCast != -1)
      reservEntryToBroadCast = reservFP[reservFpIndexToBroadCast];
   else if(reservIntIndexToBroadCast != -1 && reservFpIndexToBroadCast == -1)
      reservEntryToBroadCast = reservINT[reservIntIndexToBroadCast];
   else 
      reservEntryToBroadCast = getOlder(reservINT[reservIntIndexToBroadCast], reservFP[reservFpIndexToBroadCast]);

   // there is a valid entry to broadcast
   if(reservEntryToBroadCast && reservEntryToBroadCast -> tom_cdb_cycle == -1){
      commonDataBus = reservEntryToBroadCast;
      reservEntryToBroadCast -> tom_cdb_cycle = current_cycle;

      // go through reservation station and find the matching one on CDB to clear
      for(int i = 0; i < RESERV_INT_SIZE; i++){
         if(reservINT[i] && reservINT[i] -> index == reservEntryToBroadCast -> index){
            // printf("Freeing rs int entry: %d @ cycle\n", i, current_cycle);
            reservINT[i] = NULL;
         }
      }

      for(int i = 0; i < RESERV_FP_SIZE; i++){
         if(reservFP[i] && reservFP[i] -> index == reservEntryToBroadCast -> index){
            // printf("Freeing rs fp entry: %d @ cycle\n", i, current_cycle);
            reservFP[i] = NULL;
         }
      }
      // go through fu and free current occupied
      for(int i = 0; i < FU_INT_SIZE; i++){
         if(fuINT[i] && fuINT[i] -> index == commonDataBus -> index) {
            fuINT[i] = NULL;
            // printf("FU_INT[%d] is freed at cycle %d\n", i, current_cycle);
         }
      }

      if(fuFP[0] && fuFP[0] -> index == commonDataBus -> index)
         fuFP[0] = NULL;
   }
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

// ENTER EXECUTE STAGE
void issue_To_execute(int current_cycle) {

   /* ECE552: YOUR CODE GOES HERE */
   // printf("\n\nEnterring execute:\n");
   // printReservationTable(current_cycle);
   // printFU(current_cycle);
   
   // because there's 3 fuINT: can execute at most 3
   for(int i = 0; i < FU_INT_SIZE; i++){
      // for each fu that's free: get an rs entry to execute
      if(fuINT[i] == NULL){
         instruction_t *reservIntEntryToExecute = getOldestRsToExecute(reservINT, RESERV_INT_SIZE, current_cycle);
         // there is something available to execute
         if(reservIntEntryToExecute && reservIntEntryToExecute -> tom_execute_cycle == -1){
            reservIntEntryToExecute -> tom_execute_cycle = current_cycle;
            fuINT[i] = reservIntEntryToExecute;
         }
      }
   }


   // there's only 1 fuFP: so just check if it's null or not
   if(fuFP[0] == NULL){
      instruction_t *reservFpEntryToExecute = getOldestRsToExecute(reservFP, RESERV_FP_SIZE, current_cycle);
      if(reservFpEntryToExecute && reservFpEntryToExecute -> tom_execute_cycle == -1){
         reservFpEntryToExecute -> tom_execute_cycle = current_cycle;
         fuFP[0] = reservFpEntryToExecute;
      }
   }
}

/* 
 * Description: 
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */

// DISPATCH STAGE
// Main purpose: check if issuable: if source operands are ready(Q is all NULL)
// can issue
void dispatch_To_issue(int current_cycle) {

   /* ECE552: YOUR CODE GOES HERE */
   // printf("\n\nEnterring issue\n");
   // printReservationTable(current_cycle);
   // printFU(current_cycle);

   bool success = false;
   // can't dispatch
   if(IFQ -> head == NULL){
      return;
   }
  
   instruction_t* currInstr = IFQ->head->data;
   enum md_opcode currOp = currInstr->op;

   // for branches: issue, execute, and cdb are all 0 
   if(IS_UNCOND_CTRL(currOp) || IS_COND_CTRL(currOp)){
      currInstr -> tom_issue_cycle = 0;
      currInstr -> tom_execute_cycle = 0;
      currInstr -> tom_cdb_cycle = 0;
      queuePop(IFQ);
      return;
   }else if (USES_INT_FU(currOp)) {
      // INT
      // returns true if avail entry and entryToInsert is where to insert
      // returns false if no entry avail
      int indexToInsert = getFreeReservationEntry(reservINT, RESERV_INT_SIZE);
      if(indexToInsert != -1){
         success = true;
         reservINT[indexToInsert] = currInstr;
         update_rs_entry(currInstr);
         //printf("current cycle update rs %d at cycle %d\n", indexToInsert, current_cycle);
         update_maptable(currInstr);
      }
   } else if (USES_FP_FU(currOp)) {
      // FP
      // returns true if avail entry and entryToInsert is where to insert
      // returns false if no entry avail
      int indexToInsert = getFreeReservationEntry(reservFP, RESERV_FP_SIZE);
      if(indexToInsert != -1){
         success = true;
         reservFP[indexToInsert] = currInstr;
         update_rs_entry(currInstr);
         //printf("current cycle update rs %d at cycle %d\n", indexToInsert, current_cycle);
         update_maptable(currInstr);
      }
   }
   
   // remove instruction from queue
   if(success){
      currInstr->tom_issue_cycle = current_cycle;
      queuePop(IFQ);
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
   // printf("\n\nEnterring dispatch\n");
   // printReservationTable(current_cycle);
   // printFU(current_cycle);
   if (isQueueFull(IFQ)) {
      return;
   }

   instruction_t *instr = NULL;
   fetch_index++;

   // no more instructions to fetch case
   if (fetch_index > sim_num_insn)
      return;

   instr = get_instr(trace, fetch_index);
   
   // if it is a trap instruction: get another instr
   while (IS_TRAP(instr->op)) {
      fetch_index++;
      instr = get_instr(trace, fetch_index);
   }

   bool instrInsertionSuccess = queuePush(instr, IFQ);

   // that means that this instruction couldn't be inserted
   // (either instr queue full or ran out of memory: try again)
   if(!instrInsertionSuccess){
      fetch_index--;
   }else{
      instr->tom_dispatch_cycle = current_cycle;
      instr->tom_issue_cycle = -1;
      instr->tom_execute_cycle = -1;
      instr->tom_cdb_cycle = -1;
	} 
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

   IFQ = malloc(sizeof(Queue));
   IFQ -> count = 0;
   IFQ -> head = NULL;
   IFQ -> tail = NULL;

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
   int cycle = 1;
   /* ECE552: my code end*/

   while (true) {
     /* ECE552: YOUR CODE GOES HERE */
      CDB_To_retire(cycle);
      execute_To_CDB(cycle);
      issue_To_execute(cycle);
      dispatch_To_issue(cycle);
      fetch_To_dispatch(trace, cycle);
      
      if (is_simulation_done(sim_num_insn))
         break;
      cycle++;
   }
   is_simulation_done(sim_num_insn);
   // print_all_instr(trace, sim_num_insn);
   return cycle;
}
