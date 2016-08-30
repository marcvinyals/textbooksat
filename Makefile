CXX = g++
CPPFLAGS = -std=c++0x -Wall
LDFLAGS=
GRAPHVIZ_LIBS = -lgvc -lcgraph -lcdt
CIMG_LIBS = -lX11 -lpthread
LIBS =
SOURCES = solver.cc cdcl.cc dimacs.cc data_structures.cc formatting.cc analysis.cc log.cc ui.cc pebble_util.cc

BUILD ?= debug
ifeq ($(BUILD),debug)
CPPFLAGS += -g -Og -DDEBUG
else ifeq ($(BUILD),release)
CPPFLAGS += -O2 -DNDEBUG
else ifeq ($(BUILD),asan)
CXX=clang++
CPPFLAGS += -g -DDEBUG -U_FORTIFY_SOURCE -fsanitize=address -fsanitize=undefined
endif

# Visualization only on Marc machines by default
ifeq ($(shell hostname -s),wille)
VIZ=VIZ
LDFLAGS+=-L/usr/local-noprio/lib -Wl,-rpath=/usr/local-noprio/lib
CPPFLAGS+=-I/usr/local-noprio/include
endif
ifeq ($(shell hostname -s),pcbox-marc)
VIZ=VIZ
endif
ifeq ($(shell hostname -s),tcs57)
VIZ=VIZ
endif


ifeq ($(VIZ),VIZ)
$(info Visualization for pebbling formulas active)
LIBS += $(GRAPHVIZ_LIBS) $(CIMG_LIBS)
SOURCES += viz.cc vizpebble.cc viztseitin.cc
else
CPPFLAGS += -DNO_VIZ
endif

OBJS = $(SOURCES:.cc=.o)
ROBJS = $(addprefix $(BUILD)/,$(OBJS))




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

all: debug

-include $(OBJS:.o=.d)

$(BUILD)/%.o : %.cc $(HEADERS)
	@-mkdir -p $(BUILD)
	$(CXX) $(CPPFLAGS) -g -c -o $@ $<
	$(CXX) $(CPPFLAGS) -MM $< > $*.d

$(BUILD)/sat: $(BUILD)/main.o $(ROBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -o  $@ $+ $(LIBS)

clean:
	rm -f sat *.o satr release/*.o *.d
	rm -fr debug/ release/

test: $(BUILD)/test.o $(ROBJS)
	$(CXX) $(CPPFLAGS) -o $@ $+ -lgtest -lpthread
	./test

sat: $(BUILD)/sat

debug:
	$(MAKE) BUILD=debug sat
	cp debug/sat sat

release:
	$(MAKE) BUILD=release sat

asan:
	$(MAKE) BUILD=asan sat

.PHONY : all clean test sat debug release asan
