#ifndef _MASTER_WORKER_H_
#define _MASTER_WORKER_H_

#include <cstdlib>
#include <unistd.h>
#include <mpi.h>
using namespace std;

// Worker commands
enum { MW_ask_for_job, MW_return_result, MW_job_done };
// Flags for various messages
enum { MW_listen_tag, MW_send_job_data_tag, MW_recv_result_data_tag };
// Status flags of Workers
enum { MW_worker_free, MW_worker_working, MW_worker_suspended };

// **********
class Master
{
	int num_working_workers;
	MPI_Comm comm;
	int num_workers, *working;
	MPI_Datatype result_dt;
	int MPI_Count;

	public:
	Master(int num_workerss, MPI_Datatype result_dts, int MPI_Count);
	~Master(void);
	bool some_working(void);
	int listen(int *command);
	void send_work(int worker_rank, void *data);
	void get_result(int worker_rank, void *data);
	void free_worker(int worker_rank);
	void suspend_worker(int worker_rank);
};

// **********
Master::Master(int num_workerss, MPI_Datatype result_dts, int MPI_Counts) :
	num_working_workers(0), num_workers(num_workerss), result_dt(result_dts), MPI_Count(MPI_Counts)
{
	working = new int[num_workers];
	for (int i=0; i<num_workers; ++i)
		working[i] = MW_worker_free;
}

// **********
Master::~Master()
{
	int worker_rank, command;
	while (num_workers>0)
	{
		if (worker_rank = listen(&command))
		{
			if (command == MW_ask_for_job)
				suspend_worker(worker_rank);
			else
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		}
		usleep(1);
	}
	delete[] working;
}

// **********
bool Master::some_working()
{
	return num_working_workers>0;
}

// **********
int Master::listen(int *command)
{
	MPI_Status status;
	int flag;
	MPI_Iprobe(MPI_ANY_SOURCE, MW_listen_tag, MPI_COMM_WORLD, &flag, &status);
	if (flag)
	{
		MPI_Recv(command, 1, MPI_INT, status.MPI_SOURCE, MW_listen_tag, MPI_COMM_WORLD, &status);
		return status.MPI_SOURCE;
	}
	return 0;
}

// **********
void Master::send_work(int worker_rank, void *data)
{
	MPI_Send(data, MPI_Count, MPI_INT, worker_rank, MW_send_job_data_tag, MPI_COMM_WORLD);
	if (working[worker_rank-1] == MW_worker_free)
	{
		working[worker_rank-1] = MW_worker_working;
		++num_working_workers;
	}
}

// **********
void Master::get_result(int worker_rank, void *data)
{
	MPI_Status status;
	MPI_Recv(data, MPI_Count, result_dt, worker_rank, MW_recv_result_data_tag, MPI_COMM_WORLD, &status);
}

// **********
void Master::free_worker(int worker_rank)
{
	if (working[worker_rank-1] == MW_worker_working)
		--num_working_workers;
	working[worker_rank-1] = MW_worker_free;
}

// **********
void Master::suspend_worker(int worker_rank)
{
	MPI_Send(NULL, 0, MPI_INT, worker_rank, MW_send_job_data_tag, MPI_COMM_WORLD);
	if (working[worker_rank-1] != MW_worker_suspended)
		--num_workers;
	working[worker_rank-1] = MW_worker_suspended;
}

// **********
class Worker
{
	MPI_Datatype result_dt;
	int MPI_Count;

	public:
	Worker(MPI_Datatype result_dts, int MPI_Count);
	bool get_work(void *data);
	void send_result(void *data);
	void done(void);
};

// **********
Worker::Worker(MPI_Datatype result_dts, int MPI_Counts) : result_dt(result_dts), MPI_Count(MPI_Counts)
{
}

// **********
bool Worker::get_work(void *data)
{
	int command = MW_ask_for_job, count;
	MPI_Status status;

	MPI_Send(&command, 1, MPI_INT, 0, MW_listen_tag, MPI_COMM_WORLD);
	MPI_Recv(data, MPI_Count, MPI_INT, 0, MW_send_job_data_tag, MPI_COMM_WORLD, &status);
	MPI_Get_count(&status, MPI_INT, &count);

	return count>0;
}

// **********
void Worker::send_result(void *data)
{
	int command = MW_return_result;

	MPI_Send(&command, 1, MPI_INT, 0, MW_listen_tag, MPI_COMM_WORLD);
	MPI_Send(data, MPI_Count, result_dt, 0, MW_recv_result_data_tag, MPI_COMM_WORLD);
}

// **********
void Worker::done()
{
	int command = MW_job_done;
	MPI_Send(&command, 1, MPI_INT, 0, MW_listen_tag, MPI_COMM_WORLD);
}

#endif
