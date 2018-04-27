# Project: any_iterator_test
# Makefile created by Thomas Becker Jan 10, 2007

CPP  = "C:\msys\1.0\mingw\bin\g++.exe"
CC   = "C:\msys\1.0\mingw\bin\gcc.exe"

WINDRES = windres.exe

OBJ  = main.o any_iterator_demo.o any_iterator_wrapper_test.o any_iterator_test.o boost_iterator_library_example_test.o
LIBS =  -L"C:/msys/1.0/mingw/lib" 

INCS = -I"C:/Program Files/include" \
  -I"C:/msys/1.0/mingw/include" \
 -I"C:/msys/1.0/mingw/include/c++" \
 -I"C:/msys/1.0/mingw/include/GL" \
 -I"C:/msys/1.0/mingw/include/sys" \
 -I"C:/msys/1.0/mingw/include/c++/backward" \
 -I"C:/msys/1.0/mingw/include/c++/bits" \
 -I"C:/msys/1.0/mingw/include/c++/ext" \
 -I"C:/msys/1.0/mingw/include/c++/mingw32" \
 -I"C:/msys/1.0/mingw/include/c++/mingw32/bits"

BIN = any_iterator_test.exe

# CXXFLAGS = $(INCS)  -g3
# CFLAGS = $(INCS)  -g3

CXXFLAGS = $(INCS)
CFLAGS = $(INCS)

all: any_iterator_test.exe

clean:
	rm -f $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(OBJ) -o "any_iterator_test.exe" $(LIBS) $(CXXFLAGS)

main.o: ../main.cpp
	$(CPP) -c ../main.cpp -o main.o $(CXXFLAGS)

any_iterator_demo.o: ../../documentation/any_iterator_demo.cpp
	$(CPP) -c ../../documentation/any_iterator_demo.cpp -o any_iterator_demo.o $(CXXFLAGS)

any_iterator_wrapper_test.o: ../any_iterator_wrapper_test.cpp
	$(CPP) -c ../any_iterator_wrapper_test.cpp -o any_iterator_wrapper_test.o $(CXXFLAGS)

any_iterator_test.o: ../any_iterator_test.cpp
	$(CPP) -c ../any_iterator_test.cpp -o any_iterator_test.o $(CXXFLAGS)

boost_iterator_library_example_test.o: ../boost_iterator_library_example_test.cpp
	$(CPP) -c ../boost_iterator_library_example_test.cpp -o boost_iterator_library_example_test.o $(CXXFLAGS)
