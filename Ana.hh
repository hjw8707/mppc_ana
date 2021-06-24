#ifndef ANA_HH
#define ANA_HH

#include "TGraph.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TMath.h"
#include "TPad.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "Math/Interpolator.h"
#include "TLine.h"

#include <iostream>

#include "Wave.hh"

using namespace std;

class Ana { //: public TObject {

public:
  Ana();
  Ana(Int_t _nRun);
  ~Ana();

  void LoadRun(Int_t _nRun);
  void LoadTree();

  void MakeCoin(ULong64_t tWidth);
  void LoadCoinTree();
  
  Int_t GetEntry(Long64_t entry); 
  TGraph* DrawWave(Int_t _iEv, Bool_t flagBL = false, const char* option = "AC");

  void Loop();
  void Loop2();
  
public:
  Int_t nRun;
  
  TFile *file;
  TTree *tree;

  TFile *fileCoin;
  TTree *treeCoin;
  
  ////////////////////////////////////////////////////////////
  // Tree Branches
  UShort_t        Channel;
  ULong64_t       Timestamp;
  UShort_t        Board;
  UShort_t        Energy;
  UShort_t        EnergyShort;
  UInt_t          Flags;
  TArrayS         *Samples;
  ////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////
  // CoinTree Branches
  Bool_t flagLastCoin;
  Int_t nCoin;
  ////////////////////////////////////////////////////////////
  
  Wave* wave;
};

#endif
