CXX=c++
CXXFLAGS=-I. -ggdb -fPIC
LIBFILTER=libfilter.a
FILTER_SOURCES=Scanner.cpp Parser.cpp CodeGenerator.cpp # TODO: switch to automatic find
FILTER_OBJS=$(patsubst %.cpp,%.o,$(FILTER_SOURCES))

AR=ar
RANLIB=ranlib
COCOCPP=cococpp # from the coco-cpp package

$(LIBFILTER): $(FILTER_OBJS)
	$(RM) $@
	$(AR) qc $@ $^
	$(RANLIB) $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

Scanner.cpp Parser.cpp: filter.atg Scanner.frame Parser.frame
	$(RM) Scanner.cpp Parser.cpp
	cococpp -frames . $<

clean:
	$(RM) $(LIBFILTER) Scanner.cpp Scanner.h Parser.cpp Parser.h $(FILTER_OBJS)

.PHONY: clean
