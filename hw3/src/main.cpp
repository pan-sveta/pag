#include <iostream>
#include <mpi.h>
#include "Worker.h"


int main(int argc, char **argv) {
    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    MPI_Init(&argc, &argv);
    int worldSize, myRank;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    Worker worker(myRank, worldSize, outputPath);


    worker.InitialTasksDistribution(inputPath);



    worker.InitialJobDistribution();
    //Assign initial jobs
    if (myRank == 0) {

        worker.token = GREEN_TOKEN;
    }

    worker.WorkingLoop();


    MPI_Finalize();

    return 0;
}
