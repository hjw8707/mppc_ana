#include "Ana.hh"

Ana::Ana() {}

Ana::Ana(Int_t _nRun):
  file(NULL), tree(NULL),
  fileCoin(NULL), treeCoin(NULL),
  wave(NULL) {
  LoadRun(_nRun);}

Ana::~Ana() {
  if (wave) delete wave;
  if (file) delete file;
  if (fileCoin) delete fileCoin; }

void Ana::LoadRun(Int_t _nRun) {
  nRun = _nRun;
  TString path(Form("run_%2d/UNFILTERED/Data_run_%2d.root",_nRun,_nRun));
  file = new TFile(path.Data());
  tree = static_cast<TTree*>(file->Get("Data"));
  LoadTree();}

void Ana::LoadTree() {
   Samples = 0;

   if (!tree) return;
   tree->SetBranchAddress("Channel", &Channel);
   tree->SetBranchAddress("Timestamp", &Timestamp);
   tree->SetBranchAddress("Board", &Board);
   tree->SetBranchAddress("Energy", &Energy);
   tree->SetBranchAddress("EnergyShort", &EnergyShort);
   tree->SetBranchAddress("Flags", &Flags);
   tree->SetBranchAddress("Samples", &Samples);}

void Ana::LoadCoinTree() {
   if (!treeCoin) return;
   treeCoin->SetBranchAddress("LastCoin", &flagLastCoin);
   treeCoin->SetBranchAddress("NCoin", &nCoin);}

Int_t Ana::GetEntry(Long64_t entry) {
  if (!tree) return 0;
  return tree->GetEntry(entry);}

void Ana::MakeCoin(ULong64_t tWidth) {
  if (fileCoin) delete fileCoin;
  
  TString path(Form("run_%2d/UNFILTERED/Coin_run_%2d.root",nRun,nRun));
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
  //if (GetEntry(_iEv) <= 0) return NULL;
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

