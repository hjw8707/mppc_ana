#include "Ana.hh"

#include "TSystemDirectory.h"
#include "TSpectrum.h"
#include "TF1.h"
#include "TPaveLabel.h"
#include "TCanvas.h"

Ana::Ana() {}

Ana::Ana(Int_t _nRun, const char* _runTitle):
  file(NULL), tree(NULL),
  fileCoin(NULL), treeCoin(NULL),
  noWave(true), wave(NULL) {
  LoadRun(_nRun, _runTitle);}

Ana::~Ana() {
  if (wave) delete wave;
  if (file) delete file;
  if (fileCoin) delete fileCoin; }

void Ana::LoadRun(Int_t _nRun, const char* _runTitle) {
  nRun = _nRun;
  if (_runTitle) runTitle = _runTitle;
  else runTitle.Clear();
  
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

  tsd = new TSystemDirectory("tsd",Form("%s/UNFILTERED",runName.Data()));
  for (const auto&& obj: *tsd->GetListOfFiles()) {
    if (obj->IsFolder()) continue;
    TString fileName(obj->GetName());
    if (fileName.Contains(Form("Data_%s",runName.Data()))) {
      file = new TFile(Form("%s/%s",tsd->GetTitle(),fileName.Data()));
      if (!file->IsOpen()) {
	std::cerr << "No such a root file can be found." << std::endl;
	return; }
      tree = static_cast<TTree*>(file->Get("Data"));
      std::cout << "Tree loaded." << std::endl;
      LoadTree();
      LoadCoinFile();
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


void Ana::FitSpectrum(TH1* hist, Double_t sig, Bool_t drawFlag) {

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
  for (Int_t i = 0 ; i < nPeaks ; i++) {
    //////////////////////
    // rough fit wit gaus
    Int_t fitRes = hist->Fit("gaus","Q0",0,
			     posPeaks[i] - sig,
			     posPeaks[i] + sig);
    if (fitRes != 0) {
      std::cerr << "Rough fit error for the peak ";
      std::cerr << i << "." <<  std::endl;
      continue; }

    Double_t* fitParGaus;
    fitParGaus = hist->GetFunction("gaus")->GetParameters();
    //////////////////////

    ////////////////////
    // real fit
    TF1 *fresp = new TF1(Form("resp%d",i),"gaus(0)+pol2(3)",
			 posPeaks[i] - 5*fitParGaus[2],
			 posPeaks[i] + 5*fitParGaus[2]);
    fresp->SetParameters(fitParGaus[0],fitParGaus[1],fitParGaus[2],
			 0,0,0);
    fitRes = hist->Fit(fresp,"Q0",0,posPeaks[i] - 5*fitParGaus[2],
		       posPeaks[i] + 5*fitParGaus[2]);

    if (fitRes != 0) {
      std::cerr << "Real fit error for the peak ";
      std::cerr << i << "." <<  std::endl;
      continue; }
    ////////////////////

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
  hist->Draw("Hist");
  for (Int_t i = 0 ; i < nPeaks ; i++) {
    TF1* f1 = static_cast<TF1*>(gROOT->GetFunction(Form("resp%d",i)));
    f1->SetLineColor(i+1);
    f1->Draw("SAME");  }
  ////////////////////////////////////////

  DrawGlobalTitle();
}

void Ana::DrawGlobalTitle() {
  TPaveLabel pl;
  pl.DrawPaveLabel(0.15,0.92,0.65,0.99,
		   Form("Run%04d: %s",nRun,runTitle.Data()),"NDC");}

void Ana::FitSingleChannel(Double_t sig, Bool_t doubleFlag, Double_t lowR, Double_t uppR) {

  tree->Draw("Energy>>h0(4000,0,4000)",0,"GOFF");
  TH1* h1 = static_cast<TH1*>(gDirectory->Get("h0"));

  if (doubleFlag) FitSpectrumDouble(h1, lowR, uppR, sig, true);
  else FitSpectrum(h1, sig, true);
}

void Ana::FitEachChannel(Double_t sig) {

  for (Int_t i = 0 ; i < 4 ; i++) {
    tree->Draw(Form("Energy>>h%d(4000,0,4000)",i),
	       Form("Channel == %d && NCoin == 4",i),
	       "GOFF");
    TH1* h1 = static_cast<TH1*>(gDirectory->Get(Form("h%d",i)));

    std::cout << "Fit the spectrum for Channel " << i << "." << std::endl;
    FitSpectrum(h1, sig, false);
    std::cout << std::endl;}

  TCanvas *c1 = new TCanvas;
  c1->Divide(2,2);
  for (Int_t i = 0 ; i < 4 ; i++) {
    c1->cd(i+1);
    TH1* h1 = static_cast<TH1*>(gDirectory->Get(Form("h%d",i)));
    h1->Draw();}

  c1->cd();
  DrawGlobalTitle();
}

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

void Ana::FitSpectrumDouble(TH1* hist, Double_t lowR, Double_t uppR, Double_t sig, Bool_t drawFlag) {

  //////////////////////////////
  // Axis range
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

  //////////////////////

  ////////////////////
  // real fit
  TF1 *fresp = new TF1("respd","gaus(0)+gaus(3)+pol2(6)",lowR,uppR);
  fresp->SetParameters(fitParGaus0[0],fitParGaus0[1],fitParGaus0[2],
		       fitParGaus1[0],fitParGaus1[1],fitParGaus1[2],
		       0,0,0);
  fitRes = hist->Fit(fresp,"Q0",0, lowR, uppR);

  if (fitRes != 0) {
    std::cerr << "Real fit error." << std::endl;
    return; }
  ////////////////////

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
  hist->Draw("Hist");

  fresp->SetLineColor(1);
  fresp->Draw("SAME"); 
  ////////////////////////////////////////

  DrawGlobalTitle();
}
