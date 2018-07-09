
lib%.a: %/Makefile
	make -C $*
	cp $*/$@ $@

%/Makefile: %/CMakeLists.txt
	cd $(@D) && cmake .

%/Makefile: %/configure
	cd $(@D) && ./configure

libobliv-c.a: obliv-c/Makefile
	if [ ! -f obliv-c/_build/libobliv.a ]; then \
		make -C obliv-c; \
	fi;
	cp obliv-c/_build/libobliv.a $@

liback.a: absentminded-crypto-kit/Makefile
	CFLAGS="$(CFLAGS) -D_Float128=double" make -C absentminded-crypto-kit
	cp absentminded-crypto-kit/build/lib/liback.a $@

SUBDIRS=absentminded-crypto-kit fastpoly mpc-utils obliv-c
CMAKE_FILES = CMakeFiles Makefile CMakeCache.txt cmake_install.cmake
.PHONY: clean
clean:
	$(foreach dir, $(SUBDIRS), make -C $(dir) clean || true;)
	$(foreach file, $(CMAKE_FILES), $(RM) -r fastpoly/$(file);)
