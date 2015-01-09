#include <iostream>
#include <fstream>
#include <omp.h>
#include <cmath>

using namespace std;

// Beliebige Funktion um CPU zu "beschäftigen"
void SleepCPU()
{
	double f = 0;

	for (int j=0; j<100000; j++)
	{
		for (int i=0; i<100000; i++)
		{
			f = pow(3.,2./7.-M_2_PI);
		}
	}

	f -= f;
}
// Bei hoher Wahl der Thread-Anzahl ist das CPU komplett ausgelastet, die einzelnen Threads erhalten dabei ungefähr gleiche Anteile.

int main()
{
	#ifdef _OPENMP
		omp_set_num_threads(10);
	#endif

	ofstream file ("aufg1.txt", ios::trunc);
	int sum = 0;

	// Wird den einzelnen Threads nicht gemeisamer Zugriff auf die Summationsvariable gewährt (mittels "shared(sum)"), sonder erhält jeder seine eigene Summationsvariable (mittels "firstprivate(sum)"), dann wird für jeden Thread eine eigene Variable angelegt, und die i-Werte die der jeweilige Thread bekommt werden in ihr aufsummiert. Erhält ein Thread nur einen i-Wert so entspricht die jeweilige Summationsvariable auch nur diesem i-Wert.
	#pragma omp parallel shared(sum)
	{
		// Die Abfrage nach der Anzahl der Threads kann nur innerhalb eines "parallel"-Blockes erfolgen, außerhalb dieses gibt die Funktion sonst immer "1" zurück. Sinnvollerweise wird die Abfrage auch nur von einem Tread durchgeführt (mittels "single").
		#pragma omp single
			cout << "Anzahl Threads: " << omp_get_num_threads() << endl;

		// Für verschiedene "schedule"-Anweisungen ("guided", "dynamic", etc.) verändert sich die Reihenfolge in der die For-Schleife abgearbeitet wird und auch die Zuordnung welcher Thread welches i bekommt ändert sich.
		#pragma omp for schedule(dynamic)
		for (int i=0; i<=10; i++)
		{
			sum += i;

			SleepCPU();

			#pragma omp critical(aufg1_txt)
				file << omp_get_thread_num() << '\t' << i << '\t' << sum << endl;
		}
	}

//	cout << "sum=" << sum << endl;

	file.close();
}
