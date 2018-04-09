SRCDIR=$(PWD)/src
LIBDIR=$(PWD)/lib
BINDIR=$(PWD)/bin

SOURCES := $(shell find -L $(SRCDIR) -type f -name '*.cpp' -not -path '*/cmd/*')
OBJECTS := $(patsubst %.cpp, %.o, $(SOURCES))
SOURCES_OBLIVC := $(shell find -L $(SRCDIR) -type f -name '*.oc')
OBJECTS_OBLIVC := $(patsubst %.oc, %.oo, $(SOURCES_OBLIVC))
SOURCES_BIN := $(shell find -L $(SRCDIR)/cmd -type f -name '*.cpp')
OBJECTS_BIN := $(patsubst %.cpp, %.o, $(SOURCES_BIN))

LIBRARIES = fastpoly mpc-utils obliv-c ack
STATIC_FILES = $(foreach lib, $(LIBRARIES), $(LIBDIR)/lib$(lib).a)
BINARIES = $(patsubst $(SRCDIR)/cmd/%.cpp, $(BINDIR)/%, $(SOURCES_BIN))

export OBLIVC_PATH = $(LIBDIR)/obliv-c
OBLIVCC = $(OBLIVC_PATH)/bin/oblivcc

LDFLAGS = $(STATIC_FILES) -lntl -lboost_program_options -lboost_serialization \
	-lboost_system -lboost_thread -lboost_iostreams -lgcrypt
INCLUDES = -I$(SRCDIR) -I$(LIBDIR) -I$(LIBDIR)/obliv-c/src/ext/oblivc \
	-I$(LIBDIR)/absentminded-crypto-kit/src
export CXXFLAGS = -O3 -pthread -g -std=gnu++11 $(INCLUDES) \
	-DMPC_UTILS_USE_NTL -DMPC_UTILS_USE_OBLIVC \


all: $(BINARIES)

.PHONY: $(STATIC_FILES)
$(STATIC_FILES):
	$(MAKE) -C $(LIBDIR) $(@F)

%.d: %.cpp #| $(STATIC_FILES)
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) -MT "$*.o $@" $< > $@;

%.od: %.oc #| $(STATIC_FILES)
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) -MT "$*.oo $@" $< > $@;

-include $(SOURCES:.cpp=.d)
-include $(SOURCES_BIN:.cpp=.d)
-include $(SOURCES_OBLIVC:.oc=.od)

$(BINDIR)/%: $(OBJECTS) $(OBJECTS_OBLIVC) $(SRCDIR)/cmd/%.o | $(STATIC_FILES)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# do not delete intermediate objects
.SECONDARY: $(OBJECTS) $(OBJECTS_BIN) $(OBJECTS_OBLIVC)

# compile using obliv-c
%.oo : %.oc | $(LIBDIR)/libobliv-c.a
	$(OBLIVCC) $(INCLUDES) $(OBLIVCCFLAGS) -o $@ -c $<

.PHONY: clean cleanall
clean:
	$(RM) -r $(BINDIR)
	$(RM) $(OBJECTS) $(OBJECTS_BIN) $(OBJECTS_OBLIVC)
	$(RM) $(OBJECTS:.o=.d) $(OBJECTS_BIN:.o=.d) $(OBJECTS_OBLIVC:.oo=.od)
	$(RM) $(STATIC_FILES)
cleanall: clean
	$(foreach lib, $(LIBRARIES), $(MAKE) -C $(LIBDIR)/$(lib) clean;)
