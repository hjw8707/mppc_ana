TARGET = libMPPC

SRC := Ana.cc Wave.cc
OBJ := $(SRC:.cc=.o)
INC = .
HDR := Ana.hh Wave.hh
LINKDEF = LinkDef.hh

DICTSRC = $(TARGET)_dict.cc
DICTOBJ = $(DICTSRC:.cc=.o)
DICTHDR = $(DICTSRC:.cc=_rdict.pcm)

ROOTCFLAGS  = $(shell root-config --cflags)
ROOTLDFLAGS = $(shell root-config --libs) -lSpectrum

CXX = g++
CXXFLAGS = -O2 -Wall -Werror -fPIC --std=c++11 -I. -I$(INC) -I$(HOME)/include $(ROOTCFLAGS)
LDFLAGS = -shared -L$(HOME)/lib -lstrline -lm $(ROOTLDFLAGS)
LIB = -L. -lMPPC -L$(HOME)/lib -lstrline -lm 

.PHONY: clean install

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

install: $(TARGET)
	cp $(TARGET).so $(HOME)/lib
	cp $(HDR) $(HOME)/include
	cp $(DICTHDR) $(HOME)/lib

