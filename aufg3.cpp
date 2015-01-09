#include <iostream>
#include <mpi.h>

using namespace std;

int main(int argc, char *argv[])
{
	// initialization
	MPI_Init(&argc, &argv);

	int size; // total number of processes
	int rank; // ID of individuell process

	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );

	MPI_Status status; // status of receiving procedure
	const int id = 1; // label for communication

	int sum = 0; // "message" between threads

	// actual computation of threads

	if (!rank)
	{
		// MASTER
		cout << "Number of threads: " << size << endl;

		MPI_Send(&sum, 1, MPI_INT, rank+1, id, MPI_COMM_WORLD); // send "message" to next thread
		MPI_Recv(&sum, 1, MPI_INT, size-1, id, MPI_COMM_WORLD, &status); // receive end result from last thread

		cout << "End result of whole program: " << sum << endl;

		int forLoop = 0;
		for (int i=0; i<size; i++)
		{
			forLoop += i;
		}

		cout << "Result of normal 'for' loop (expected result): " << forLoop << endl;

		// This "normal" for-loop gives back the same result as the multi-threaded program: The implementation works. For very large thread numbers the run failes with an error message ("The system limit on number of pipes a process can open was reached").
	} else if (rank == size-1)
	{
		// LAST thread
		MPI_Recv(&sum, 1, MPI_INT, rank-1, id, MPI_COMM_WORLD, &status); // receive message from preceding thread
		sum += rank;
		MPI_Send(&sum, 1, MPI_INT, 0, id, MPI_COMM_WORLD); // send message result to MASTER
	} else
	{
		// all other threads
		MPI_Recv(&sum, 1, MPI_INT, rank-1, id, MPI_COMM_WORLD, &status); // receive message from preceding thread
		sum += rank;
		MPI_Send(&sum, 1, MPI_INT, rank+1, id, MPI_COMM_WORLD); // send message result to next thread
	}

	// finalizing MPI
	MPI_Finalize();
}
