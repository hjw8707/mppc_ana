#ifndef WAVE_HH
#define WAVE_HH

#include "Math/Interpolator.h"
#include "TGraph.h"

#include <string>
#include <vector>
#include <exception>

class Wave {

public:
  Wave(double _period, double* _samp, int n);
  Wave(double _period, short* _samp, int n);
  Wave(double _period, int* _samp, int n);
  Wave(double _period, std::vector<double> &_samp);
  Wave(double _period, std::vector<int> &_samp);
  ~Wave();

  void Clear();

  ////////////////////////////////////////////////////////////
  // Get 
  inline double GetPeriod() const { return period; }
  inline double GetFrequency() const { return 1./period; }

  inline int GetNSample() const { return static_cast<int>(samp.size()); }
  inline const double* GetSamples() const { return samp.data(); }
  double GetSample(int i) const {
    if (i < 0 || i >= static_cast<int>(samp.size()))
      throw std::out_of_range("The index is out of range of the vector");
    return (samp[i] - baseline); }
  double GetRawSample(int i) {
    if (i < 0 || i >= static_cast<int>(samp.size()))
      throw std::out_of_range("The index is out of range of the vector");
    return samp[i]; }
  ////////////////////////////////////////////////////////////
  
  ////////////////////////////////////////////////////////////
  // Operation
  Wave& Scale(double c);
  Wave& operator*=(double c) { return Scale(c); }
  
  Wave& AddConst(double c);
  Wave& operator+=(double c) { return AddConst(c); }

  Wave& SubConst(double c);
  Wave& operator-=(double c) { return SubConst(c); }

  Wave& AddWave(const Wave& w);
  Wave& operator+=(const Wave& w) { return AddWave(w); }

  Wave& SubWave(const Wave& w);
  Wave& operator-=(const Wave& w) { return SubWave(w); }
  ////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////
  // Analysis
  double SetBaseLine(int nSamp, int iStart);
  int    SetTrigTiming(double thres);
  void   SetGate(int iPreGate, int iWidth);

  double GetPeakHeight();
  double GetPeakArea();
  ////////////////////////////////////////////////////////////

  TGraph* GetGraph();
  
private:
  double period;
  std::vector<double> t, samp;

  double baseline;
  int iTrigTiming;
  int iGateBegin, iGateEnd;

  TGraph *gr;
};

#endif
