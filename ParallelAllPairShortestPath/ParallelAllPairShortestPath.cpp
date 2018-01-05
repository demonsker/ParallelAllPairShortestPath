// ParallelAllPairShortestPath.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include <mpi.h>
#include <stdlib.h>

#define INF 999999
#define SIZE 8

void generate(int **);
void initialize(int **, int **);
void findAllPairShortestPath(int **, int);
int getSizePerProcess(int);
int getBeginIndexFromInput();
void printProcess(int **, int n);
void print(int **);
void useExampleData(int **);

int world_size, world_rank;

int main(int argc, char** argv) {
	//Before setup MPI
	system("@echo Before setup %time%");

	// Initialize the MPI environment
	MPI_Init(NULL, NULL);

	// Get the number of processes
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// Get the rank of the process
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	//Number of row per processor
	int n = getSizePerProcess(world_rank);

	//create memory for receive
	int i;
	int **mpart;
	mpart = (int**)malloc((n + 1) * sizeof(int*));
	for (i = 0; i < n + 1; i++)
	{
		mpart[i] = (int*)malloc(SIZE * sizeof(int));
	}	

	//Master
	if (world_rank == 0)
	{
		//declare data for generate
		int **data;
		data = (int**)malloc(SIZE * sizeof(int*));
		int i, j;
		for (i = 0; i < SIZE; i++)
		{
			data[i] = (int*)malloc(SIZE * sizeof(int));
		}

		//After setup
		system("@echo After setup %time%");

		//Generate data
		//generate(data);
		useExampleData(data);

		//Start time
		system("@echo Start %time%");

		//declare input
		int **distance;
		distance = (int**)malloc(SIZE * sizeof(int*));
		for (i = 0; i < SIZE; i++)
		{
			distance[i] = (int*)malloc(SIZE * sizeof(int));
		}

		initialize(data, distance);

		//Send data partition to another processor
		int begin, end, np;
		end = getSizePerProcess(0);
		for (i = 1; i < world_size; i++)
		{
			np = getSizePerProcess(i);
			begin = end;
			end = begin + np;
			MPI_Send(distance[0], SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
			for (j = begin; j < end; j++)
				MPI_Send(distance[j], SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
		}
		//partition data for master 
		for (i = 0; i < n; i++)
			for (j = 0; j < SIZE; j++)
			{
				if (i == 0)
					mpart[0][j] = distance[0][j];
				mpart[i + 1][j] = distance[i][j];
			}
	}
	//Slave
	else
	{
		//receive data from master
		for (i = 0; i <= n; i++)
			MPI_Recv(mpart[i], SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	
	//Find shrotest path
	findAllPairShortestPath(mpart, n);

	//Finish time
	system("@echo Finish %time%");

	printProcess(mpart, n);

	MPI_Finalize();
}

void findAllPairShortestPath(int **graph, int n)
{
	//k = number of pass node
	int k = 0;

	//get postion of data from input
	int begin = getBeginIndexFromInput();
	int end = begin + getSizePerProcess(world_rank);

	//Loop when k < SIZE
	while (1)
	{
		//find min route  when pass node k
		for (int i = 1; i <= n; i++)
		{
			for (int j = 0; j < SIZE; j++)
			{
				if (graph[i][k] + graph[0][j] < graph[i][j])
					graph[i][j] = graph[i][k] + graph[0][j];
			}
		}

		//check k < SIZE
		if (++k >= SIZE) break;

		//Find processor that must send pass node data
		if (k >= begin && k < end)
		{
			// index of pass node data
			int index = k - begin + 1;

			int  j, r;

			//send pass node data to another processor
			for (r = 0; r < world_size; r++)
				if (r != world_rank) //not send to itself
					MPI_Send(graph[index], SIZE, MPI_INT, r, 0, MPI_COMM_WORLD);

			//update pass node data self
			for (j = 0; j < SIZE; j++)
				graph[0][j] = graph[index][j];
		}
		else
		{	//receive pass node data
			MPI_Recv(graph[0], SIZE, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	}
	//printf("Processor : %d\n",rank);
	//Print(graph,n);
}

void generate(int **data)
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

void initialize(int **sour, int **dest)
{
	int i, j;

	for (i = 0; i < SIZE; i++)
	{
		for (j = 0; j < SIZE; j++)
		{
			dest[j][i] = sour[i][j];
		}
	}
}

void printProcess(int **distance, int n)
{
	printf("Shortest distances of Process %d \n",world_rank);

	for (int i = 1; i <= n; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			if (distance[i][j] == INF)
				printf("%7s", "INF");
			else
				printf("%7d", distance[i][j]);
		}

		printf("\n");
	}
}

void print(int **m)
{
	printf("Shortest distances between every pair of vertices: \n");

	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			if (m[i][j] == INF)
				printf("%7s", "INF");
			else
				printf("%7d", m[i][j]);
		}

		printf("\n");
	}
}

int getSizePerProcess(int rank)
{
	int n = SIZE / world_size;
	int m = SIZE % world_size;
	if (m != 0)
		if (rank < m)
			n++;
	return n;
}
int getBeginIndexFromInput()
{
	if (world_rank == 0) return 0;
	int begin, end, np, i;
	end = getSizePerProcess(0);
	for (i = 1; i < world_size; i++)
	{
		np = getSizePerProcess(i);
		begin = end;
		end = begin + np;
		if (world_rank == i) 
			return begin;
	}
	return -1;
}

void useExampleData(int **data)
{
	int example[SIZE][SIZE] = {
		{ 0,1,9,3,INF,INF,INF,INF },
		{ 1,0,INF,1,INF,3,INF,INF },
		{ 9,INF,0,INF,INF,3,10,INF },
		{ 3,1,INF,0,5,INF,INF,8 },
		{ INF,INF,INF,5,0,2,2,1 },
		{ INF,3,3,INF,2,0,INF,INF },
		{ INF,INF,10,INF,2,INF,0,4 },
		{ INF,INF,INF,8,1,INF,4,0 }
	};

	int i, j;

	for (i = 0; i < SIZE; i++)
	{
		for (j = 0; j < SIZE; j++)
		{
			data[i][j] = example[i][j];
		}
	}
}
