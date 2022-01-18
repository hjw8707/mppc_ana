#ifndef ANAMULT_HH
#define ANAMULT_HH

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

class AnaMult { //: public TObject {

public:
  AnaMult();
  AnaMult(Int_t _nRun, const char* _runTitle = NULL);
  ~AnaMult();

  void LoadRun(Int_t _nRun, const char* _runTitle = NULL);
  void LoadTree();
  void LoadAlias();

  inline void SetGains(Double_t _gain0, Double_t _gain1, Double_t _gain2, Double_t _gain3) {
    gain[0] = _gain0;
    gain[1] = _gain1;
    gain[2] = _gain2;
    gain[3] = _gain3;
    LoadAlias();}
  
  void DrawGlobalTitle(const char *sub = NULL);

  void FitSpectrum(TH1* hist, Double_t sig = 50, Bool_t drawFlag = true);
  void FitAllChannel(Double_t sig = 50);
  void FitSums(Double_t sig = 50);
  
public:
  Int_t nRun;
  TString runName;
  TString runTitle;
  
  TFile *file;
  TTree *tree;

  ////////////////////////////////////////////////////////////
  // MultiTree Branches
  Int_t       nCh;
  Long64_t   time[4];
  Int_t       energy[4];
  ////////////////////////////////////////////////////////////

  Double_t gain[4];
};

#endif
