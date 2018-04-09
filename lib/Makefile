
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
