CXX ?= g++
CPPFLAGS ?= -std=c++14 -Wall
LDFLAGS ?=
GRAPHVIZ_LIBS = -lgvc -lcgraph -lcdt
CIMG_LIBS = -lX11 -lpthread
LIBS =
SOURCES = solver.cc cdcl.cc clause_database.cc reference_clause_database.cc watched_clause_database.cc dimacs.cc data_structures.cc formatting.cc analysis.cc log.cc ui.cc pebble_util.cc

BUILD := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),debug)
ifeq ($(BUILD),debug)
CPPFLAGS += -g -Og -DDEBUG
else ifeq ($(BUILD),release)
CPPFLAGS += -O2 -DNDEBUG -march=native
else ifeq ($(BUILD),hpc2n)
CPPFLAGS = -O2 -DNDEBUG -march=bdver1
LDFLAGS += -static
else ifeq ($(BUILD),asan)
CXX = clang++
CPPFLAGS += -g -DDEBUG -U_FORTIFY_SOURCE -fsanitize=address -fsanitize=undefined
else ifeq ($(BUILD),test)
CPPFLAGS += -g -Og -DDEBUG
else ifeq ($(BUILD),coverage)
CPPFLAGS += -g -Og -DDEBUG --coverage
LDFLAGS += --coverage
endif

# Visualization only on Marc machines by default
ifeq ($(shell hostname -s),wille)
VIZ?=VIZ
LDFLAGS+=-L/usr/local-noprio/lib -Wl,-rpath=/usr/local-noprio/lib
CPPFLAGS+=-I/usr/local-noprio/include
endif
ifeq ($(shell hostname -s),pcbox-marc)
VIZ=VIZ
endif
ifeq ($(shell hostname -s),tcs57)
VIZ=VIZ
endif

TSOURCES := $(SOURCES)

ifeq ($(VIZ),VIZ)
$(info Visualization for pebbling formulas active)
LIBS += $(GRAPHVIZ_LIBS) $(CIMG_LIBS)
SOURCES += viz.cc vizpebble.cc viztseitin.cc
else
CPPFLAGS += -DNO_VIZ
endif

OBJS = $(addprefix $(BUILD)/,$(SOURCES:.cc=.o))
TOBJS = $(addprefix $(BUILD)/,$(TSOURCES:.cc=.o))



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

-include $(addprefix $(BUILD)/,$(SOURCES:.cc=.d))

$(BUILD)/%.o: src/%.cc $(HEADERS)
	@-mkdir -p $(BUILD)
	$(CXX) $(CPPFLAGS) -g -c -o $@ $<
	$(CXX) $(CPPFLAGS) -MM $< -MT $@ > $(BUILD)/$*.d

$(BUILD)/sat: $(BUILD)/main.o $(OBJS)
	$(CXX) $(LDFLAGS) -o  $@ $+ $(LIBS)

clean:
	rm -f sat *.o satr release/*.o *.d
	rm -fr debug/ release/

$(BUILD)/gtest: $(BUILD)/test/test.o $(TOBJS)
	$(CXX) $(LDFLAGS) -o $@ $+ -lgtest -lpthread
	$(BUILD)/gtest

sat: $(BUILD)/sat
gtest: $(BUILD)/gtest
debug: sat
release: sat
hpc2n: sat
asan: sat
test: gtest
coverage: gtest

.PHONY : all clean test sat debug release asan
