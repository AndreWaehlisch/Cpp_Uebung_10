// **************************
// *** Numerikum, WS 2014 ***
// *** Sheet 09 Problem 2 ***
// **************************

#include "helfer.h"
#include "adapt.h"
#include <limits>
#include <iostream>
#include <fstream>
#include <omp.h>

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
int main()
{
	#ifdef _OPENMP
		omp_set_num_threads( omp_get_num_procs() );
	#endif

//	Set number of time steps and maximum time
	const int N = 200;
	const double tmax = 100.;

// 	Determine time increment and smallest representable number
	const double dt = tmax/(N-1);
	const double eps = numeric_limits<double>::epsilon();

	ofstream datei("aufg2.txt",ios::trunc);

// 	Initialize integrand
	Diffusion f;

// 	Initialize adaptive integration object
	Adapt adapt(1e-10);

	#pragma omp parallel firstprivate(f, adapt)
	{
		#pragma omp for schedule(dynamic)
		for (int i=0; i<N; ++i)
		{
			// Set time in integrand
			double t = i*dt;
			f.set(t);

			// Performe integration for time t
			double ans = adapt.integrate(f,eps,1.);

			// Output result as a function of time
			#pragma omp critical(aufg2_txt)
				datei << t << '\t' << ans << endl;

			// Warn if integration fails
			if (adapt.offTolerance())
				cout << "Required tolerance has not been met" << endl;
		}
	}

	datei.close();
}
