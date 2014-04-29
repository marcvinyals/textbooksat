CXX = g++
CPPFLAGS = -std=c++0x -Wall
LDFLAGS=
GRAPHVIZ_LIBS = -lgvc -lcgraph -lcdt
CIMG_LIBS = -lX11 -lpthread
LIBS =
SOURCES = solver.cc cdcl.cc dimacs.cc data_structures.cc formatting.cc analysis.cc log.cc ui.cc

# Visualization only on wille by default
ifeq ($(shell hostname -a),wille)
VIZ=VIZ
endif

ifeq ($(VIZ),VIZ)
$(info Visualization for pebbling formulas active)
LIBS += $(GRAPHVIZ_LIBS) $(CIMG_LIBS)
SOURCES += viz.cc
else
CPPFLAGS += -DNO_VIZ
endif

OBJS = $(SOURCES:.cc=.o)
ROBJS = $(addprefix release/,$(OBJS))




# argp.h under MacOSX
ifeq ($(shell uname -s),Darwin)
$(info Mac OS: using argp-standalone package)
LIBS +=/opt/local/lib/libargp.a
CPPFLAGS+=-I/opt/local/include/
endif

# gcc 4.8.1 and libstdc++ path at runtime
ifeq ($(CXX),g++)
ifneq (,$(findstring gcc/4.8.1,$(LOADEDMODULES)))
$(info Using gcc/4.8.1 module)
LDFLAGS+=-L/opt/gcc/4.8.1 -Wl,-rpath=/opt/gcc/4.8.1/lib64 
endif
endif

all: sat satr

-include $(OBJS:.o=.d)

%.o : %.cc
	$(CXX) $(CPPFLAGS) -g -c -o $@ $<
	$(CXX) $(CPPFLAGS) -MM $< > $*.d

sat: main.o $(OBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -g -o  $@ $+ $(LIBS)

clean:
	rm -f sat *.o satr release/*.o
	rm -fr release/

test: test.o $(OBJS)
	$(CXX) $(CPPFLAGS) -g -o $@ $+ -lgtest -lpthread
	./test

release/%.o: %.cc $(HEADERS)
	@-mkdir -p release/
	$(CXX) $(CPPFLAGS) -O2 -DNDEBUG -c -o $@ $<

satr: release/main.o $(ROBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -O2 -o $@ $+ $(LIBS)

.PHONY : all clean test
