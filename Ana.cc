#include "Ana.hh"

#include "TSystemDirectory.h"
#include "TSpectrum.h"
#include "TF1.h"
#include "TPaveLabel.h"
#include "TCanvas.h"

Ana::Ana() {}

Ana::Ana(Int_t _nRun, const char* _runTitle, Bool_t mult):
  file(NULL), tree(NULL),
  fileCoin(NULL), treeCoin(NULL),
  noWave(true), loadMult(mult), wave(NULL) {
  LoadRun(_nRun, _runTitle);}

Ana::~Ana() {
  if (wave) delete wave;
  if (file) delete file;
  if (fileCoin) delete fileCoin;
  if (fileMult) delete fileMult; }

void Ana::LoadRun(Int_t _nRun, const char* _runTitle) {
  nRun = _nRun;
  if (_runTitle) runTitle = _runTitle;
  else runTitle.Clear();

  ////////////////////////////////////////////////////////////
  // obtaining a run name from the run number
  TSystemDirectory *tsd = new TSystemDirectory("tsd",".");
  runName.Clear();
  for (const auto&& obj: *tsd->GetListOfFiles()) {
    if (!obj->IsFolder()) continue;
    TString _runName(obj->GetName());
    if (_runName.BeginsWith(Form("run%04d",nRun))) {
      runName = _runName;
      break;}}
  delete tsd;
  
  if (runName.IsNull()) {
    std::cerr << "No such a run can be found." << std::endl;
    return; }
  ////////////////////////////////////////////////////////////
  
  tsd = new TSystemDirectory("tsd",Form("%s/UNFILTERED",runName.Data()));
  for (const auto&& obj: *tsd->GetListOfFiles()) {
    if (obj->IsFolder()) continue;
    TString fileName(obj->GetName());
    if (!loadMult && fileName.Contains(Form("Data_%s",runName.Data()))) {
      file = new TFile(Form("%s/%s",tsd->GetTitle(),fileName.Data()));
      if (!file->IsOpen()) {
	std::cerr << "No such a root file can be found." << std::endl;
	return; }
      tree = static_cast<TTree*>(file->Get("Data"));
      std::cout << "Tree loaded." << std::endl;
      LoadTree();
      LoadCoinFile();
      break;}
    if (loadMult && fileName.Contains(Form("Mult_%s",runName.Data()))) {
      fileMult = new TFile(Form("%s/%s",tsd->GetTitle(),fileName.Data()));
      if (!fileMult->IsOpen()) {
	std::cerr << "No such a root file can be found." << std::endl;
	return; }
      treeMult = static_cast<TTree*>(fileMult->Get("mult"));
      std::cout << "Mult. Tree loaded." << std::endl;
      LoadMultTree();
      break;}}
  delete tsd;}

void Ana::LoadTree() {
   Samples = 0;

   if (!tree) return;
   tree->SetBranchAddress("Channel", &Channel);
   tree->SetBranchAddress("Timestamp", &Timestamp);
   tree->SetBranchAddress("Board", &Board);
   tree->SetBranchAddress("Energy", &Energy);
   if (tree->FindBranch("EnergyShort"))
     tree->SetBranchAddress("EnergyShort", &EnergyShort);
   tree->SetBranchAddress("Flags", &Flags);
   if (tree->FindBranch("Samples")) {
     noWave = false;
     tree->SetBranchAddress("Samples", &Samples);}
   std::cout << "Tree branches set." << std::endl;}

void Ana::LoadCoinFile() {
  TSystemDirectory* tsd = new TSystemDirectory("tsd",Form("%s/UNFILTERED",runName.Data()));
  for (const auto&& obj: *tsd->GetListOfFiles()) {
    if (obj->IsFolder()) continue;
    TString fileName(obj->GetName());
    if (fileName.Contains(Form("Coin_%s",runName.Data()))) {
      fileCoin = new TFile(Form("%s/%s",tsd->GetTitle(),fileName.Data()));
      if (!fileCoin->IsOpen()) {
	std::cerr << "No such a root file for coin can be found." << std::endl;
	return; }
      treeCoin = static_cast<TTree*>(fileCoin->Get("Coin"));
      std::cout << "Tree for coin. loaded." << std::endl;
      LoadCoinTree();
      tree->AddFriend(treeCoin);
      break;}}
  delete tsd;}
   
void Ana::LoadCoinTree() {
   if (!treeCoin) return;
   treeCoin->SetBranchAddress("LastCoin", &flagLastCoin);
   treeCoin->SetBranchAddress("NCoin", &nCoin);
   std::cout << "Tree branches for coin. set." << std::endl;}

void Ana::LoadMultTree() {
  if (!treeMult) return;
  treeMult->SetBranchAddress("nCh", &nChannel);
  treeMult->SetBranchAddress("time", Timestamps);
  treeMult->SetBranchAddress("energy", Energies);
  std::cout << "Tree branches for mult. set." << std::endl;}


Int_t Ana::GetEntry(Long64_t entry) {
  if (!tree) return 0;
  return tree->GetEntry(entry);}

void Ana::MakeCoin(ULong64_t tWidth) {
  if (fileCoin) delete fileCoin;
  
  TString path(Form("%s/UNFILTERED/Coin_%s.root",runName.Data(), runName.Data()));
  fileCoin = new TFile(path.Data(),"RECREATE");

  treeCoin = new TTree("Coin","Coin");
  treeCoin->Branch("LastCoin", &flagLastCoin, "LastCoin/O");
  treeCoin->Branch("NCoin", &nCoin, "NCoin/I");

  ULong64_t tRef = 0;
  flagLastCoin = false;
  nCoin = 0;
  
  for (Int_t i = 0 ; i < tree->GetEntries() ; i++) {
    GetEntry(i);
    cout << "\r Event #: " << i;
    cout.flush();

    if (tRef == 0 || (Timestamp - tRef) > tWidth) {
      for (Int_t j = 0 ; j < nCoin ; j++) {
	flagLastCoin = j >= (nCoin - 1);
	treeCoin->Fill();}
      tRef = Timestamp;
      flagLastCoin = false; nCoin = 1;}
    else {
      nCoin++; }}
  for (Int_t j = 0 ; j < nCoin ; j++) {
    flagLastCoin = j < (nCoin - 1);
    treeCoin->Fill();}
	
  treeCoin->Write(0,TObject::kOverwrite);
  tree->AddFriend(treeCoin);
  LoadCoinTree();}

TGraph* Ana::DrawWave(Int_t _iEv, Bool_t flagBL, const char* option) {

  if (noWave) return NULL;
  GetEntry(_iEv);

  if (wave) delete wave;
  wave = new Wave(2, Samples->GetArray(), Samples->GetSize());
  double baseLine = wave->SetBaseLine(50, 0);
  wave->SetTrigTiming(500);
  wave->SetGate(30, 400);
  TGraph* gr = wave->GetGraph();
  gr->SetTitle(";Time [ns];ADC [ch]");
  gr->Draw(option);
  gPad->GetCanvas()->Update();

  cout << "Event # = " << _iEv << endl;
  cout << "Base Line   = " << baseLine << endl;
  cout << "Peak Height = " << wave->GetPeakHeight() << endl;
  cout << "Peak Area   = " << wave->GetPeakArea() << endl;

  return gr;}

void Ana::Loop() {
  TH1D *h_ph = new TH1D("h_ph","Peak Height",4000,0,20000);
  TH1D *h_pa = new TH1D("h_pa","Peak Area",4000,0,4000000);
  TH2D *h_phpa = new TH2D("h_phpa","Peak Height vs Peak Area",4000,0,20000,4000,0,4000000);
  
  for (Int_t i = 0 ; i < tree->GetEntries() ; i++) {
    GetEntry(i);
    cout << "\r Event #: " << i;
    cout.flush();
    
    if (wave) delete wave;
    wave = new Wave(2, Samples->GetArray(), Samples->GetSize());
    wave->SetBaseLine(50, 0);
    wave->SetTrigTiming(500);
    wave->SetGate(30, 400);
    Double_t peakHeight = wave->GetPeakHeight();
    Double_t peakArea   = wave->GetPeakArea  ();

    if (peakHeight > 0 && peakArea > 0) {
      h_ph->Fill(peakHeight);
      h_pa->Fill(peakArea);
      h_phpa->Fill(peakHeight, peakArea);}}}

void Ana::Loop2() {
  TH1D *h_ph = new TH1D("h_ph","Peak Height",4000,0,20000);
  TH1D *h_pa = new TH1D("h_pa","Peak Area",4000,0,4000000);
  TH2D *h_phpa = new TH2D("h_phpa","Peak Height vs Peak Area",4000,0,20000,4000,0,4000000);

  double gain[4] = { 1.08836, 1.63571, 0.95106, 1.87630 };
  
  for (Int_t i = 0 ; i < tree->GetEntries() ; i++) {
    GetEntry(i);
    cout << "\r Event #: " << i;
    cout.flush();

    if (nCoin != 4) continue;

    if (!wave) {// 'wave' exist, accumulating
      wave = new Wave(2, Samples->GetArray(), Samples->GetSize());
      wave->Scale(1./gain[Channel]); }
    else {
      Wave tempWave(2, Samples->GetArray(), Samples->GetSize());
      tempWave.Scale(1./gain[Channel]);
      wave->AddWave(tempWave);}

    if (flagLastCoin) {
      wave->SetBaseLine(50, 0);
      wave->SetTrigTiming(2000);
      wave->SetGate(30, 400);
      Double_t peakHeight = wave->GetPeakHeight();
      Double_t peakArea   = wave->GetPeakArea  ();

      if (peakHeight > 0 && peakArea > 0) {
	h_ph->Fill(peakHeight);
	h_pa->Fill(peakArea);
	h_phpa->Fill(peakHeight, peakArea);}
      delete wave;
      wave = NULL;}}}

void Ana::Loop3(Double_t* gain) {

  TH1D *h_ph = new TH1D("h_ph","Peak Height",4000,0,16000);

  TH1D *h_phch[4];
  for (Int_t i = 0 ; i < 4 ; i++)
    h_phch[i] = new TH1D(Form("h_phch%d",i),
			 "Peak Height",
			 4000,0,4000);
  
  Double_t eSum = 0;
  for (Int_t i = 0 ; i < tree->GetEntries() ; i++) {
    GetEntry(i);
    cout << "\r Event #: " << i;
    cout.flush();

    if (nCoin != 4) continue;

    eSum += Energy * gain[Channel];
    h_phch[Channel]->Fill(Energy * gain[Channel]);
    if (flagLastCoin) {
      h_ph->Fill(eSum);
      eSum = 0;}}

  TCanvas *c1 = new TCanvas;
  c1->Divide(3,2);
  
  for (Int_t i = 0 ; i < 4 ; i++) {
    c1->cd(i+1);
    h_phch[i]->Draw();}
  
  c1->cd(5);
  h_ph->Draw();}


void Ana::FitSpectrum(TH1* hist, Double_t sig, Bool_t drawFlag, Double_t lowR, Double_t uppR, Int_t maxPeak, Double_t nSig, Int_t bgSel) {

  hist->SetAxisRange(lowR, uppR);
  ////////////////////////////////////////
  // Rough fit with TSpectrum
  TSpectrum ts;
  ts.Search(hist,sig);

  Int_t nPeaks = ts.GetNPeaks();
  Double_t* posPeaks = ts.GetPositionX();

  if (nPeaks <= 0) {
    std::cerr << "No peak found." << std::endl;
    return; }
  ////////////////////////////////////////

  ////////////////////////////////////////
  // peak loop
  for (Int_t i = 0 ; i < std::min(nPeaks, maxPeak)  ; i++) {
    //////////////////////
    // rough fit wit gaus
    Int_t fitRes = hist->Fit("gaus",0,0,
			     posPeaks[i] - sig,
			     posPeaks[i] + sig);
    if (fitRes != 0) {
      std::cerr << "Rough fit error for the peak ";
      std::cerr << i << "." <<  std::endl;
      continue; }

    Double_t fitParGaus[3];
    fitParGaus[0] = hist->GetFunction("gaus")->GetParameter(0);
    fitParGaus[1] = hist->GetFunction("gaus")->GetParameter(1);
    fitParGaus[2] = hist->GetFunction("gaus")->GetParameter(2);

    Double_t fitParBG[50];
    if (bgSel == 0) { // polynomial BG
      fitRes = hist->Fit("pol2","Q0",0,
			 posPeaks[i] - nSig*fitParGaus[2],
			 posPeaks[i] + nSig*fitParGaus[2]);
      if (fitRes != 0) {
	std::cerr << "Rough fit error for the peak ";
	std::cerr << i << "." <<  std::endl;
	continue; }
      for (Int_t j = 0 ; j < 3 ; j++) 
	fitParBG[j] = hist->GetFunction("pol2")->GetParameter(j);}

    if (bgSel == 1) { // exponential BG
      fitRes = hist->Fit("expo","Q0",0,
			 posPeaks[i] - nSig*fitParGaus[2],
			 posPeaks[i] + nSig*fitParGaus[2]);
      if (fitRes != 0) {
	std::cerr << "Rough fit error for the peak ";
	std::cerr << i << "." <<  std::endl;
	continue; }
      fitParBG[0] = hist->GetFunction("expo")->GetParameter(0);
      fitParBG[1] = hist->GetFunction("expo")->GetParameter(1);
      
      fitRes = hist->Fit("expo","Q0",0,
			 posPeaks[i] - nSig*fitParGaus[2],
			 posPeaks[i] + nSig*fitParGaus[2]);
      if (fitRes != 0) {
	std::cerr << "Rough fit error for the peak ";
	std::cerr << i << "." <<  std::endl;
	continue; }
      fitParBG[2] = hist->GetFunction("expo")->GetParameter(0);
      fitParBG[3] = hist->GetFunction("expo")->GetParameter(1) * 2;}

    //////////////////////

    ////////////////////
    // real fit
    TF1 *fresp;
    if (bgSel == 0) {
      fresp = new TF1(Form("resp%d",i),"gaus(0)+pol2(3)",
		      posPeaks[i] - nSig*fitParGaus[2],
		      posPeaks[i] + nSig*fitParGaus[2]);
      fresp->SetParameters(fitParGaus[0],fitParGaus[1],fitParGaus[2],
			   fitParBG[0],fitParBG[1],fitParBG[2]);}
    else {
      fresp = new TF1(Form("resp%d",i),"gaus(0)+expo(3)+expo(5)",
		      posPeaks[i] - nSig*fitParGaus[2],
		      posPeaks[i] + nSig*fitParGaus[2]);
      fresp->SetParameters(fitParGaus[0],fitParGaus[1],fitParGaus[2],
			   fitParBG[0],fitParBG[1],fitParBG[2],fitParBG[3]);}
    fitRes = hist->Fit(fresp,0,0,
		       posPeaks[i] - nSig*fitParGaus[2],
		       posPeaks[i] + nSig*fitParGaus[2]);

    if (fitRes != 0) {
      std::cerr << "Real fit error for the peak ";
      std::cerr << i << "." <<  std::endl;
      continue; }
    ////////////////////

    ////////////////////
    // Resulted functions
    fresp->SetRange(fresp->GetParameter(1) - nSig*fresp->GetParameter(2),
		    fresp->GetParameter(1) + nSig*fresp->GetParameter(2));
    TF1 *fbg;
    if (bgSel == 0) {
      fbg = new TF1(Form("bg%d",i),"pol2",
		    fresp->GetParameter(1) - nSig*fresp->GetParameter(2),
		    fresp->GetParameter(1) + nSig*fresp->GetParameter(2));
      fbg->SetParameters(fresp->GetParameter(3),
			 fresp->GetParameter(4),
			 fresp->GetParameter(5));}
    else {
      fbg = new TF1(Form("bg%d",i),"expo(0)+expo(2)",
		    fresp->GetParameter(1) - nSig*fresp->GetParameter(2),
		    fresp->GetParameter(1) + nSig*fresp->GetParameter(2));
      fbg->SetParameters(fresp->GetParameter(3),
			 fresp->GetParameter(4),
			 fresp->GetParameter(5),
			 fresp->GetParameter(6));}
    
    //////////////////////////////
    // Results
    std::cout << "Peak " << i << " : ";
    std::cout << "Cent. = " << std::setprecision(5) << fresp->GetParameter(1) << ", ";
    std::cout << "Sigma = " << fresp->GetParameter(2) << ", ";
    std::cout << "Reso. = " << (fresp->GetParameter(2)/fresp->GetParameter(1)) << ", ";
    std::cout << "Reso. = " << (fresp->GetParameter(2)/fresp->GetParameter(1)*2.35482);
    std::cout << std::endl;
    //////////////////////////////  
  }
  
  if (!drawFlag) return;
  
  ////////////////////////////////////////
  // Drawing
  if (lowR >= 0 && uppR >= 0) hist->GetXaxis()->SetRangeUser(lowR, uppR);
  if (lowR >= 0 && uppR <  0) hist->GetXaxis()->SetRangeUser(lowR, 4000);
  if (lowR <  0 && uppR <  0) hist->GetXaxis()->SetRangeUser(   0, uppR);
  hist->Draw("Hist");
  for (Int_t i = 0 ; i < std::min(nPeaks, maxPeak) ; i++) {
    TF1* f1 = static_cast<TF1*>(gROOT->GetFunction(Form("resp%d",i)));
    f1->SetLineColor(i+1);
    f1->Draw("SAME");  
    f1 = static_cast<TF1*>(gROOT->GetFunction(Form("bg%d",i)));
    f1->SetLineColor(i+1);
    f1->SetLineStyle(2);
    f1->Draw("SAME");  }
  ////////////////////////////////////////

  //  DrawGlobalTitle();
}

void Ana::DrawGlobalTitle() {
  TPaveLabel pl;
  pl.DrawPaveLabel(0.15,0.92,0.65,0.99,
		   Form("Run%04d: %s",nRun,runTitle.Data()),"NDC");}

void Ana::FitSingleChannel(Double_t sig, Bool_t doubleFlag, Double_t lowR, Double_t uppR, Int_t maxPeak, Double_t nSig, Double_t lowRP, Double_t uppRP) {

  tree->Draw("Energy>>h0(4000,0,4000)",0,"GOFF");
  TH1* h1 = static_cast<TH1*>(gDirectory->Get("h0"));

  if (doubleFlag) FitSpectrumDouble(h1, lowR, uppR, sig, true, lowRP, uppRP, maxPeak, nSig);
  else FitSpectrum(h1, sig, true, lowR, uppR, maxPeak, nSig);
}

void Ana::FitEachChannel(Double_t sig, Double_t lowR, Double_t uppR,
			 Int_t maxPeak, Double_t nSig) {

  TCanvas *c1 = new TCanvas;
  c1->Divide(2,2);

  for (Int_t i = 0 ; i < 4 ; i++) {
    c1->cd(i+1);
    treeMult->Draw(Form("energy[%d]>>h%d(4000,0,4000)",i,i),
		   "nCh == 4", "GOFF");
    TH1* h1 = static_cast<TH1*>(gDirectory->Get(Form("h%d",i)));

    std::cout << "Fit the spectrum for Channel " << i << "." << std::endl;
    FitSpectrum(h1, sig, true, lowR, uppR, maxPeak, nSig, 1);
    std::cout << std::endl;
    DrawGlobalTitle();}


  c1->cd();}

void Ana::MakeMult(ULong64_t tWidth) {
  if (fileMult) delete fileMult;
  
  TString path(Form("%s/UNFILTERED/Mult_%s.root",runName.Data(), runName.Data()));
  fileMult = new TFile(path.Data(),"RECREATE");

  treeMult = new TTree("mult","mult");
  treeMult->Branch("nCh", &nChannel, "nCh/I");
  treeMult->Branch("time", Timestamps, "time[4]/L");
  treeMult->Branch("energy", Energies, "energy[4]/I");

  ULong64_t tRef = 0;

  //////////////////////////////
  // initilize
  nChannel = 0;
  for (Int_t i = 0 ; i < 4 ; i++ ) {
    Timestamps[i] = -1;
    Energies[i] = -1; }
  //////////////////////////////
  
  for (Int_t i = 0 ; i < tree->GetEntries() ; i++) {
    GetEntry(i);
    cout << "\r Event #: " << i;
    cout.flush();

    if (tRef == 0 || (Timestamp - tRef) > tWidth) {
      treeMult->Fill();
      nChannel = 0;
      for (Int_t i = 0 ; i < 4 ; i++ ) {
	Timestamps[i] = -1;
	Energies[i] = -1; }
      tRef = Timestamp; }
    
    Timestamps[Channel] = Timestamp;
    Energies[Channel] = Energy;
    nChannel++; }
  treeMult->Fill();
	
  treeMult->Write(0,TObject::kOverwrite);}

void Ana::FitSpectrumDouble(TH1* hist, Double_t lowR, Double_t uppR, Double_t sig, Bool_t drawFlag, Double_t lowRP, Double_t uppRP, Int_t maxPeak, Double_t nSig, Int_t bgSel) {

  //////////////////////////////
  // Axis range
  hist->SetAxisRange(lowR, uppR);
  ////////////////////////////////////////
  // Rough fit with TSpectrum
  TSpectrum ts;
  ts.Search(hist,sig);

  Int_t nPeaks = ts.GetNPeaks();
  std::cout << nPeaks << std::endl;
  if (nPeaks <= 1) {
    Double_t rr = 0;
    while (nPeaks != 2) {
      std::cout << nPeaks << std::endl;
      rr += 5;
      hist->SetAxisRange(lowR - rr, uppR + rr);
      ts.Search(hist,sig);
      nPeaks = ts.GetNPeaks();
      if (lowR - rr < 0 || uppR + rr > 4000) {
	std::cerr << "No peak found." << std::endl;
	return; }
    }}

  Double_t* posPeaks = ts.GetPositionX();
  ////////////////////////////////////////

  ////////////////////////////////////////
  // peak loop for two peaks

  //////////////////////
  // rough fit wit gaus 
  Int_t fitRes = hist->Fit("gaus","Q0",0, posPeaks[0] - sig, posPeaks[0] + sig);
  if (fitRes != 0) {
    std::cerr << "Rough fit error for the peak." << std::endl;
    return; }

  Double_t fitParGaus0[3];
  for (Int_t i = 0 ; i < 3 ; i++)
    fitParGaus0[i] = hist->GetFunction("gaus")->GetParameter(i);

  fitRes = hist->Fit("gaus","Q0",0, posPeaks[1] - sig, posPeaks[1] + sig);
  if (fitRes != 0) {
    std::cerr << "Rough fit error for the peak." << std::endl;
    return; }

  Double_t fitParGaus1[3];
  for (Int_t i = 0 ; i < 3 ; i++)
    fitParGaus1[i] = hist->GetFunction("gaus")->GetParameter(i);

  fitRes = hist->Fit("pol2","Q0",0, lowR, uppR);
  if (fitRes != 0) {
    std::cerr << "Rough fit error for the peak." << std::endl;
    return; }

  Double_t fitParBG[50];
  if (bgSel == 0) { // polynomial BG
    fitRes = hist->Fit("pol2","Q0",0,lowR, uppR);
    if (fitRes != 0) {
      std::cerr << "Rough fit error for the peak.";
      return; }
    for (Int_t j = 0 ; j < 3 ; j++) 
      fitParBG[j] = hist->GetFunction("pol2")->GetParameter(j);}
  
  if (bgSel == 1) { // exponential BG
    fitRes = hist->Fit("expo","Q0",0,lowR,uppR);
    if (fitRes != 0) {
      std::cerr << "Rough fit error for the peak.";
      return; }
    fitParBG[0] = hist->GetFunction("expo")->GetParameter(0);
    fitParBG[1] = hist->GetFunction("expo")->GetParameter(1);
      
    fitRes = hist->Fit("expo","Q0",0,lowR,uppR);
    if (fitRes != 0) {
      std::cerr << "Rough fit error for the peak.";
      return; }
    fitParBG[2] = hist->GetFunction("expo")->GetParameter(0);
    fitParBG[3] = hist->GetFunction("expo")->GetParameter(1) * 2;}

  //////////////////////

  ////////////////////
  // real fit
  TF1* fresp;
  if (bgSel == 0) {
    fresp = new TF1("resp","gaus(0)+gaus(3)+pol2(6)",lowR,uppR);
    fresp->SetParameters(fitParGaus0[0],fitParGaus0[1],fitParGaus0[2],
			 fitParGaus1[0],fitParGaus1[1],fitParGaus1[2],
			 fitParBG   [0],fitParBG   [1],fitParBG   [2]);}
  else {
    fresp = new TF1("resp","gaus(0)+gaus(3)+expo(6)+expo(8)",lowR,uppR);
    fresp->SetParameters(fitParGaus0[0],fitParGaus0[1],fitParGaus0[2],
			 fitParGaus1[0],fitParGaus1[1],fitParGaus1[2],
			 fitParBG[0],fitParBG[1],fitParBG[2],fitParBG[3]);}

  Double_t lowFitR, uppFitR;
  if (fitParGaus0[1] < fitParGaus1[1]) {
    lowFitR = fitParGaus0[1] - nSig * fitParGaus0[2];
    uppFitR = fitParGaus1[1] + nSig * fitParGaus1[2]; }
  else {
    lowFitR = fitParGaus1[1] - nSig * fitParGaus1[2];
    uppFitR = fitParGaus0[1] + nSig * fitParGaus0[2]; }
  //  fitRes = hist->Fit(fresp,"Q0",0, lowR, uppR);
  fitRes = hist->Fit(fresp,"Q0",0, lowFitR, uppFitR);

  if (fitRes != 0) {
    std::cerr << "Real fit error." << std::endl;
    return; }
  ////////////////////

  ////////////////////
  // Resulted functions
  fresp->SetRange(lowFitR, uppFitR);
  
  TF1 *fbg;
  if (bgSel == 0) {
    fbg = new TF1("bg","pol2", lowFitR, uppFitR);
    fbg->SetParameters(fresp->GetParameter(6),
		       fresp->GetParameter(7),
		       fresp->GetParameter(8));}
  else {
    fbg = new TF1("bg","expo(0)+expo(2)",lowFitR,uppFitR);
    fbg->SetParameters(fresp->GetParameter(6),
		       fresp->GetParameter(7),
		       fresp->GetParameter(8),
		       fresp->GetParameter(9));}

  
  //////////////////////////////
  // Results
  std::cout << "Peak " << 0 << " : ";
  std::cout << "Cent. = " << std::setprecision(5) << fresp->GetParameter(1) << ", ";
  std::cout << "Sigma = " << fresp->GetParameter(2) << ", ";
  std::cout << "Reso. = " << (fresp->GetParameter(2)/fresp->GetParameter(1)) << ", ";
  std::cout << "Reso. = " << (fresp->GetParameter(2)/fresp->GetParameter(1)*2.35482);
  std::cout << std::endl;
  std::cout << "Peak " << 1 << " : ";
  std::cout << "Cent. = " << std::setprecision(5) << fresp->GetParameter(4) << ", ";
  std::cout << "Sigma = " << fresp->GetParameter(5) << ", ";
  std::cout << "Reso. = " << (fresp->GetParameter(5)/fresp->GetParameter(4)) << ", ";
  std::cout << "Reso. = " << (fresp->GetParameter(5)/fresp->GetParameter(4)*2.35482);
  std::cout << std::endl;
  //////////////////////////////  
  
  if (!drawFlag) return;
  
  ////////////////////////////////////////
  // Drawing
  //////////////////////////////
  // Axis range
  hist->SetAxisRange(lowRP, uppRP);
  hist->Draw("Hist");

  fresp->SetLineColor(1);
  fresp->Draw("SAME");
  fbg->SetLineColor(1);
  fbg->SetLineStyle(2);
  fbg->Draw("SAME");
  ////////////////////////////////////////

  DrawGlobalTitle();
}

void Ana::FitChannelSum(Double_t sig, Double_t lowR, Double_t uppR,
			Int_t maxPeak, Double_t nSig, Int_t bgSel,
			Bool_t flagDouble, Bool_t flagGeo) {

  Double_t gain[4] = { 1.000, 1.152, 1.255, 1.134 };

  if (flagGeo)
    treeMult->Draw("(energy[0]*energy[1]*energy[2]*energy[3])**0.25>>hsum(4000,0,4000)",
		   "nCh == 4", "GOFF");
  else
    treeMult->Draw(Form("(energy[0]*%f + energy[1]*%f + energy[2]*%f + energy[3]*%f)/4>>hsum(4000,0,4000)",gain[0],gain[1],gain[2],gain[3]),
		   "nCh == 4", "GOFF");

  TH1* h1 = static_cast<TH1*>(gDirectory->Get("hsum"));

  if (flagDouble) FitSpectrumDouble(h1, lowR, uppR, sig,
				    true, 0, 1400, maxPeak, nSig, bgSel);
  else FitSpectrum(h1, sig, true, lowR, uppR, maxPeak, nSig, bgSel);
  DrawGlobalTitle();}
