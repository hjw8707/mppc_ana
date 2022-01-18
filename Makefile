TARGET = libMPPC

SRC := Ana.cc Wave.cc AnaMult.cc
OBJ := $(SRC:.cc=.o)
INC = .
HDR := Ana.hh Wave.hh AnaMult.hh
LINKDEF = LinkDef.hh

DICTSRC = $(TARGET)_dict.cc
DICTOBJ = $(DICTSRC:.cc=.o)
DICTHDR = $(DICTSRC:.cc=_rdict.pcm)

ROOTCFLAGS  = $(shell root-config --cflags)
ROOTLDFLAGS = $(shell root-config --libs) -lSpectrum

CXX = g++
CXXFLAGS = -O2 -Wall -Werror -fPIC --std=c++11 -I$(INC) $(ROOTCFLAGS)
LDFLAGS = -shared -lm $(ROOTLDFLAGS)
LIB = -L. -lMPPC -lm 

.PHONY: clean

$(TARGET): $(OBJ) $(DICTOBJ)
	$(CXX) -o $@.so $^ $(LDFLAGS)

$(DICTSRC): $(HDR) $(LINKDEF)
#	rootcint -f $@ -I. $(INCLUDE) -I$(ROOTSYS)/include $^
	rootcling -f $@ -I. $(INCLUDE) -I$(ROOTSYS)/include $^	

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET).so

dictclean:
	rm -f $(DICTSRC) $(DICTHDR)

