# Makefile to build executables: double2ascii, diff2d
CXX = mpicxx
CXXFLAGS = -I. -O0 -std=c++17 -g -gdwarf-3 -pg -Wall -Wfatal-errors -Wno-sign-compare 
LDLIBS = -O0 -g -pg

all: double2ascii diff2d

double2ascii.o: double2ascii.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o double2ascii.o double2ascii.cpp

diff2d.o: diff2d.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o diff2d.o diff2d.cpp

double2ascii: double2ascii.o
	$(CXX) $(LDFLAGS) -o double2ascii double2ascii.o $(LDLIBS)

diff2d: diff2d.o 
	$(CXX) $(LDFLAGS) -o diff2d diff2d.o $(LDLIBS)

clean:
	$(RM) double2ascii.o diff2d.o

run: double2ascii diff2d
	$(RM) snapshot.bin snapshot.txt
	mpirun --mca fs_ufs_lock_algorithm 1 --oversubscribe -np 16 ./diff2d diff2d.ini
	./double2ascii snapshot.bin 20 220 240 > snapshot.txt
	gnuplot --persist snapshot.gp

large: double2ascii diff2dlarge.ini
	$(RM) snapshot.bin snapshot.txt
	time mpirun --mca fs_ufs_lock_algorithm 1 --oversubscribe -np 16 ./diff2d diff2dlarge.ini
	./double2ascii snapshot.bin 400 800 1200 > snapshot.txt
	gnuplot --persist snapshotlarge.gp

huge: double2ascii diff2dhuge.ini
	$(RM) snapshot.bin snapshot.txt
	time mpirun --mca fs_ufs_lock_algorithm 1 --oversubscribe -np 16 ./diff2d diff2dhuge.ini
	./double2ascii snapshot.bin 1600 3200 4800 > snapshot.txt
	gnuplot --persist snapshotlarge.gp

.PHONY: all clean run large



