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
  Ana(Int_t _nRun, const char* _runTitle = NULL);
  ~Ana();

  void LoadRun(Int_t _nRun, const char* _runTitle = NULL);
  void LoadTree();

  void LoadCoinFile();
  void MakeCoin(ULong64_t tWidth); // in [ps]
  void LoadCoinTree();

  void MakeMult(ULong64_t tWidth); // in [ps]
  
  Int_t GetEntry(Long64_t entry); 
  TGraph* DrawWave(Int_t _iEv, Bool_t flagBL = false, const char* option = "AC");

  void Loop();
  void Loop2();
  void Loop3(Double_t* gain);  

  void DrawSpectra();
  void FitSpectrum(TH1* hist, Double_t sig = 50, Bool_t drawFlag = true,
		   Double_t lowR = -1, Double_t uppR = -1, Int_t maxPeak = 100,
		   Int_t nSig = 3);
  void FitSpectrumDouble(TH1* hist, Double_t lowR, Double_t uppR,
			 Double_t sig = 50, Bool_t drawFlag = true,
			 Double_t lowRP = -1, Double_t uppRP = -1,
			 Int_t maxPeak = 100, Int_t nSig = 3);

  void DrawGlobalTitle();

  void FitSingleChannel(Double_t sig = 50, Bool_t doubleFlag = false,
			Double_t lowR = -1, Double_t uppR = -1, Int_t maxPeak = 100,
			Int_t nSig = 3, Double_t lowRP = -1, Double_t uppRP = -1);
  void FitEachChannel(Double_t sig = 50);
  
public:
  Int_t nRun;
  TString runName;
  TString runTitle;
  
  TFile *file;
  TTree *tree;

  TFile *fileCoin;
  TTree *treeCoin;

  TFile *fileMult;
  TTree *treeMult;
  
  Bool_t noWave;
  
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

  ////////////////////////////////////////////////////////////
  // MultiTree Branches
  Int_t nChannel;
  ULong64_t       Timestamps[4];
  Int_t        Energies[4];
  ////////////////////////////////////////////////////////////

  Wave* wave;
};

#endif
