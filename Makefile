SRCDIR=src
LIBDIR=lib
BINDIR=bin

SOURCES := $(shell find -L $(SRCDIR) -type f -name '*.cpp' -not -path '*/cmd/*')
OBJECTS := $(patsubst %.cpp, %.o, $(SOURCES))
SOURCES_OBLIVC := $(shell find -L $(SRCDIR) -type f -name '*.oc')
OBJECTS_OBLIVC := $(patsubst %.oc, %.oo, $(SOURCES_OBLIVC))
SOURCES_BIN := $(shell find -L $(SRCDIR)/cmd -type f -name '*.cpp')
OBJECTS_BIN := $(patsubst %.cpp, %.o, $(SOURCES_BIN))

LIBRARIES = fastpoly mpc-utils
STATIC_FILES = $(foreach lib, $(LIBRARIES), $(LIBDIR)/$(lib)/lib$(lib).a)
BINARIES = $(patsubst $(SRCDIR)/cmd/%.cpp, $(BINDIR)/%, $(SOURCES_BIN))

LDFLAGS = $(STATIC_FILES) -lntl -lboost_program_options
CXXFLAGS = -O3 -pthread -I$(SRCDIR) -I$(LIBDIR) -g -std=gnu++11\
	-DMPC_UTILS_USE_OBLIVC

all: $(LIBRARIES) $(BINARIES)

.PHONY: $(LIBRARIES)
$(LIBRARIES): $(foreach lib, $(LIBRARIES), lib/$(lib)/Makefile)
	$(MAKE) -C lib/$@

lib/%/Makefile: lib/%/CMakeLists.txt
	cd $(@D) && cmake .

%.d: %.cpp #$(LIBRARIES)
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) -MT "$*.o $@" $< > $@;

%.od: %.oc #$(LIBRARIES)
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) -MT "$*.oo $@" $< > $@;

-include $(SOURCES:.cpp=.d)
-include $(SOURCES_BIN:.cpp=.d)
-include $(SOURCES_OBLIVC:.oc=.od)

$(BINDIR)/%: $(LIBRARIES) $(OBJECTS) $(OBJECTS_OBLIVC) $(SRCDIR)/cmd/%.o
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(OBJECTS_OBLIVC) $(SRCDIR)/cmd/$*.o -o $@ $(LDFLAGS)

# do not delete intermediate objects
.SECONDARY: $(OBJECTS) $(OBJECTS_BIN) $(OBJECTS_OBLIVC)

# compile using obliv-c
%.oo : %.oc
	$(OBLIVCC) $(INCLUDES) $(OBLIVCCFLAGS) -o $@ -c $^

.PHONY: clean cleanall
clean:
	$(RM) -r $(BINDIR)
	$(RM) $(OBJECTS) $(OBJECTS_BIN) $(OBJECTS_OBLIVC)
	$(RM) $(OBJECTS:.o=.d) $(OBJECTS_BIN:.o=.d) $(OBJECTS_OBLIVC:.oo=.od)
cleanall: clean
	$(foreach lib, $(LIBRARIES), $(MAKE) -C $(LIBDIR)/$(lib) clean;)
