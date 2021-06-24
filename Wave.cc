#include "Wave.hh"

Wave::Wave(double _period, double* _samp, int n):
  period(_period), baseline(0), iTrigTiming(-1),
  iGateBegin(-1), iGateEnd(-1), gr(NULL) {
  for ( auto i = 0 ; i < n ; i++ ) {
    t.push_back(_period*i);
    samp.push_back(_samp[i]); }}

Wave::Wave(double _period, int* _samp, int n):
  period(_period), baseline(0), iTrigTiming(-1),
  iGateBegin(-1), iGateEnd(-1), gr(NULL) {
  for ( auto i = 0 ; i < n ; i++ ) {
    t.push_back(_period*i);
    samp.push_back(static_cast<double>(_samp[i]));} }

Wave::Wave(double _period, short* _samp, int n):
  period(_period), baseline(0), iTrigTiming(-1),
  iGateBegin(-1), iGateEnd(-1), gr(NULL) {
  for ( auto i = 0 ; i < n ; i++ ) {
    t.push_back(_period*i);
    samp.push_back(static_cast<double>(_samp[i]));} }

Wave::Wave(double _period, std::vector<double> &_samp):
  period(_period), baseline(0), iTrigTiming(-1),
  iGateBegin(-1), iGateEnd(-1), gr(NULL) {
  for ( size_t i = 0 ; i < _samp.size() ; i++ ) t.push_back(_period*i);
  for ( auto it = _samp.begin() ; it != _samp.end() ; it++ )
    samp.push_back(*it); }

Wave::Wave(double _period, std::vector<int> &_samp):
  period(_period), baseline(0), iTrigTiming(-1),
  iGateBegin(-1), iGateEnd(-1), gr(NULL) {
  for ( size_t i = 0 ; i < _samp.size() ; i++ ) t.push_back(_period*i);
  for ( auto it = _samp.begin() ; it != _samp.end() ; it++ )
    samp.push_back(static_cast<double>(*it)); }

Wave::~Wave() {
  t.clear();
  samp.clear();
  if (gr) delete gr;}

void Wave::Clear() {
  t.clear(); samp.clear(); period = 0;
  iTrigTiming = iGateBegin = iGateEnd = -1;
  if (gr) delete gr;}

Wave& Wave::Scale(double c) {
  for ( auto it = samp.begin() ; it != samp.end() ; it++ ) *it *= c;
  return *this;}

Wave& Wave::AddConst(double c) {
  for ( auto it = samp.begin() ; it != samp.end() ; it++ ) *it += c;
  return *this;}

Wave& Wave::SubConst(double c) {
  for ( auto it = samp.begin() ; it != samp.end() ; it++ ) *it -= c;
  return *this;}

Wave& Wave::AddWave(const Wave& w) {
  if (GetNSample() == w.GetNSample())
    for ( size_t i = 0 ; i != samp.size() ; i++ ) samp[i] += w.GetSample(i);
  return *this;}

Wave& Wave::SubWave(const Wave& w) {
  if (GetNSample() == w.GetNSample())
    for ( size_t i = 0 ; i != samp.size() ; i++ ) samp[i] -= w.GetSample(i);
  return *this;}

double Wave::SetBaseLine(int nSamp, int iStart) {
  if (iStart < 0 || iStart >= GetNSample() || (iStart + nSamp) >= GetNSample())
    throw std::out_of_range("The index is out of range of the vector");
  double res = 0;
  for (auto i = iStart ; i < (iStart + nSamp) ; i++) res += samp[i];
  baseline = res / nSamp;
  return baseline;}

int Wave::SetTrigTiming(double thres) {
  for (size_t i = 0 ; i != samp.size() ; i++) {
    if ( samp[i] > (thres + baseline) ) {
      iTrigTiming = i; return i; }}
  iTrigTiming = -1;
  return -1;}

void Wave::SetGate(int iPreGate, int iWidth) {
  if (iTrigTiming < 0) return; 
  iGateBegin = iTrigTiming - iPreGate;
  if (iGateBegin < 0 || iGateBegin > GetNSample()) { iGateBegin = -1; return; }
  iGateEnd = iGateBegin + iWidth;
  if (iGateEnd < 0 || iGateEnd > GetNSample()) { iGateEnd = -1; return; }}

double Wave::GetPeakHeight() {
  if (baseline == 0 || iTrigTiming < 0 || iGateBegin < 0 || iGateEnd < 0) return 0;
  double height = -1;
  for (auto i = iGateBegin ; i < iGateEnd ; i++) {
    if ( samp[i] > height ) height = samp[i]; }
  return (height - baseline); }

double Wave::GetPeakArea() {
  if (baseline == 0 || iTrigTiming < 0 || iGateBegin < 0 || iGateEnd < 0) return 0;
  ROOT::Math::Interpolator intp;
  std::vector<double> x, y;
  for (auto i = 0 ; i < GetNSample() ; i++) {
    x.push_back(static_cast<double>(i));
    y.push_back(samp[i] - baseline); }
  intp.SetData(x, y);
  return intp.Integ(static_cast<double>(iGateBegin),
		    static_cast<double>(iGateEnd));}

TGraph* Wave::GetGraph() {
  if (gr) return gr;
  gr = new TGraph(GetNSample(), t.data(), samp.data());
  return gr;}
