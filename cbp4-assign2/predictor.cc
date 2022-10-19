#include "predictor.h"
#include <vector>
#include <bitset>
/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////

// array to hold all of the prections
// since prediction tables have 8192 bits, each entry is 2 bits => 8192/2=4096
#define NUM_ENTRIES_TWOBIT_SAT 4096
// 4096 is 2 ^ 12  thus use this bit mask to get lowest 12 bits of pc
#define TWELVE_BIT_MASK 0xFFF 
#define TWO_BIT_MASK 0X3
#define THREE_BIT_MASK 0x7
#define FOUR_BIT_MASK 0xF  //COUNTER 'char'
#define SIX_BIT_MASK 0x3F
#define EIGHT_BIT_MASK 0XFF
#define NINE_BIT_MASK 0XFF8
#define SIXTEEN_BIT_MASK 0xFFFF


#define INDEX_MASK 0x3F
#define PHR_MASK 0X20
#define TOP_BIT_MASK 0X80000000
#define ELEVEN_BIT_MASK 0X7FF

UINT32 predictionTable_2bitsat[NUM_ENTRIES_TWOBIT_SAT];

void InitPredictor_2bitsat() {
	// init all entries to weak not-taken
	std::fill_n(predictionTable_2bitsat, NUM_ENTRIES_TWOBIT_SAT, 0b01);	
}

bool GetPrediction_2bitsat(UINT32 PC) {
	//get the lowest 12 bits to index into the global array
	UINT32 index = PC & TWELVE_BIT_MASK;
	UINT32 prediction = predictionTable_2bitsat[index];
	// if second lowest bit is 1: TAKEN	
	if(prediction >> 1)
		return TAKEN;
	else
		return NOT_TAKEN;
}

void update_2bitsat(bool resolveDir, bool predDir, UINT32 * table, UINT32 index){
	if(resolveDir == TAKEN){
		if(table[index] != 3)
			++table[index];
	}else{	
		if(table[index] != 00)
			--table[index];
	}	
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	// get the lowest 12 bit to index into global array
	UINT32 index = PC & TWELVE_BIT_MASK;
	update_2bitsat(resolveDir, predDir, predictionTable_2bitsat, index);
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

#define NUM_ENTRIES_PRIVATE_HISTORY_TABLE 512
#define NUM_PRIVATE_PREDICTOR_TABLES 8
#define NUM_ENTRIES_PRIVATE_PREDICTOR_TABLE 64



UINT32 privateHistoryTable_2level[NUM_ENTRIES_PRIVATE_HISTORY_TABLE];
UINT32 privatePredictorTable_2level[NUM_PRIVATE_PREDICTOR_TABLES][NUM_ENTRIES_PRIVATE_PREDICTOR_TABLE];

void InitPredictor_2level() {
	// init all history to 0
	std::fill_n(privateHistoryTable_2level, NUM_ENTRIES_PRIVATE_HISTORY_TABLE, 0);	

	// init all entries to weak not-taken
	for(int i = 0; i < NUM_PRIVATE_PREDICTOR_TABLES; i++){
		std::fill_n(privatePredictorTable_2level[i], NUM_ENTRIES_PRIVATE_PREDICTOR_TABLE, 0b01);	
	}
}

bool GetPrediction_2level(UINT32 PC) {
	UINT32 bht_index = PC & NINE_BIT_MASK;
	bht_index = bht_index >> 3;
	UINT32 pht_index = PC & THREE_BIT_MASK;

	UINT32 history = privateHistoryTable_2level[bht_index] & SIX_BIT_MASK;
	UINT32 prediction = privatePredictorTable_2level[pht_index][history];
	if(prediction >> 1)
		return TAKEN;
	else
  		return NOT_TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	UINT32 bht_index = PC & NINE_BIT_MASK;
	bht_index = bht_index >> 3;
	UINT32 pht_index = PC & THREE_BIT_MASK;

	UINT32 history = privateHistoryTable_2level[bht_index] & SIX_BIT_MASK;

	// update predictor table
	update_2bitsat(resolveDir, predDir, privatePredictorTable_2level[pht_index], history);

	// update history
	privateHistoryTable_2level[bht_index] = privateHistoryTable_2level[bht_index] << 1 | resolveDir;
}

#define NUM_OE_PREDICTOR_TABLES 8
#define NUM_ENTRIES_OE_PREDICTOR_TABLE 2048

#define OE_PREDICTOR_TABLES_INDEX_WIDTH 11  //2^11 = 2048

// 5 bit: 1 bit is sign 
// 4 bit unsigned ++ -- 
// go pass 0: check to update sign

// 1 negative 0 positive
// look at top bit for +/- to take / not take


//unsigned long long int g_bhr [2]; 
std::vector<bool> phr(16);
// std::vector<bool> g_bhr(128);

// 64 bit
unsigned long long g_bhr_top;
unsigned long long g_bhr_bottom;
std::vector<std::vector<char>> predictor_table(NUM_OE_PREDICTOR_TABLES, std::vector <char>(NUM_ENTRIES_OE_PREDICTOR_TABLE) );

void InitPredictor_openend() {
	for(UINT32 i = 0; i < NUM_OE_PREDICTOR_TABLES; i++){
		// init all to -1 very weak not taken
		for(UINT32 j = 0; j < NUM_ENTRIES_OE_PREDICTOR_TABLE; j++){
			predictor_table[i][j] = 0b1111;
		}	
	} 
}

void print_binary(UINT32 PC){
	std::cout << "a = " << std::bitset<64>(PC)  << std::endl;
}

// compress any number with 8 bits to 1 bit
UINT32 Compress_NBitToOneBit(UINT32 history_bits, UINT32 n){
	// i: current set of 8
	UINT32 res = 0;
	for(UINT32 i = 0; i < n; i++){
		res ^= (history_bits >> i);
	}
	return res;
}

// 11100010101010
// 00000011100010

// for any number: find groups of 8 bits to call Compress_EightBitToOneBit
UINT32 Find_NBitToCompress(UINT32 history_bits, UINT32 num_bits_to_compress){
	UINT32 group_size;
	UINT32 res = 0;
	switch(num_bits_to_compress){
		case 16:
			group_size = 2;
			break;
		case 32:
			group_size = 4;
			break;
		case 64:
			group_size = 8;
			break;
		default:
			group_size = 1000;
	}

	// groups = [0...3]
	for(int i = 0; i < 8; i++){
		//
		res = (res << 1)| Compress_NBitToOneBit( history_bits >> (i * group_size) , group_size);
	}
	return res;
}


UINT32 GetPredictor_Index(UINT32 PC, UINT32 i){
	switch(i){
		case 0:
			return PC & ELEVEN_BIT_MASK;
			
		case 1:
			return (PC & ELEVEN_BIT_MASK) ^ (g_bhr_bottom & TWO_BIT_MASK);
			
		case 2:
			return (PC & ELEVEN_BIT_MASK) ^ (g_bhr_bottom & FOUR_BIT_MASK);
		
		case 3:
			return (PC & ELEVEN_BIT_MASK) ^ (g_bhr_bottom & EIGHT_BIT_MASK);
		
		case 4:
			return (Find_NBitToCompress(g_bhr_bottom & SIXTEEN_BIT_MASK, 16) << 3 )| ((PC >> 2) & THREE_BIT_MASK);
			
		case 5:
			return (Find_NBitToCompress(g_bhr_bottom & SIXTEEN_BIT_MASK, 32) << 3)| ((PC >> 2) & THREE_BIT_MASK);
		
		case 6:
			return (Find_NBitToCompress(g_bhr_bottom & SIXTEEN_BIT_MASK, 32) << 3) | ((PC >> 2) & THREE_BIT_MASK);
			
		case 7:
			return (Find_NBitToCompress(g_bhr_bottom & SIXTEEN_BIT_MASK, 64) << 3) | ((PC >> 2) & THREE_BIT_MASK);

		default:
			return PC & ELEVEN_BIT_MASK;
	}
}

bool GetPrediction_openend(UINT32 PC) {
//access every table and get the value of counter
//sum up the values and return the prediction result

	UINT32 sum = 0;
	// TODO: define INDEX_MASK and num bits to shift later
	
	for(UINT32 i = 0; i < NUM_OE_PREDICTOR_TABLES; i++){
		cout << "i: " << i << endl;
		cout << "PC: ";
		print_binary(PC);
		UINT32 index = GetPredictor_Index(PC, i);
		cout << "index: ";
		print_binary(index);
		if(i == 5)
			break;
		sum += (predictor_table[i][index] & FOUR_BIT_MASK);
	} 

	UINT32 phr_bit = PC & PHR_MASK >> 6;
	phr.erase(phr.begin());
	phr.push_back(phr_bit);
	return TAKEN;
}


void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	if(resolveDir == TAKEN){
		for(UINT32 i = 0; i < NUM_OE_PREDICTOR_TABLES; i++){
			UINT32 index = GetPredictor_Index(PC, i);
			if(predictor_table[i][index] != 7)
				++predictor_table[i][index];
		}
	}else{
		for(UINT32 i = 0; i < NUM_OE_PREDICTOR_TABLES; i++){
			UINT32 index = GetPredictor_Index(PC, i);
			if(predictor_table[i][index] != -8)
				--predictor_table[i][index];
		}
	}

	bool highestBit = g_bhr_bottom & (TOP_BIT_MASK >> 63);
	g_bhr_top = g_bhr_top << 1 | highestBit;
	g_bhr_bottom = g_bhr_bottom << 1 | predDir;
}