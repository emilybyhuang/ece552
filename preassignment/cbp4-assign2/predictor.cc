#include "predictor.h"

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////

// array to hold all of the prections
#define NUM_PRED_TWOBIT_SAT 4096
#define TWELVE_BIT_MASK 0xFFF 

UINT32 predictionTable[NUM_PRED_TWOBIT_SAT];

void InitPredictor_2bitsat() {
	std::fill_n(predictionTable, NUM_PRED_TWOBIT_SAT, 0b01);	
}

bool GetPrediction_2bitsat(UINT32 PC) {
	//get the lowest 12 bits to index into the global array
	UINT32 pcForIndex = PC & TWELVE_BIT_MASK;
	UINT32 prediction = predictionTable[pcForIndex];	
	if(prediction >> 1)return TAKEN;
	else return NOT_TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	// get the lowest 12 bit to index into global array
	UINT32 pcForIndex = PC & TWELVE_BIT_MASK;

	if(predDir == NOT_TAKEN){
		if(resolveDir == TAKEN)
			predictionTable[pcForIndex]++;
		else	
			predictionTable[pcForIndex] = 0b00;
	}	

	if(predDir == TAKEN){
		if(resolveDir == TAKEN)
			predictionTable[pcForIndex] = 0b11;
		else
			predictionTable[pcForIndex]--;	
	}	
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

void InitPredictor_2level() {

}

bool GetPrediction_2level(UINT32 PC) {

  return TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

}

/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////

void InitPredictor_openend() {

}

bool GetPrediction_openend(UINT32 PC) {

  return TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

}

