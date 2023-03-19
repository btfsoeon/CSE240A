//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"


//
// Student Information
//
const char *studentName = "Alex Park";
const char *studentID   = "A59017234";
const char *email       = "sop010@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;
int selector;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
uint32_t ghist;
uint32_t gmask;
uint32_t lmask;
uint32_t pcmask;

uint32_t *global_pht;
uint32_t *local_pht;
uint32_t *local_bht;
uint32_t *selection_pht;
//


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

uint32_t init_mask(n_bits) {
  uint32_t mask = 0;
  for (int i=0; i<n_bits; i++) {
    mask = mask | 1<<i;
  }
  return mask;
}


// Initialize the predictor
//
void init_predictor()
{
  //
  // Initialize Mask
  //
  gmask = init_mask(ghistoryBits);
  lmask = init_mask(lhistoryBits);
  pcmask = init_mask(pcIndexBits);

  //
  // Initialize Branch Predictor Data Structures
  //
  ghist = 0;
  int tablesize = 0;
  switch(bpType) {
    case GSHARE:
      tablesize = 1<<ghistoryBits;
      global_pht = (uint32_t*) malloc(sizeof(uint32_t)*tablesize);
      for (int i=0; i<tablesize; i++) {
        global_pht[i] = TAKEN;
      }
      break;

    case TOURNAMENT:
      tablesize = 1<<ghistoryBits;
      global_pht = (uint32_t*) malloc(sizeof(uint32_t)*tablesize);
      for (int i=0; i<tablesize; i++) {
        global_pht[i] = TAKEN;
      }
      tablesize = 1<<lhistoryBits;
      local_pht = (uint32_t*) malloc(sizeof(uint32_t)*tablesize);
      for (int i=0; i<tablesize; i++) {
        local_pht[i] = TAKEN;
      }
      tablesize = 1<<pcIndexBits;
      local_bht = (uint32_t*) malloc(sizeof(uint32_t)*tablesize);
      for (int i=0; i<tablesize; i++) {
        local_bht[i] = 0; // miss
      }
      tablesize = 1<<ghistoryBits; // size of selection table is equal to size of global table
      selection_pht = (uint32_t*) malloc(sizeof(uint32_t)*tablesize);
      for (int i=0; i<tablesize; i++) {
        selection_pht[i] = SEL_THRESHOLD; // intialize to threshold (weak taken)
      }
      break;

    case CUSTOM:
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  // Implement prediction scheme
  //
  uint32_t ghistbits;
  uint32_t gindex;
  uint32_t pcbits;
  uint32_t pcindex;
  uint32_t lhist;

  uint32_t selection;
  uint32_t pred;
  uint32_t gpred;
  uint32_t lpred;

  // Make a prediction based on the bpType
  
  switch (bpType) {
    case STATIC:
      return TAKEN;

    case GSHARE:
      pcbits = pc & gmask;
      ghistbits = ghist & gmask;
      gindex = pcbits ^ ghistbits;
      // printf("gindex: %d", gindex);
      pred = global_pht[gindex];
      return pred < PRED_THRESHOLD ? NOTTAKEN : TAKEN;

    case TOURNAMENT:
      ghistbits = ghist & gmask;
      selection = selection_pht[ghistbits];
      if (selection < SEL_THRESHOLD) {
        pcindex = pc & pcmask;
        lhist = local_bht[pcindex] & lmask;
        pred = local_pht[lhist];
      }
      else {
        pred = global_pht[ghistbits];
      }
      return pred < PRED_THRESHOLD ? NOTTAKEN : TAKEN;

    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  uint32_t ghistbits;
  uint32_t gindex;
  uint32_t pcbits;
  uint32_t pcindex;
  uint32_t lhist;

  uint32_t selection;
  uint32_t pred;
  uint32_t gpred;
  uint32_t lpred;

  switch (bpType) {
    case GSHARE:
      pcbits = pc & gmask;
      ghistbits = ghist & gmask;
      gindex = pcbits ^ ghistbits;
      // if (outcome == TAKEN && global_pht[gindex] < ST) {
      //   global_pht[gindex]++;
      // }
      // if (outcome == NOTTAKEN && global_pht[gindex] > SN) {
      //   global_pht[gindex]--;
      // }
      if (outcome == TAKEN) {
        global_pht[gindex] = increment(global_pht[gindex], ST);
      }
      else {
        global_pht[gindex] = decrement(global_pht[gindex], SN);
      }
      ghist = ghist << 1 | outcome;
      break;

    case TOURNAMENT:
      ghistbits = ghist & gmask;
      selection = selection_pht[ghistbits];
      gpred = global_pht[ghistbits];
      gpred = gpred < PRED_THRESHOLD ? NOTTAKEN : TAKEN;
      pcindex = pc & pcmask;
      lhist = local_bht[pcindex] & lmask;
      lpred = local_pht[lhist];
      lpred = lpred < PRED_THRESHOLD ? NOTTAKEN : TAKEN;
      
      // update selection table - 0/1/2/3 values - down to local and up to global
      if (gpred == outcome && lpred != outcome) {
        selection_pht[ghistbits] = increment(selection_pht[ghistbits], SG);
      }
      if (gpred != outcome && lpred == outcome) {
        selection_pht[ghistbits] = decrement(selection_pht[ghistbits], SL);
      }

      // update global/local table
      if (outcome == TAKEN) {
        global_pht[ghistbits] = increment(global_pht[ghistbits], ST);
        local_pht[lhist] = increment(local_pht[lhist], ST);
      }
      else {
        global_pht[ghistbits] = decrement(global_pht[ghistbits], SN);
        local_pht[lhist] = decrement(local_pht[lhist], SN);
      }

      ghist = ((ghist<<1) | outcome) & gmask;
      local_bht[pcindex] = ((local_bht[pcindex]<<1) | outcome) & lmask;
      break;

    case CUSTOM:
    default:
      break;
  }

}

uint32_t increment(uint32_t value, uint32_t bound) {
  return value < bound? value+1 : bound;
}

uint32_t decrement(uint32_t value, uint32_t bound) {
  return value > bound? value-1 : bound;
}
