# Makefile to build executables: double2ascii, diff2d
CXX = mpicxx
CXXFLAGS = -Ofast -std=c++17

all: double2ascii diff2d

double2ascii.o: double2ascii.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o double2ascii.o double2ascii.cpp

diff2d.o: diff2d.cpp inifile.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o diff2d.o diff2d.cpp

inifile.o: inifile.cpp inifile.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o inifile.o inifile.cpp


double2ascii: double2ascii.o inifile.o
	$(CXX) $(LDFLAGS) -o double2ascii double2ascii.o inifile.o $(LDLIBS)
diff2d: diff2d.o inifile.o
	$(CXX) $(LDFLAGS) -o diff2d diff2d.o inifile.o $(LDLIBS)
clean:
	$(RM) double2ascii.o diff2d.o inifile.o

run: double2ascii diff2d
	$(RM) snapshot.bin snapshot.txt
	mpirun --oversubscribe -np 4 ./diff2d diff2d.ini
	./double2ascii snapshot.bin 20 > snapshot.txt
	gnuplot --persist snapshot.gp

large: double2ascii diff2d
	$(RM) snapshot.bin snapshot.txt
	mpirun --oversubscribe -np 4 ./diff2d diff2dlarge.ini
	./double2ascii snapshot.bin 400 > snapshot.txt
	gnuplot --persist snapshotlarge.gp

.PHONY: all clean run large

