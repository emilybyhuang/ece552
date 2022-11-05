
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
	if(isQueueFull(q))return false;
	Node * node = malloc(sizeof(Node));
   // can't allocate cuz out of memory
	if(node == NULL) return false;
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
   // bool isFinished = false;

   // Check IFQ
   if(IFQ -> count != 0){
      //printf("IFQ not empty yet!\n");
      return false;
   }

   // Check RS
   for(int i = 0; i < RESERV_INT_SIZE; i++){
      if(reservINT[i] != NULL){
         //printf("RS INT entry not empty yet!\n");
         return false;
      }
   }

   for(int i = 0; i < RESERV_FP_SIZE; i++){
      if(reservFP[i] != NULL){
         printf("RS FP entry not empty yet!\n");
         return false;
      }
   }

   // Check CDB
   if(commonDataBus != NULL){
      printf("CDB not empty yet!\n");
      return false;
   }

   return true; //ECE552: you can change this as needed; we've added this so the code provided to you compiles
}


int getOldestRsToBroadcast(instruction_t *rsTable[], int rsTableSize, int current_cycle){
   int oldest_cycle = current_cycle;
   int rsIndex = -1;
   printReservationTable(current_cycle);
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
         printf("\n\nCycle done is %d for instr index: %d\n", cycle_done, rsTable[i] -> index);

         // if already done executing
         if(cycle_done < oldest_cycle){
            oldest_cycle = cycle_done;
            rsIndex = i;
         }  
      }
   }
   return rsIndex;
}

instruction_t *getOlder(instruction_t *a, instruction_t *b){
   if(!a)
      return b;
   if(!b)
      return a;
   if(a -> tom_execute_cycle < b -> tom_execute_cycle)
      return a;
   else
      return b;
}


void setRSTagsToNull(instruction_t *instrToSearch){
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
   
  // check all rs entries for something with execute cycle < current cycle: that means it's done
  // get the oldest one and put on CDB
  // free it's rs entry here as well
   printf("\n\n\nEnterrring retire\n");
   printFU(current_cycle);
   printReservationTable(current_cycle);

   // that means there's nothing to retire
   if(!commonDataBus)
      return;

   setRSTagsToNull(commonDataBus);

   // go through fu and free current occupied
   for(int i = 0; i < FU_INT_SIZE; i++){
      if(fuINT[i] && fuINT[i] -> index == commonDataBus -> index) {
         fuINT[i] = NULL;
         printf("FU_INT[%d] is freed at cycle %d\n", i, current_cycle);
      }
   }

   if(fuFP[0] && fuFP[0] -> index == commonDataBus -> index)
      fuFP[0] = NULL;

   // update map table s.t. the output reg is now null
   for(int i = 0; i < NUM_OUTPUT_REGS; i++){
      if(commonDataBus -> r_out[i] != DNA && map_table[commonDataBus -> r_out[i]] == commonDataBus){
         map_table[commonDataBus -> r_out[i]] = NULL;
      }
   }

   // printFU(current_cycle);
   // printf("Exiting retire\n");
   commonDataBus = NULL;   
}

bool operandsReady(instruction_t *entry,int current_cycle){
   printf("Checking instr: %d @ cycle %d to see if operands ready\n", entry -> index, current_cycle);
   for(int i = 0; i < NUM_INPUT_REGS; i++){
      // waiting for an instruction
      if(entry -> r_in[i] != DNA && entry -> Q[i] != NULL){
         printf("Operand index: %d not ready\n\n", i);
         return false;
      }
   }
   return true;
}

// get oldest that has been issued and operands ready
instruction_t *getOldestRsToExecute(instruction_t *rsTable[], int rsTableSize, int current_cycle){
   printf("\n\nTTTrying to find something to execute at cycle: %d\n", current_cycle);
   int oldest_cycle = current_cycle;
   instruction_t *oldestRSEntry = NULL;
   for(int i = 0; i < rsTableSize; i++){
      if(rsTable[i] == NULL)
         continue;

      printf("IIIIInstr %d in reservation table\n", rsTable[i] -> index);

      // already has something that's executing
	   if(rsTable[i] -> tom_execute_cycle != -1)
		   continue;  
      //printf("rsTable[i] -> tom_issue_cycle: %d\n", rsTable[i] -> tom_issue_cycle);

      if(rsTable[i] -> tom_issue_cycle != -1){
         printf("issued and trying to executeneed check operand ready for index: %d at cycle: %d\n", rsTable[i] -> index, current_cycle);
        
      }
      // if all operands ready and dispatch cycle is before current cycle: candidate for issue
      if(rsTable[i] -> tom_issue_cycle != -1 && rsTable[i] -> tom_issue_cycle < oldest_cycle && operandsReady(rsTable[i], current_cycle)){
         oldest_cycle = rsTable[i] -> tom_issue_cycle;
         oldestRSEntry = rsTable[i];
      }  
   }
   return oldestRSEntry;
}

void freeFuRSForStoreDone(int current_cycle){
   // check for store that's done
   for(int i = 0; i < FU_INT_SIZE; i++){
      if(fuINT[i] && IS_STORE(fuINT[i] -> op) && fuINT[i] -> tom_execute_cycle != -1){
         int latency = FU_INT_LATENCY;
         
         int cycle_done = fuINT[i]  -> tom_execute_cycle + latency - 1;
         if(cycle_done < current_cycle){
            printf("Done store @ cycle: %d for instr index: %d\n", current_cycle,fuINT[i] -> index);
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
   printf("\n\n\nEnterring CDB\n");
   freeFuRSForStoreDone(current_cycle);
   printReservationTable(current_cycle);
   printFU(current_cycle);

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
      printf("puting instr index: %d on cdb\n", reservEntryToBroadCast -> index);
      // setRSTagsToNull(commonDataBus);

      // go through reservation station and find the matching one on CDB to clear
      for(int i = 0; i < RESERV_INT_SIZE; i++){
         if(reservINT[i] && reservINT[i] -> index == reservEntryToBroadCast -> index){
            printf("Freeing rs int entry: %d @ cycle\n", i, current_cycle);
            reservINT[i] = NULL;
         }
      }

      for(int i = 0; i < RESERV_FP_SIZE; i++){
         if(reservFP[i] && reservFP[i] -> index == reservEntryToBroadCast -> index){
            printf("Freeing rs fp entry: %d @ cycle\n", i, current_cycle);
            reservFP[i] = NULL;
         }
      }
   }
}


instruction_t *getFuAvail(instruction_t *fu[], int numFU){
   for(int i = 0; i < numFU; i++){
      if(fu[i] == NULL)
         return fu[i];
   }
   return NULL;
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
   printf("\n\n\nEnterring execute:\n");
   printReservationTable(current_cycle);
   printFU(current_cycle);
   
   // because there's 3 fuINT: can execute at most 3
   for(int i = 0; i < FU_INT_SIZE; i++){
      // for each fu that's free: get an rs entry to execute
      if(fuINT[i] == NULL){
         printf("int HHHHave fu int avail @ cycle: %d\n", current_cycle);
         instruction_t *reservIntEntryToExecute = getOldestRsToExecute(reservINT, RESERV_INT_SIZE, current_cycle);
         
         
         // there is something available to execute
         if(reservIntEntryToExecute)
            printf("!!!!fu int, trying to execute instr index %d at cycle %d\n", reservIntEntryToExecute -> index, current_cycle);
         if(reservIntEntryToExecute && reservIntEntryToExecute -> tom_execute_cycle == -1){
            reservIntEntryToExecute -> tom_execute_cycle = current_cycle;
            fuINT[i] = reservIntEntryToExecute;
         }
      }
   }


   // there's only 1 fuFP: so just check if it's null or not
   if(fuFP[0] == NULL){
      printf("fp HHHHave fu int avail @ cycle: %d\n", current_cycle);
      instruction_t *reservFpEntryToExecute = getOldestRsToExecute(reservFP, RESERV_FP_SIZE, current_cycle);
      
      
      if(reservFpEntryToExecute)
         printf("!!!!fu fp, trying to execute instr index %d\n", reservFpEntryToExecute -> index);
      if(reservFpEntryToExecute && reservFpEntryToExecute -> tom_execute_cycle == -1){
         reservFpEntryToExecute -> tom_execute_cycle = current_cycle;
         fuFP[0] = reservFpEntryToExecute;
      }
   }
   printf("++++leaving execute now:\n");
   printReservationTable(current_cycle);
      
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
   printf("\n\nEnterring dispatch\n");
   
   bool success = false;
   // can't dispatch
   if(IFQ -> head == NULL){
      printf("!!!!Nothing in ifq now\n");
      return;
   }

      
   instruction_t* currInstr = IFQ->head->data;
   // check for structural hazard

   enum md_opcode currOp = currInstr->op;

   // for branches: issue, execute, and cdb are all 0 
   if(IS_UNCOND_CTRL(currOp) || IS_COND_CTRL(currOp)){
      currInstr -> tom_issue_cycle = 0;
      currInstr -> tom_execute_cycle = 0;
      currInstr -> tom_cdb_cycle = 0;
      queuePop(IFQ);
      printReservationTable(current_cycle);
      return;
   }else if (USES_INT_FU(currOp)) {
      //INT
      // returns true if avail entry and entryToInsert is where to insert
      // returns false if no entry avail
      int indexToInsert = getFreeReservationEntry(reservINT, RESERV_INT_SIZE);
      printf("current instr index: %d rs int index free to insert: %d\n", currInstr -> index, indexToInsert);
      if(indexToInsert != -1){
         printf("using rs int entry %d for instr index %d\n", indexToInsert, currInstr -> index);
         success = true;
         reservINT[indexToInsert] = currInstr;
         update_rs_entry(currInstr);
         //printf("current cycle update rs %d at cycle %d\n", indexToInsert, current_cycle);
         update_maptable(currInstr);
      }else{
         printf("\niiiiiiindex: %d No rs current cycle: %d\n", currInstr -> index, current_cycle);
      }
   } else if (USES_FP_FU(currOp)) {
      //FP

      // returns true if avail entry and entryToInsert is where to insert
      // returns false if no entry avail
      int indexToInsert = getFreeReservationEntry(reservFP, RESERV_FP_SIZE);
      printf("rs fp index free to insert: %d\n", indexToInsert);
      if(indexToInsert != -1){
         printf("using rs fp entry %d for instr index %d\n", indexToInsert, currInstr -> index);
         success = true;
         reservFP[indexToInsert] = currInstr;
         update_rs_entry(currInstr);
         //printf("current cycle update rs %d at cycle %d\n", indexToInsert, current_cycle);
         update_maptable(currInstr);
      } else{
        printf("\niiiiiindex: %d No rs current cycle: %d\n", currInstr -> index, current_cycle);
      }
   }
   
   // remove instruction from queue
   if(success){
      currInstr->tom_issue_cycle = current_cycle;
      queuePop(IFQ);
   }
   printReservationTable(current_cycle);
   // case where no instruction in instruction queue: can't dispatch
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

void printMapTableForReg(int current_cycle, instruction_t *reg){
   printf("At cycle: %d map table for reg: %d");
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
Description:
   Update the output reg in map table with it's corresponding instruction
*/
void update_maptable(instruction_t* currInstr){
   for(int i = 0; i < NUM_OUTPUT_REGS; i++){
      if(currInstr -> r_out[i] != DNA)
         map_table[currInstr -> r_out[i]] = currInstr;
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
void fetch_To_dispatch(instruction_trace_t* trace, int current_cycle) {
   /* ECE552: YOUR CODE GOES HERE */
   if (fetch_index >= INSTR_TRACE_SIZE || isQueueFull(IFQ)) {
      return;
   }

   instruction_t *instr = NULL;
   fetch_index++;
   instr = get_instr(trace, fetch_index);
   
   // if it is a trap instruction: get another instr
   while (IS_TRAP(instr->op)) {
      fetch_index++;
      instr = get_instr(trace, fetch_index);
   }

   // this must be a non trap instruction: 
   // it's valid to be pushed into instruction queue
   // printf("\n---lzh: index: %d---\n", instr->index);
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
   //printf("Inserted instr index: %d into IFQ\n", instr -> index); 
}

void printReservationTable(int current_cycle){
   printf("-----printing rs tables at cycle: %d----\n", current_cycle);
   for(int i = 0; i < RESERV_INT_SIZE; i++){
      if(reservINT[i] != NULL){
         printf("reservINT index: %d, instr index in it: %d\n", i, reservINT[i] -> index);
         // for(int j = 0; j < NUM_INPUT_REGS; j++){
         //    if(reservINT[i] -> Q[j] == NULL){
         //       printf("Q[%d] == NULL\n", j);
         //    }else{
         //       printf("Q[%d] points to instr %d\n", reservINT[i] -> Q[j] -> index);
         //    }
         // }
      }
   }

   for(int i = 0; i < RESERV_FP_SIZE; i++){
      if(reservFP[i] != NULL)
         printf("reservFP index: %d, instr index in it: %d\n", i, reservFP[i] -> index);
   }
}

void printMapTable(int current_cycle){
   printf("-----printing map tables at cycle: %d----\n", current_cycle);
   
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
//   for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
//     instr_queue[i] = NULL;
//   }
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
//   fetch_index = 1;
  initializeQueue(IFQ);
  /* ECE552: my code end*/

  int cycle = 1;
  while (true) {

     /* ECE552: YOUR CODE GOES HERE */
      CDB_To_retire(cycle);
      execute_To_CDB(cycle);
      issue_To_execute(cycle);
      dispatch_To_issue(cycle);
      fetch_To_dispatch(trace, cycle);
      
     cycle++;

     if (is_simulation_done(sim_num_insn))
        break;
     if (cycle == 50){
        print_all_instr(trace, sim_num_insn);
        break;
     }
  }
  //print_all_instr(trace, sim_num_insn);
  return cycle;
}