
SOURCES = $(shell find src -type f -name '*.cpp')
OBJECTS = $(patsubst %.cpp, %.o, $(SOURCES))

LIBRARIES = fastpoly mpc-utils
STATIC_FILES = $(foreach lib, $(LIBRARIES), lib/$(lib)/lib$(lib).a)

LDFLAGS = $(LIBRARIES)

all: $(LIBRARIES)

.PHONY: $(LIBRARIES)
$(LIBRARIES): $(foreach lib, $(LIBRARIES), lib/$(lib)/Makefile)
	make -C lib/$@

lib/%/Makefile: lib/%/CMakeLists.txt
	cd $(@D) && cmake .

%.d: %.cpp $(LIBRARIES)
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) -MT "$*.o $@" $< > $@;

-include $(SOURCES:.cpp=.d)

.PHONY: clean cleanall
clean:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d)
cleanall: clean
	$(foreach lib, $(LIBRARIES), $(MAKE) -C lib/$(lib) clean;)
