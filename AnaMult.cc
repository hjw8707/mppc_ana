#include "AnaMult.hh"

#include "TSystemDirectory.h"
#include "TSpectrum.h"
#include "TF1.h"
#include "TPaveLabel.h"
#include "TCanvas.h"

AnaMult::AnaMult() {}

AnaMult::AnaMult(Int_t _nRun, const char* _runTitle):
  file(NULL), tree(NULL) {
  //  gain[0] = gain[1] = gain[2] = gain[3] = 1;
  gain[0] = 1;
  gain[1] = 1.104034;
  gain[2] = 1.16801;
  gain[3] = 1.084516;
  LoadRun(_nRun, _runTitle);}

AnaMult::~AnaMult() {
  if (file) delete file; }

void AnaMult::LoadRun(Int_t _nRun, const char* _runTitle) {
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
    if (fileName.Contains(Form("Mult_%s",runName.Data()))) {
      file = new TFile(Form("%s/%s",tsd->GetTitle(),fileName.Data()));
      if (!file->IsOpen()) {
	std::cerr << "No such a root file can be found." << std::endl;
	return; }
      tree = static_cast<TTree*>(file->Get("mult"));
      std::cout << "Tree loaded." << std::endl;
      LoadTree();
      break;}}
  delete tsd;}

void AnaMult::LoadTree() {

   if (!tree) return;
   tree->SetBranchAddress("nCh", &nCh);
   tree->SetBranchAddress("time", time);
   tree->SetBranchAddress("energy", energy);
   std::cout << "Tree branches set." << std::endl;
   LoadAlias();}

void AnaMult::LoadAlias() {
  tree->SetAlias("emult","(energy[0]*energy[1]*energy[2]*energy[3])**0.25");
  tree->SetAlias("esum",Form("(energy[0]*%.3f+energy[1]*%.3f+energy[2]*%.3f+energy[3]*%.3f)/4.",gain[0],gain[1],gain[2],gain[3]));
  std::cout << "Tree aliases set." << std::endl; }
  
void AnaMult::DrawGlobalTitle(const char *sub) {
    TPaveLabel pl;
    if (sub)
      pl.DrawPaveLabel(0.15,0.92,0.65,0.99,
		       Form("Run%04d: %s, %s",nRun,runTitle.Data(), sub),"NDC");
    else
      pl.DrawPaveLabel(0.15,0.92,0.65,0.99,
		       Form("Run%04d: %s",nRun,runTitle.Data()),"NDC");
  }



void AnaMult::FitSpectrum(TH1* hist, Double_t sig, Bool_t drawFlag) {

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
			 posPeaks[i] - 2*fitParGaus[2],
			 posPeaks[i] + 2*fitParGaus[2]);
    fresp->SetParameters(fitParGaus[0],fitParGaus[1],fitParGaus[2],
			 0,0,0);
    fitRes = hist->Fit(fresp,"Q0",0,posPeaks[i] - 2*fitParGaus[2],
		       posPeaks[i] + 2*fitParGaus[2]);

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

}

void AnaMult::FitAllChannel(Double_t sig) {
  TCanvas *c1 = new TCanvas("c1","c1");
  c1->Divide(2,2);

  for (Int_t i = 0 ; i < 4 ; i++) {
    c1->cd(i+1);
    tree->Draw(Form("energy[%d]>>h%d(2000,0,2000)",i,i),
	       "nCh == 4");
    TH1 *h1 = static_cast<TH1*>(gDirectory->Get(Form("h%d",i)));
    FitSpectrum(h1,sig); }

}

void AnaMult::FitSums(Double_t sig) {
  TCanvas *c1 = new TCanvas("c1","c1",1200,500);
  c1->Divide(2,1);

  c1->cd(1);
  tree->Draw("emult>>hmult(2000,0,2000)","nCh == 4");
  TH1 *h1 = static_cast<TH1*>(gDirectory->Get("hmult"));
  FitSpectrum(h1,sig);
  DrawGlobalTitle("Mult.");
  
  c1->cd(2);
  tree->Draw("esum>>hsum(2000,0,2000)","nCh == 4");
  h1 = static_cast<TH1*>(gDirectory->Get("hsum"));
  FitSpectrum(h1,sig);
  DrawGlobalTitle("Aver.");
}
