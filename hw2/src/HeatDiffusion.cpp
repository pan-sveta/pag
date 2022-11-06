#include <mpi.h>
#include <chrono>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <algorithm>
#include <tuple>
#include <iostream>

using namespace std;
using namespace std::chrono;

#define ROOT_PROCESS 0

// Spot with permanent temperature on coordinates [x,y].
struct Spot {
    int mX;
    int mY;
    float mTemperature;

    bool operator==(const Spot &b) const {
        return (mX == b.mX) && (mY == b.mY);
    }
};

tuple<int, int, vector<Spot>> readInstance(string instanceFileName) {
    int width, height;
    vector<Spot> spots;
    string line;

    ifstream file(instanceFileName);
    if (file.is_open()) {
        int lineId = 0;
        while (std::getline(file, line)) {
            stringstream ss(line);
            if (lineId == 0) {
                ss >> width;
            } else if (lineId == 1) {
                ss >> height;
            } else {
                int i, j, temperature;
                ss >> i >> j >> temperature;
                spots.push_back({i, j, (float) temperature});
            }
            lineId++;
        }
        file.close();
    } else {
        throw runtime_error("It is not possible to open instance file!\n");
    }
    return make_tuple(width, height, spots);
}

void writeOutput(
        const int myRank,
        const int width,
        const int height,
        const string outputFileName,
        const vector<float> &temperatures) {
    // Draw the output image in Netpbm format.
    ofstream file(outputFileName);
    if (file.is_open()) {
        if (myRank == 0) {
            file << "P2\n" << width << "\n" << height << "\n" << 255 << "\n";
            for (auto temperature: temperatures) {
                file << (int) max(min(temperature, 255.0f), 0.0f) << " ";
            }
        }
    }
}

void printHelpPage(char *program) {
    cout << "Simulates a simple heat diffusion." << endl;
    cout << endl << "Usage:" << endl;
    cout << "\t" << program << " INPUT_PATH OUTPUT_PATH" << endl << endl;
}

MPI_Datatype CreateMpiSpotType() {
    MPI_Datatype spot_type;
    int lengths[3] = {1, 1, 1};

    MPI_Aint displacements[3];
    Spot dummy_spot = {};
    MPI_Aint base_address;
    MPI_Get_address(&dummy_spot, &base_address);
    MPI_Get_address(&dummy_spot.mX, &displacements[0]);
    MPI_Get_address(&dummy_spot.mY, &displacements[1]);
    MPI_Get_address(&dummy_spot.mTemperature, &displacements[2]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);
    displacements[2] = MPI_Aint_diff(displacements[2], base_address);
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_FLOAT};

    MPI_Type_create_struct(3, lengths, displacements, types, &spot_type);
    MPI_Type_commit(&spot_type);

    return spot_type;
}

vector<Spot>
distributeMatrixLines(const vector<Spot> &spots, const vector<int> &linesOnProcessors, const vector<int> &bases,
                      MPI_Datatype MPI_SPOT_TYPE) {
    int slaveChunkSize = linesOnProcessors.back();
    int rootChunkSize = linesOnProcessors[ROOT_PROCESS];

    // First, we inform the slaves about the chunk size. According to that, slaves will allocate buffers to receive
    // their chunk of the vector.
    MPI_Bcast(&slaveChunkSize, 1, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);

    // Second, distribute the vector chunks (the first chunk of size rootChunkSize is for the root process).
    vector<Spot> buf(rootChunkSize);      // Root process will also receive its chunk.
    MPI_Scatterv(
            spots.data(),
            linesOnProcessors.data(),
            bases.data(),
            MPI_SPOT_TYPE,
            buf.data(),
            rootChunkSize,
            MPI_SPOT_TYPE,
            ROOT_PROCESS,
            MPI_COMM_WORLD);

    return buf;
}

vector<Spot> receiveMatrixLines(MPI_Datatype MPI_SPOT_TYPE) {
    // Get the chunk size.
    int chunkSize;
    MPI_Bcast(&chunkSize, 1, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);

    // Get the chunk of the input vector.
    vector<Spot> buf(chunkSize);
    MPI_Scatterv(
            NULL,
            NULL,
            NULL,
            MPI_BYTE,
            buf.data(),
            chunkSize,
            MPI_BYTE,
            ROOT_PROCESS,
            MPI_COMM_WORLD);

    cout << "NIG: " << buf.size() << endl;

    return buf;
}

void printMe(const std::vector<Spot> &spots, const int &rank) {
    cout << "Content of processor with rank " << rank << ": {" << endl;
    for (auto spot: spots) {
        cout << "\tX:" << spot.mX << " Y:" << spot.mY << " Temperature:" << spot.mTemperature << endl;
    }
    cout << "}" << endl;
}


int main(int argc, char **argv) {
    // Initialize MPI
    MPI_Init(&argc, &argv);
    int worldSize, myRank;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    if (argc == 1) {
        if (myRank == 0) {
            printHelpPage(argv[0]);
        }
        MPI_Finalize();
        exit(0);
    } else if (argc != 3) {
        if (myRank == 0) {
            printHelpPage(argv[0]);
        }
        MPI_Finalize();
        exit(1);
    }

    // Read the input instance.
    int width, height;  // Width and height of the matrix.
    vector<Spot> spots; // Spots with permanent temperature.
    if (myRank == 0) {
        tie(width, height, spots) = readInstance(argv[1]);
    }

    high_resolution_clock::time_point start = high_resolution_clock::now();


    //-----------------------\\
    // Insert your code here \\
    //        |  |  |        \\
    //        V  V  V        \\

    auto MPI_SPOT_TYPE = CreateMpiSpotType();;

    if (myRank == ROOT_PROCESS) {
        cout << "I am šéfis" << endl;
        //Calcluate number of lines allocated to processors
        int linesSlave = spots.size() / worldSize;
        int linesRoot = spots.size() - (linesSlave * (worldSize - 1));

        //Do magic
        vector<int> linesOnProcessors(worldSize, 0);
        vector<int> bases(worldSize, 0);
        for (int i = 0; i < worldSize; i++) {
            if (i == ROOT_PROCESS) {
                linesOnProcessors[i] = linesRoot;
                bases[i] = 0;
            } else {
                linesOnProcessors[i] = linesSlave;
                bases[i] = linesRoot + (i - 1) * linesSlave;
            }
        }

        vector<Spot> assignedSpots = distributeMatrixLines(spots, linesOnProcessors, bases, MPI_SPOT_TYPE);
        printMe(assignedSpots, myRank);


    } else {
        vector<Spot> assignedSpots = receiveMatrixLines(MPI_SPOT_TYPE);
        printMe(assignedSpots, myRank);
    }



    // TODO: Fill this array on processor with rank 0. It must have height * width elements and it contains the
    // linearized matrix of temperatures in row-major order
    // (see https://en.wikipedia.org/wiki/Row-_and_column-major_order)
    vector<float> temperatures;

    //-----------------------\\

    double totalDuration = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    cout << "computational time: " << totalDuration << " s" << endl;

    if (myRank == 0) {
        string outputFileName(argv[2]);
        writeOutput(myRank, width, height, outputFileName, temperatures);
    }

    MPI_Finalize();
    return 0;
}

