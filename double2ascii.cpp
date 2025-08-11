// @file double2ascii.cc
//
// @brief Reads a file (as if) containing binary double precision
//        numbers forrming an r x c matrix, and prints it out in ascii.
//
// @author Ramses van Zon
// @date June 14, 2022

#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    using namespace std;
    
    // Check if enough command line arguments were given.
    if (argc<4) {
        
        cerr << "Need two or three parameters, a file and a number of row,"
                " and optionally, the number of columns." << endl;
        return 1;

    } else {
        
        // Open file given as first argument, and check.
        ifstream filein(argv[1], ios::binary);
        if (not filein.good()) { 
            cerr << "Could not open file '" << argv[1] << "'." << endl;
            return 2;
        }

        // Read number of rows, and check if not zero.
        size_t r = atol(argv[2]);
        if (r == 0) { 
            cerr << "Incorrect number of columns '" << argv[2] << "'." << endl;
            return 3;
        }

        // Read number of first column, if given a third command line argument.
        size_t c1 = (argc>3)?atol(argv[3]):0;

        // Read number of last columns, if given a third command line argument.
        size_t c = (argc>3)?atol(argv[4]):0;

        // Keep reading rows and printing them to console, until reaching
        // c or end of file.
        double row[r];
        int i = 0;
        while (0==c or i<c)
        {
            filein.read((char*)row, sizeof(double)*r);
            if (not filein) break;
            if (i>=c1)
                for (int j = 0; j < r; j++)
                    cout << row[j] << ' ';
            cout << endl;
            i++;
        }

    }

}
