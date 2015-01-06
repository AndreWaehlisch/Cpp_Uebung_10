#include "helfer.h"
#include "adapt.h"
#include <limits>
#include <iostream>
#include <fstream>

#include <mpi.h>
#include "masterworker.h"

using namespace std;

// **********
class Diffusion
{
	double lambda, border;
	double s, t;

// 	Inline auxiliary function for the spectrum g
	inline double g(const double x)
	{
		return pow( 1.+x*x, -s/2. );
	}

// 	Auxiliary function for the integrand
	double integrand(const double x)
	{
		// Define constants
		double f1, f2, f3;
		const double f4 = exp( -t/(2.*lambda) );

		// Select range
		if (x < border)
		{
			f1 = sqrt( 1./(4.*lambda*lambda) - x*x/3. );
			f2 = 1./f1;
			f3 = sinh(t*f1);
			return g(x) * f2 * f3 * f4;
		}
		else if (x > border)
		{
			f1 = sqrt( -1./(4.*lambda*lambda) + x*x/3. );
			f2 = 1./f1;
			f3 = sin(t*f1);
			return g(x) * f2 * f3 * f4;
		}
		else
			return g(x) * t * f4;
	}

	public:
// 	Constructor: initialize parameters
	Diffusion()
	{
		lambda = 3.;
		border = sqrt(3.)/(2.*lambda);
		s = 5./3.;
		t = 0.;
	}

// 	Set the time for each integration
	void set(const double tt)
	{
		t = tt;
	}

// 	Return combined integrand for x<1 and for x>1
	double operator() (const double x)
	{
		const double y = 1./x;
		return integrand(x) + y*y * integrand(y);
	}
};


// **********
int main(int argc, char *argv[])
{
// 	Some necessary MPI variables
	int rank, size, worker_rank, idx=0;

	// initialization
	MPI_Init(&argc, &argv);
	MPI_Comm_size( MPI_COMM_WORLD, &size ); // ID of individuell process
	MPI_Comm_rank( MPI_COMM_WORLD, &rank ); // total number of processes

// 	additional variables
	const int N = 200.; // Integrationsschritte
	double ans[2];	// paramter der Integration: Zeit t und das Ergebnis


// 	Main branch: master process
	if (!rank)
	{
		int command, cnt=0;

		ofstream file ("aufg4.txt", ios::trunc);

		// Initialize master communication object
		Master m(size-1, MPI_DOUBLE, 2);

		// Iterate over all indices, which correspond to individual tasks
		while ( idx<N || m.some_working() )
		{
			// If a worker reports finished work, take results
			if (worker_rank = m.listen(&command))
				switch (command)
				{
					case MW_ask_for_job:
						if (idx<N)
						{
							// Assign new task to worker
							m.send_work(worker_rank, &idx);
							++idx;
						}
						else
							m.suspend_worker(worker_rank);
						break;

					case MW_return_result:
						m.get_result(worker_rank, ans);

						// Report reception of result and write to file
						cout << ++cnt << " von " << N << endl;
						file << ans[0] << '\t' << ans[1] << endl;
						break;

					case MW_job_done:
						m.free_worker(worker_rank);
						break;
				}
			usleep(100);
		}

		file.close();
	}

// 	Main branch: worker processes
	else
	{
		const double tmax = 100.; // maximale Zeit
		const double dt = tmax/(N-1); // Zeitschritt
		const double eps = numeric_limits<double>::epsilon(); // kleinste darstellbare Zahl

		Diffusion f; // initialize integrand

		Adapt adapt(1e-10); // initialize adaptive integratino obj.

		// Initialize worker communication object
		Worker w(MPI_DOUBLE, 2);

		// Repeatedly ask for work, do integration, and submit result to master
		while ( w.get_work(&idx) )
		{
			// set time in integrand
			ans[0] = idx*dt;
			f.set(ans[0]);

			ans[1] = adapt.integrate(f,eps,1.); // integration

			w.send_result(ans); // send result to master
			w.done(); // ready for more work
		}
	}

	// finalizing MPI
	MPI_Finalize();
}
