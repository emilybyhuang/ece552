#include "predictor.h"
#include <vector>
#include <bitset>
#include <unordered_map>
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
#define THIRTYTWO_BIT_MASK 0XFFFFFFFF
#define FOURTYEIGHT_BIT_MASK 0XFFFFFFFFFFFF
#define SIXTYFOUR_BIT_MASK 0XFFFFFFFFFFFFFFFF 



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


/////////////////////////////////////////////////////////////
// openended
/////////////////////////////////////////////////////////////


#define NUM_OE_PREDICTOR_TABLES 11
#define NUM_ENTRIES_OE_PREDICTOR_TABLE 2048
#define OE_PREDICTOR_TABLES_INDEX_WIDTH 11  //2^11 = 2048


// 64 bit
unsigned long long g_bhr_top;
unsigned long long g_bhr_middle;
unsigned long long g_bhr_bottom;
// 9 bit
int32_t g_aliasing_counter;
// 7 bit
int32_t g_threshold_counter;
UINT32 g_threshold;
int32_t g_sum_of_table_entries;

vector<set<int>> set_of_indices(NUM_OE_PREDICTOR_TABLES);
std::vector<std::vector<int>> predictor_table(NUM_OE_PREDICTOR_TABLES, std::vector <int>(NUM_ENTRIES_OE_PREDICTOR_TABLE) );
std::vector<char> tag_bits(NUM_ENTRIES_OE_PREDICTOR_TABLE/2, 0);

void InitPredictor_openend() {
	//cout << "initialize" << endl;
	for(UINT32 i = 0; i < NUM_OE_PREDICTOR_TABLES; i++){
		// init all to -1 very weak not taken
		for(UINT32 j = 0; j < NUM_ENTRIES_OE_PREDICTOR_TABLE; j++){
			predictor_table[i][j] = 0b0;
		}	
	} 
	g_threshold = 8;
	g_threshold_counter = 0;
	g_sum_of_table_entries = 0;
	g_aliasing_counter = 0;
	g_bhr_top = 0;
	g_bhr_middle = 0;
	g_bhr_bottom = 0;
}

// compress any number with 8 bits to 1 bit
UINT32 Compress_NBitToOneBit(UINT32 history_bits, UINT32 n){
	// i: current set of 
	UINT32 res = 0b0;
	for(UINT32 i = 0; i < n; i++){
		res = res ^ ((history_bits >> i) & 0x1);
	}
	return res;
}


// for any number: find groups of 8 bits to call Compress_EightBitToOneBit
UINT32 Find_NBitToCompress(unsigned long long history_bits, UINT32 num_bits_to_compress, UINT32 num_bits_to_output){
	UINT32 group_size = num_bits_to_compress/8;
	UINT32 res = 0;	

	for(UINT32 i = 0; i < num_bits_to_output; i++){
		res = (res << 1)| Compress_NBitToOneBit( history_bits >> (i * group_size) , group_size);
	}
	return res;
}


UINT32 GetPredictor_Index(UINT32 PC, UINT32 i){
	UINT32 temp = 0;
	UINT32 parameter = 0;
	UINT64 parameter1 = 0, parameter2 = 0, parameter3 = 0;
	switch(i){
		case 0:
			return (g_bhr_bottom & ELEVEN_BIT_MASK) ^ (PC & 0X11);
			
		case 1:
			return (g_bhr_bottom & 0X7) ^ (PC & ELEVEN_BIT_MASK);

		case 2:
			return ((g_bhr_bottom & 0x1F) << 5 | (g_bhr_bottom & 0x1F)) ^ (PC & ELEVEN_BIT_MASK);
		
		case 3:
			return ((g_bhr_bottom & EIGHT_BIT_MASK)^(PC & ELEVEN_BIT_MASK));
		
		case 4:
			return (((g_bhr_bottom >> 2) & ELEVEN_BIT_MASK)^(PC & ELEVEN_BIT_MASK));
			
		case 5:
			return (Find_NBitToCompress(g_bhr_bottom & SIXTEEN_BIT_MASK, 16, 8) << 3) | (PC & THREE_BIT_MASK);
		
		case 6:
			return (Find_NBitToCompress(g_bhr_bottom & THIRTYTWO_BIT_MASK, 32, 8) << 3) | (PC & THREE_BIT_MASK);
		case 7:
			return (Find_NBitToCompress(g_bhr_bottom & FOURTYEIGHT_BIT_MASK, 48, 8) << 3) | (PC & THREE_BIT_MASK);
		
		case 8:	
			parameter1 = (g_bhr_bottom & 0XFFFFFFFFFF) % 2039;
			parameter2 = ((g_bhr_middle & 0XFFFF) << 24 | (g_bhr_bottom >> 40)) % 2039;
			return (parameter1 ^ parameter2) ^ (PC & ELEVEN_BIT_MASK) & ELEVEN_BIT_MASK;
			
		case 9:
			parameter1 = (g_bhr_bottom % 2039) & ELEVEN_BIT_MASK;
			parameter2 = (g_bhr_middle % 2039) & ELEVEN_BIT_MASK;
			return ((parameter1 ^ parameter2) ^ (PC & ELEVEN_BIT_MASK)) & ELEVEN_BIT_MASK;	
			
		case 10:
			parameter1 = (g_bhr_top % 2029) & ELEVEN_BIT_MASK;
			parameter2 = (g_bhr_middle % 2029) & ELEVEN_BIT_MASK;
			parameter3 = (g_bhr_bottom % 2029) & ELEVEN_BIT_MASK;
			return ((parameter1 ^ parameter2 ^ parameter3) ^ (PC & ELEVEN_BIT_MASK)) & ELEVEN_BIT_MASK;
			
		default:
			return PC & ELEVEN_BIT_MASK;
	}
}

bool GetPrediction_openend(UINT32 PC) {
	g_sum_of_table_entries = 0;
	for(UINT32 i = 0; i < NUM_OE_PREDICTOR_TABLES; i++){
		UINT32 index = GetPredictor_Index(PC, i);
		set_of_indices[i].insert(index);
		g_sum_of_table_entries += (predictor_table[i][index]);
	} 

	if(g_sum_of_table_entries + NUM_OE_PREDICTOR_TABLES / 2 >= 0)
		return TAKEN;
	else
		return NOT_TAKEN;
}


void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	if(resolveDir != predDir || abs(g_sum_of_table_entries) <= g_threshold){
		if(resolveDir == TAKEN){
			for(UINT32 i = 0; i < NUM_OE_PREDICTOR_TABLES; i++){
				UINT32 index = GetPredictor_Index(PC, i);
				if(predictor_table[i][index] != 15)
					++predictor_table[i][index];
			}
		}else{
			for(UINT32 i = 0; i < NUM_OE_PREDICTOR_TABLES; i++){
				UINT32 index = GetPredictor_Index(PC, i);
				if(predictor_table[i][index] != -16)
					--predictor_table[i][index];
			}
		}
	}
	
	// threshold counter update
	if(predDir != resolveDir){
		g_threshold_counter++;
		if(g_threshold_counter == 63){
			g_threshold++;
			g_threshold_counter = 0;
		}
	}
	

	if((predDir == resolveDir) && (abs(g_sum_of_table_entries) <= g_threshold)){
		g_threshold_counter--;
		if(g_threshold_counter == -64){
			g_threshold--;
			g_threshold_counter = 0;
		}
	}
	

	unsigned long long bottomHighestBit = (g_bhr_bottom & TOP_BIT_MASK) >> 63;
	unsigned long long middleHighestBit = (g_bhr_middle & TOP_BIT_MASK) >> 63;
	if(resolveDir == TAKEN){
		g_bhr_bottom = (g_bhr_bottom << 1) | ((unsigned long long) 1);
		g_bhr_middle = (g_bhr_middle << 1)| bottomHighestBit;
		g_bhr_top = (g_bhr_top << 1) | middleHighestBit;
	}else{
		g_bhr_bottom = (g_bhr_bottom << 1) | ((unsigned long long) 0);
		g_bhr_middle = (g_bhr_middle << 1)| bottomHighestBit;
		g_bhr_top = (g_bhr_top << 1) | middleHighestBit;
	}
}

void print_set(){
	cout << endl;
	for(auto s: set_of_indices){
		cout << s.size() << endl;
	}
}
