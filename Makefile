CC=cc
CXX=c++
CFLAGS=-g -I./include
CXXFLAGS=-g -I./include
LDFLAGS=-g -lstfl -lmrss -lnxml
OUTPUT=noos
SRC=$(wildcard *.cpp) $(wildcard src/*.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SRC))

STFLHDRS=$(patsubst %.stfl,%.h,$(wildcard stfl/*.stfl))

STFLCONV=./stfl2h.pl
RM=rm -f

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $(OUTPUT) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.h: %.stfl
	$(STFLCONV) $< > $@

clean:
	$(RM) $(OUTPUT) $(OBJS) $(STFLHDRS) core *.core Makefile.deps

Makefile.deps: $(SRC)
	$(CXX) -MM -MG $(SRC) > Makefile.deps

include Makefile.deps
