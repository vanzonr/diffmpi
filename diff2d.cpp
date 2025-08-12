#include <cmath>
#include <iostream>
#include <rarray>
#include "inifile.h"
#include <stdexcept>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <mpi.h>
#include <complex>

namespace mpi
{
    
template<typename T> struct Type_struct {};
template<typename T> inline const MPI_Datatype type = Type_struct<T>::value_;
#define MPITYPEMAP(ctype,mpitype) template<> struct Type_struct<ctype> { inline static const MPI_Datatype value_ = mpitype; }
MPITYPEMAP(char, MPI_CHAR);
MPITYPEMAP(signed char, MPI_SIGNED_CHAR);
MPITYPEMAP(unsigned char, MPI_UNSIGNED_CHAR);
MPITYPEMAP(wchar_t, MPI_WCHAR);
MPITYPEMAP(short, MPI_SHORT);
MPITYPEMAP(unsigned short, MPI_UNSIGNED_SHORT);
MPITYPEMAP(int, MPI_INT);
MPITYPEMAP(unsigned int, MPI_UNSIGNED);
MPITYPEMAP(long, MPI_LONG);
MPITYPEMAP(unsigned long, MPI_UNSIGNED_LONG);
MPITYPEMAP(long long, MPI_LONG_LONG);
MPITYPEMAP(unsigned long long, MPI_UNSIGNED_LONG_LONG);
MPITYPEMAP(float, MPI_FLOAT);
MPITYPEMAP(double, MPI_DOUBLE);
MPITYPEMAP(long double, MPI_LONG_DOUBLE);
MPITYPEMAP(bool, MPI_C_BOOL);
MPITYPEMAP(std::complex<float>, MPI_C_FLOAT_COMPLEX);
MPITYPEMAP(std::complex<double>, MPI_C_DOUBLE_COMPLEX);
MPITYPEMAP(std::complex<long double>, MPI_C_LONG_DOUBLE_COMPLEX);

using Comm = MPI_Comm;

class Context
{
  private:
    const bool must_finalize_;
    Comm comm_;
    int rank_;
    int size_;
    int thread_support_;
  public:
    Context(Comm comm): must_finalize_(false), comm_(comm) 
    {
        MPI_Comm_rank(comm_, &rank_);
        MPI_Comm_size(comm_, &size_);
    }
    Context(int& argc, char**& argv): must_finalize_(true)
    {
        int required = MPI_THREAD_MULTIPLE;
        MPI_Init_thread(&argc, &argv, required, &thread_support_);
        comm_ = MPI_COMM_WORLD;
        MPI_Comm_rank(comm_, &rank_);
        MPI_Comm_size(comm_, &size_);
    }
    Context(): must_finalize_(true)
    {
        int required = MPI_THREAD_MULTIPLE;
        MPI_Init_thread(nullptr, nullptr, required, &thread_support_);
        comm_ = MPI_COMM_WORLD;
        MPI_Comm_rank(comm_, &rank_);
        MPI_Comm_size(comm_, &size_);
    }
    ~Context()
    {
        if (must_finalize_) 
            MPI_Finalize();
    }
    Comm get_comm() const noexcept(true)
    {
        return comm_;
    }
    int getthread_support_() const
    {
        return thread_support_;
    }
    int get_size() const
    {
        return size_;
    }
    int get_rank() const
    {
        return rank_;
    }
    void error(int code, const char* msg) const
    {
        if (rank_ == 0)
            std::cerr << msg << std::endl;
        if (code != 0)
            MPI_Abort(comm_, code);
    }
    template<typename T>
    rvector<T> gather(const T& x, int root) const
    {
        rvector<T> allx(size_*(rank_==root));
        MPI_Gather(&x, 1, type<T>, allx.data(), 1, type<T>, root, comm_);
        return allx;
    }
    template<typename U, int S, typename T, int R>
    MPI_Status sendrecv(const rarray<U,S>& sendarr, int torank, int totag,
                        rarray<T,R> recvarr, int fromrank, int fromtag) const
    {
        MPI_Status status;
        MPI_Sendrecv(sendarr.data(), sendarr.size(), type<U>, torank, totag,
                     recvarr.data(), recvarr.size(), type<T>, fromrank, fromtag,
                     comm_, &status);
        return status;
    }
};

class OutputFile {
  private:
    const Context& context_;
    MPI_File file_;
  public:
    OutputFile(const Context& context) : context_(context)
    {}
    OutputFile& open(const std::string& filename, int amode, MPI_Info info)    
    {
        MPI_File_open(context_.get_comm(), filename.c_str(),
                      amode|MPI_MODE_WRONLY, info, &file_);
        return *this;
    }
    OutputFile(const Context& context, const std::string& filename,
               int amode, MPI_Info info)
      : context_(context)
    {
        this->open(filename, amode, info);
    }
    auto get_file() const
    {
        return file_;
    }
    template<typename T, int R>
    MPI_Status write_at(MPI_Offset offset, const rarray<T,R>& arr)
    {
        MPI_Status status;
        MPI_File_write_at(file_, offset, arr.data(), arr.size(), mpi::type<T>, &status);
        return status;
    }
    void close()
    {
        MPI_File_close(&file_);
    }
};
  
}

int main(int argc, char* argv[])    
{
    const mpi::Context world(argc, argv);
    
    if (argc < 2)
      world.error(1, "No inifile given on command line");

    // Read settings
    boost::property_tree::ptree settings;
    boost::property_tree::ini_parser::read_ini(argv[1], settings);
    const auto Lx = settings.get<double>("diff2d.LX");
    const auto Ly = settings.get<double>("diff2d.LY");
    const auto D  = settings.get<double>("diff2d.D");
    const auto dx = settings.get<double>("diff2d.DX");
    if (!settings.get_optional<int>("diff2d.DY"))
      settings.put("Settings.DY", dx);
    const auto dy = settings.get<double>("diff2d.DY");
    const auto runtime = settings.get<double>("diff2d.TIME");
    const auto outtime = settings.get<double>("diff2d.OUTPUT");
    const auto snapshotname = settings.get<std::string>("diff2d.OUTFILE");
    // Derive number of lattice cells, timesep, output frequency
    const auto nx  = long(Lx/dx);
    const auto ny  = long(Lx/dy);
    const auto dtx = dx*dx*D/5;
    const auto dty = dy*dy*D/5;
    const auto dt  = (dtx<dty)?dtx:dty;
    const auto nt  = long(0.5+runtime/dt);
    const auto per = long(0.5+outtime/dt);
    // checks
    if (dt > runtime) world.error(2, "runtime (TIME) is too short");
    if (per == 0) world.error(3, "output interval (OUTPUT) is too short");
    
    // Distribute domain over MPI processes by create slabs
    const int rank = world.get_rank();
    const int size = world.get_size();
    // first check if mpi decomposition strategy will work:
    const auto tol = 1.0e-8;
    if (fabs( (Lx/dx)/nx - 1.0) > tol)
        world.error(2, "DX does not fit in LX");
    if (fabs( (Ly/dy)/ny - 1.0) > tol)
        world.error(2, "DY does not fit in LY");
    if (ny < size)
        world.error(2, "LY/DY not large enough for communicator size");   
    // now divide
    const long   localny  = long(((rank+1)*ny)/size) - long((rank*ny)/size);
    const double localy1  = long((rank*ny)/size)*dx;
    const int    rankdown = (rank == 0) ? MPI_PROC_NULL : (rank - 1);
    const int    rankup   = (rank == (size-1)) ? MPI_PROC_NULL : (rank + 1);
    // write out decomposition summary  
    auto alllocalny = world.gather(localny, 0);
    if (0==rank) {
	std::cout << "===\n";
	std::cout 
	    << "Domain size:\t"   << Lx << " x " << Ly << "\n"
	    << "Grid size:\t"     << nx << " x " << ny << "\n"
	    << "MPI processes:\t" << size << "\n"
	    << "Local grids:\t"   << nx << " x " << alllocalny << "\n"
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
    #pragma omp parallel default(none) shared(rhonow,rhoprv,x,y,localny,nguards,nx,Lx,Ly)
    for (int i: xrange(localny+nguards))
        #pragma omp for 
        for (int j: xrange(nx+nguards))  
            rhonow[i][j] = rhoprv[i][j] = sin(7*(y[i]+x[j])*3.1415926535/Lx)
                                         *sin(pow(x[j]/Ly,2)*11*3.1415926535);

    // Prepare output
    mpi::OutputFile fileout(world, snapshotname, MPI_MODE_CREATE, MPI_INFO_NULL);
    MPI_Offset offset = nx*long((rank*ny)/size)*sizeof(double);

    size_t t;
    for (t = 0; t < nt; t++) {

        // sometimes write snapshot
        if (t%per==0) {
            if (rank==0)
                std::cout << t << "/" << nt << "\n";
            for (size_t i = 0; i < localny; i++) {
                static_assert(RA_VERSION_NUMBER >= 2008001);
		fileout.write_at(offset, rhoprv.at(i+1).slice(1,nx+1));
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
            if (rank == 0) rhoprv[0][j] = 0;
            if (rank == size-1) rhoprv[localny+1][j] = 0.0; // top boundary
        }
        // ghost cell exchange
        world.sendrecv(rhoprv.at(1),         rankdown, 13,
                       rhoprv.at(localny+1), rankup,   13);
        world.sendrecv(rhoprv.at(localny),   rankup,   14,
                       rhoprv.at(0),         rankdown, 14);
        // evolve
        #pragma omp parallel default(none) shared(rhonow,rhoprv,localny,nx,dt,D,dx,dy)
        #pragma omp for collapse(2)
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
            static_assert(RA_VERSION_NUMBER >= 2008001);
            fileout.write_at(offset, rhoprv.at(i+1).slice(1,nx+1));
	    offset += ny*sizeof(double);
	}
	offset += nx*(ny-localny)*sizeof(double);
    }
    
    fileout.close();
    
    return 0;
}

/*

vtune: Error: Cannot start data collection because the scope of ptrace system call is limited. To enable profiling, please set /proc/sys/kernel/yama/ptrace_scope to 0. To make this change permanent, set kernel.yama.ptrace_scope to 0 in /etc/sysctl.d/10-ptrace.conf and reboot the machine.
vtune: Warning: Microarchitecture performance insights will not be available. Make sure the sampling driver is installed and enabled on your system.



 */
