#include "stdafx.h"
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#define PATH "C:\\Users\\Eucliwood\\Desktop\\stat(SaveMode)\\Parallel\\"
#define INF 999999
#define SIZE 16384

void distance_generate(int[][SIZE]);
void distance_useexample(int[][SIZE]);
void find_AllPairShortestPath(int[][SIZE], int[][SIZE], int);
int get_datasize_per_process(int);
int get_beginindex_frominput(int);
void process_print(int[][SIZE], int n);
void array_print(int[][SIZE]);
void log_save(float);

int world_size, world_rank;

int main(int argc, char** argv) {
	
	double start_1, end_1 , start_2, end_2;

	// Initialize the MPI environment
	MPI_Init(NULL, NULL);

	//Before setup
	start_1 = MPI_Wtime();

	// Get the number of processes
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// Get the rank of the process
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	//Number of row per processor
	int row_per_process = get_datasize_per_process(world_rank);

	//create memory for receive distance , path
	int i, j;
	int (*part_of_distance)[SIZE], (*part_of_path)[SIZE];
	part_of_distance = (int(*)[SIZE]) malloc((row_per_process + 1) * sizeof(int[SIZE]));
	part_of_path = (int(*)[SIZE]) malloc((row_per_process + 1) * sizeof(int[SIZE]));
	for(i = 0; i <= row_per_process; i++)
		for (j = 0; j < SIZE; j++)
		{
			part_of_path[i][j] = j;
		}

	//Master
	if (world_rank == 0)
	{
		//declare input
		int(*distance)[SIZE];
		distance = (int(*)[SIZE]) malloc(SIZE * sizeof(int[SIZE]));

		//After setup
		end_1 = MPI_Wtime();

		//Generate data
		//distance_useexample(distance);
		distance_generate(distance);

		//Start calculate
		start_2 = MPI_Wtime();

		//Send data partition to another processor
		for (i = 1; i < world_size; i++)
		{
			int row_begin = get_beginindex_frominput(i);
			int number_of_row = get_datasize_per_process(i);
			//send first pass node
			MPI_Send(distance[0], SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
			//send parition data to another processor
			MPI_Send(distance[row_begin], number_of_row*SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
		}

		//partition data for master 
		for (i = 0; i < row_per_process; i++)
			for (j = 0; j < SIZE; j++)
			{
				if (i == 0)
					part_of_distance[0][j] = distance[0][j];
				part_of_distance[i + 1][j] = distance[i][j];
			}
	}
	//Slave
	else
	{
		//receive data from master
		MPI_Recv(part_of_distance[0], SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(part_of_distance[1], row_per_process*SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	
	//Find shrotest path
	find_AllPairShortestPath(part_of_distance, part_of_path, row_per_process);

	//End calculate
	end_2 = MPI_Wtime();

	//printf("Process %d : Distance\n", world_rank);
	//process_print(part_of_distance, row_per_process);

	//printf("Process %d : Path\n", world_rank);
	//process_printpartOfPath, n);
	
	float diff = (float)(end_1 - start_1 + end_2 - start_2);
	printf("Time : %.4f\n", diff);
	
	log_save(diff);

	MPI_Finalize();
}

void find_AllPairShortestPath(int part_of_distance[][SIZE],int part_of_path[][SIZE], int row_per_process)
{
	//k = number of pass node
	int k = 0;

	//get postion of data from input
	int row_begin = get_beginindex_frominput(world_rank);
	int row_end = row_begin + get_datasize_per_process(world_rank);

	//Loop when k < SIZE
	while (1)
	{
		//find min route  when pass node k
		for (int i = 1; i <= row_per_process; i++)
		{
			for (int j = 0; j < SIZE; j++)
			{
				if (part_of_distance[i][k] + part_of_distance[0][j] < part_of_distance[i][j])
				{
					part_of_distance[i][j] = part_of_distance[i][k] + part_of_distance[0][j];
					part_of_path[i][j] = part_of_path[i][k];
				}
			}
		}

		//check k < SIZE
		if (++k >= SIZE) break;

		//Find processor that must send pass node data
		if (k >= row_begin && k < row_end)
		{
			// index of pass node data
			int index = k - row_begin + 1;

			int  r;

			//send pass node data to another processor
			for (r = 0; r < world_size; r++)
				if (r != world_rank) //not send to itself
				{
					MPI_Send(part_of_distance[index], SIZE, MPI_INT, r, 0, MPI_COMM_WORLD);
					MPI_Send(part_of_path[index], SIZE, MPI_INT, r, 0, MPI_COMM_WORLD);
				}

			//update pass node data self
			for (r = 0; r < SIZE; r++)
				part_of_distance[0][r] = part_of_distance[index][r];
		}
		else
		{	//receive pass node data
			MPI_Recv(part_of_distance[0], SIZE, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(part_of_path[0], SIZE, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	}
}

void distance_generate(int data[][SIZE])
{
	int i, j, r;

	for (i = 0; i < SIZE; i++)
	{
		data[i][i] = 0;
		for (j = i + 1; j < SIZE; j++)
		{
			r = (rand() % 20) + 1;
			if (r == 19)
				data[i][j] = INF;
			else
				data[i][j] = r;
			data[j][i] = data[i][j];
		}
	}
}

void process_print(int distance[][SIZE], int n)
{
	for (int i = 1; i <= n; i++)
	{
		for (int j = 0; j < SIZE; j++)
		{
			if (distance[i][j] == INF)
				printf("%7s", "INF");
			else
				printf("%7d", distance[i][j]);
		}

		printf("\n");
	}
}

void array_print(int data[][SIZE])
{
	for (int i = 0; i < SIZE; i++)
	{
		for (int j = 0; j < SIZE; j++)
		{
			if (data[i][j] == INF)
				printf("%7s", "INF");
			else
				printf("%7d", data[i][j]);
		}
		printf("\n");
	}
}

int get_datasize_per_process(int rank)
{
	int n = SIZE / world_size;
	int m = SIZE % world_size;
	if (m != 0)
		if (rank < m)
			n++;
	return n;
}

int get_beginindex_frominput(int world_rank)
{
	if (world_rank == 0) return 0;
	int begin, end, np, i;
	end = get_datasize_per_process(0);
	for (i = 1; i < world_size; i++)
	{
		np = get_datasize_per_process(i);
		begin = end;
		end = begin + np;
		if (world_rank == i) 
			return begin;
	}
	return -1;
}

void distance_useexample(int data[][SIZE])
{
	int i, j;

	int example[8][8] = {
		{ 0,1,9,3,INF,INF,INF,INF },
		{ 1,0,INF,1,INF,3,INF,INF },
		{ 9,INF,0,INF,INF,3,10,INF },
		{ 3,1,INF,0,5,INF,INF,8 },
		{ INF,INF,INF,5,0,2,2,1 },
		{ INF,3,3,INF,2,0,INF,INF },
		{ INF,INF,10,INF,2,INF,0,4 },
		{ INF,INF,INF,8,1,INF,4,0 }
	};

	for (i = 0; i < SIZE; i++)
	{
		for (j = 0; j < SIZE; j++)
		{
			data[i][j] = example[i][j];
		}
	}
}

void log_save(float diff)
{
	int i;
	if (world_rank > 0)
		MPI_Send(&diff, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
	else
	{
		float temp;
		for (i = 1; i < world_size; i++)
		{
			MPI_Recv(&temp, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			if (temp > diff)
				diff = temp;
		}
		printf("Finish Time : %.4f\n", diff);

		FILE * fp;
		char fileName[10];
		char filePath[70] = PATH;

		sprintf(fileName, "%d.txt", SIZE);
		strcat(filePath, fileName);
		fp = fopen(filePath, "a");
		fprintf(fp, "%d Process\n %.4f\n\n", world_size, diff);
		fclose(fp);
	}
}
