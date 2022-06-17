#include <mpi.h>
#include <cmath>
#include <iostream>
#include <rarray>
#include "inifile.h"

int mpirank(MPI_Comm c=MPI_COMM_WORLD) {
    int r; MPI_Comm_rank(c, &r); return r;
}

int mpisize(MPI_Comm c=MPI_COMM_WORLD) {
    int r; MPI_Comm_size(c, &r); return r;
}

void mpierror(int code, const char* msg)
{
    if (mpirank() == 0)
        std::cerr << msg << std::endl;
    if (code != 0)
        MPI_Abort(MPI_COMM_WORLD, code);
}

int main(int argc, char* argv[])    
{
    
    MPI_Init(&argc, &argv);
    
    if (argc < 2) mpierror(1, "No inifile given on command line");

    const Inifile settings(argv[1]);
    const auto Lx = settings.get_double("LX");
    const auto Ly = settings.get_double("LY");
    const auto D  = settings.get_double("D");
    const auto dx = settings.get_double("DX");                            
    const auto dy = settings.verify("DY")?settings.get_double("DY"):dx;
    const auto runtime = settings.get_double("TIME");
    const auto outtime = settings.get_double("OUTPUT");
    const auto omega = settings.get_double("OMEGA");
    const auto K = settings.get_double("K");
    const char* snapshotname = settings.get_string("OUTFILE");

    // Derive number of lattice cells, timesep, output frequency
    const auto nx  = long(Lx/dx);
    const auto ny  = long(Lx/dy);
    const auto dtx = dx*dx*D/5;
    const auto dty = dy*dy*D/5;
    const auto dt  = (dtx<dty)?dtx:dty;
    const auto nt  = long(0.5+runtime/dt);
    const auto per = long(0.5+outtime/dt);
    // checks
    if (dt > runtime) mpierror(2, "runtime (TIME) is too short");
    if (per == 0) mpierror(3, "output interval (OUTPUT) is too short");
    
    // Distribute domain over MPI processes by create slabs
    const int rank = mpirank();
    const int size = mpisize();
    // first check if mpi decomposition strategy will work:
    const auto tol = 1.0e-8;
    if (fabs( (Lx/dx)/nx - 1.0) > tol) mpierror(2, "DX does not fit in LX");
    if (fabs( (Ly/dy)/ny - 1.0) > tol) mpierror(2, "DY does not fit in LY");
    if (ny < size) mpierror(2, "LY/DY not large enough for communicator size");   
    // now divide
    const long   localny  = long(((rank+1)*ny)/size) - long((rank*ny)/size);
    const double localy1  = long((rank*ny)/size)*dx;
    const int    rankdown = (rank == 0) ? MPI_PROC_NULL : (rank - 1);
    const int    rankup   = (rank == (size-1)) ? MPI_PROC_NULL : (rank + 1);
    // write out decomposition summary
    long alllocalny[size];
    MPI_Gather(&localny, 1, MPI_DOUBLE, alllocalny, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    if (0==rank) {
	std::cout << "===\n";
	std::cout 
	    << "Domain size:\t" << Lx << " x " << Ly << "\n"
	    << "Grid size:\t" << nx << " x " << ny << "\n"
	    << "MPI processes:\t" << size << "\n"
	    << "Local grids:\t" << nx << " x {" << alllocalny[0];
	for (int j=1;j<size;j++) {
	    std::cout << "," << alllocalny[j];

	}
	std::cout 
	    << "}\n"
	    << "Time steps:\t" << nt << "\n"
	    << "Output every\t"<< per << " steps ("
	    << (nt/per + (nt%per==0)) << " snapshots)\n";
	std::cout << "===\n";
    }

    // Create fields
    const long nguards = 2;
    const rvector<double> x = linspace(-0.5*dx, (nx + 0.5)*dx, nx + nguards);
    const rvector<double> y = linspace(localy1 - 0.5*dy,
                                       localy1 + (localny + 0.5)*dy,
                                       localny + nguards);
    rmatrix<double> rhonow(localny + nguards, nx + nguards);
    rmatrix<double> rhoprv(localny + nguards, nx + nguards);

    // Initialize
    for (int i: xrange(localny+nguards)) 
        for (int j: xrange(nx+nguards))  
            rhonow[i][j] = rhoprv[i][j] = sin(7*(y[i]+x[j])*3.1415926535/Lx)
                                         *sin(pow(x[j]/Ly,2)*11*3.1415926535);

    // Prepare output
    MPI_File f;
    MPI_File_open(MPI_COMM_WORLD, snapshotname, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
    MPI_Offset offset = nx*localny*rank*sizeof(double);

    size_t t;
    for (t = 0; t < nt; t++) {

        // sometimes write snapshot
        if (t%per==0) {
            if (rank==0)
                std::cout << t << "/" << nt << "\n";
            for (size_t i = 0; i < localny; i++) {
                MPI_File_write_at(f, offset, rhoprv[i+1]+1, nx, MPI_DOUBLE, MPI_STATUS_IGNORE);
                offset += ny*sizeof(double);
            }
            offset += nx*(ny-localny)*sizeof(double);
        }
        // boundaries conditions
        for (int i = 0; i <= localny+1; i++) {
            rhoprv[i][0] = 0.0;      // j=0 boundary 
            rhoprv[i][nx+1] = 0.0;   // j=nx+1 boundary
        }
        for (int j = 0; j <= nx+1; j++) {
            if (rank == 0) rhoprv[0][j] = sin(omega*t*dt - x[j]/Ly*K*3.1415926535);  // i=0 boundary
            if (rank == size-1) rhoprv[localny+1][j] = 0.0; // top boundary
        }
        // ghostcell exchange
        MPI_Sendrecv(rhoprv[1], nx+nguards, MPI_DOUBLE,
                     rankdown, 13, rhoprv[localny+1], nx+nguards,
                     MPI_DOUBLE, rankup, 13,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(rhoprv[localny], nx+nguards, MPI_DOUBLE,
                     rankup, 14, rhoprv[0], nx+nguards,
                     MPI_DOUBLE, rankdown, 14,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // evolve
        for (int i = 1; i <= localny; i++) {
            for (int j = 1; j <= nx; j++) {
               rhonow[i][j] = rhoprv[i][j]
                   + dt*D/(dy*dy) * (+rhoprv[i+1][j]
                                     +rhoprv[i-1][j]
                                     -2*rhoprv[i][j])
                   + dt*D/(dx*dx) * (+rhoprv[i][j+1]
                                     +rhoprv[i][j-1]
                                     -2*rhoprv[i][j]);
            }
        }

        std::swap(rhonow, rhoprv);
    }


    // sometimes last snapshot
    if (t%per==0) {
	if (rank==0)
	    std::cout << t << "/" << nt << "\n";
	for (size_t i = 0; i < localny; i++) {
	    MPI_File_write_at(f, offset, rhoprv[i+1]+1, nx, MPI_DOUBLE, MPI_STATUS_IGNORE);
	    offset += ny*sizeof(double);
	}
	offset += nx*(ny-localny)*sizeof(double);
    }
    
    MPI_File_close(&f);
    MPI_Finalize();
    return 0;
}

