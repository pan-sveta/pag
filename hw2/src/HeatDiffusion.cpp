#include <mpi.h>
#include <chrono>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <algorithm>
#include <tuple>
#include <iostream>
#include <cmath>

using namespace std;
using namespace std::chrono;

#define ROOT_PROCESS 0

struct Problem {
    int width;
    int height;
    int rootSize;
    int slaveSize;
};

// Spot with permanent temperature on coordinates [x,y].
struct Spot {
    int mX;
    int mY;
    float mTemperature;

    bool operator==(const Spot &b) const {
        return (mX == b.mX) && (mY == b.mY);
    }
};

bool compareByY(const Spot &a, const Spot &b) {
    return a.mY < b.mY;
}

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

MPI_Datatype CreateMpiProblemType() {
    MPI_Datatype problem_type;
    int lengths[4] = {1, 1, 1, 1};

    MPI_Aint displacements[4];
    Problem dummy_problem = {};
    MPI_Aint base_address;
    MPI_Get_address(&dummy_problem, &base_address);
    MPI_Get_address(&dummy_problem.width, &displacements[0]);
    MPI_Get_address(&dummy_problem.height, &displacements[1]);
    MPI_Get_address(&dummy_problem.rootSize, &displacements[2]);
    MPI_Get_address(&dummy_problem.slaveSize, &displacements[3]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);
    displacements[2] = MPI_Aint_diff(displacements[2], base_address);
    displacements[3] = MPI_Aint_diff(displacements[3], base_address);
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Type_create_struct(4, lengths, displacements, types, &problem_type);
    MPI_Type_commit(&problem_type);

    return problem_type;
}

vector<Spot> distributeSpots(const vector<vector<Spot>> &chunkedSpots, MPI_Datatype MPI_SPOT_TYPE) {
    for (int i = 1; i < chunkedSpots.size(); ++i) {
        int size = chunkedSpots[i].size();
        MPI_Send(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

        if (size > 0)
            MPI_Send(&chunkedSpots[i][0],
                     size,
                     MPI_SPOT_TYPE,
                     i,
                     0,
                     MPI_COMM_WORLD);

    }

    return chunkedSpots[0];
}

vector<Spot> receiveSpots(MPI_Datatype MPI_SPOT_TYPE) {
    int size;

    MPI_Recv(&size, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (size < 1)
        return vector<Spot>(0);

    vector<Spot> spots(size);

    MPI_Recv(&spots[0],
             size,
             MPI_SPOT_TYPE,
             0,
             MPI_ANY_TAG,
             MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);

    return spots;
}

float calculateIteration(vector<vector<float>> &matrix, vector<Spot> &spots, const Problem &problem) {
    float diff = 0;

    for (int y = 0; y < matrix.size(); ++y) {
        for (int x = 0; x < problem.width; ++x) {
            //TODO: Create something smarter
            bool isSpot = false;
            for (auto spot: spots)
                if (spot.mX == x && spot.mY == y)
                    isSpot = true;
            if (isSpot)
                continue;

            float sum = 0;
            sum += matrix[x][y];

            float count = 1;

            if (x > 0) {
                //Left
                sum += matrix[x - 1][y];
                count++;
            }
            if (y > 0) {
                //Top TODO
                sum += matrix[x][y - 1];
                count++;
            }
            if (y > 0 && x < problem.width - 1) {
                //Top right TODO
                sum += matrix[x + 1][y - 1];
                count++;
            }
            if (y > 0 && x > 0) {
                //Top left TODO
                sum += matrix[x - 1][y - 1];
                count++;
            }
            if (x < problem.width - 1) {
                //Right
                sum += matrix[x + 1][y];
                count++;
            }
            if (y < matrix.size() - 1) {
                //Bottom TODO
                sum += matrix[x][y + 1];
                count++;
            }
            if (y < matrix.size() - 1 && x < problem.width - 1) {
                //Bottom right TODO
                sum += matrix[x + 1][y + 1];
                count++;
            }
            if (y < matrix.size() - 1 && x > 0) {//Bottom left TODO
                sum += matrix[x - 1][y + 1];
                count++;
            }

            float newTemperature = sum / count;
            diff = max(abs(matrix[x][y] - newTemperature), diff);

            matrix[x][y] = newTemperature;
        }
    }

    return diff;
}

void printMe(const std::vector<Spot> &spots, const int &rank) {
    stringstream ss;

    ss << "Content of processor with rank " << rank << ": {" << endl;
    for (auto spot: spots) {
        ss << "\tX:" << spot.mX << " Y:" << spot.mY << " Temperature:" << spot.mTemperature << endl;
    }
    ss << "}" << endl;

    cout << ss.str();
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

    auto MPI_SPOT_TYPE = CreateMpiSpotType();
    auto MPI_PROBLEM_TYPE = CreateMpiProblemType();

    Problem problem;

    if (myRank == ROOT_PROCESS) {
        problem.height = height;
        problem.width = width;
        problem.slaveSize = height / worldSize;
        problem.rootSize = height - (problem.slaveSize * (worldSize - 1));;
    }

    MPI_Bcast(&problem, 1, MPI_PROBLEM_TYPE, ROOT_PROCESS, MPI_COMM_WORLD);

    vector<vector<float>> matrix;
    vector<Spot> assignedSpots;
    if (myRank == ROOT_PROCESS) {
        //Calculate spots com
        std::sort(spots.begin(), spots.end(), compareByY);
        vector<vector<Spot>> chunkedSpots(worldSize, vector<Spot>());

        int processor = 1;
        for (auto &spot: spots) {
            if (spot.mY < problem.rootSize)
                chunkedSpots[0].push_back(spot);
            else if (spot.mY) {
                while (spot.mY >= problem.rootSize + processor * problem.slaveSize) {
                    processor++;
                }

                chunkedSpots[processor].push_back(spot);
            }
        }

        assignedSpots = distributeSpots(chunkedSpots, MPI_SPOT_TYPE);
        printMe(assignedSpots, myRank);

        //Create matrix
        matrix = vector<vector<float>>(problem.width, vector<float>(problem.rootSize, 128));

        //Fill spots
        for (auto spot: assignedSpots) {
            matrix[spot.mX][spot.mY - (problem.rootSize + (myRank - 1) * problem.slaveSize)] = spot.mTemperature;
        }
    } else {
        assignedSpots = receiveSpots(MPI_SPOT_TYPE);
        printMe(assignedSpots, myRank);

        matrix = vector<vector<float>>(problem.width, vector<float>(problem.slaveSize, 128));

        for (auto spot: assignedSpots) {
            matrix[spot.mX][spot.mY - (problem.rootSize + (myRank - 1) * problem.slaveSize)] = spot.mTemperature;
        }
    }

    float maxDif;
    do {
        float myDiff = calculateIteration(matrix, spots, problem);
        vector<float> diffs(worldSize);

        MPI_Allgather(&myDiff,
                      1,
                      MPI_FLOAT,
                      &diffs[0],
                      1,
                      MPI_FLOAT,
                      MPI_COMM_WORLD);

        maxDif = *max_element(diffs.begin(), diffs.end());
        cout << "IT MAX DIF: " << maxDif << endl;
    } while (maxDif >= 0.00001);

    vector<float> message(0);
    if (myRank != ROOT_PROCESS) {
        message = vector<float>(problem.width * problem.slaveSize);
        for (int y = 0; y < problem.slaveSize; ++y) {
            for (int x = 0; x < problem.width; ++x) {
                message.push_back(matrix[x][y]);
            }
        }
    }

    vector<float> temperatures;
    int index = 0;

    if (myRank == ROOT_PROCESS) {
        temperatures = vector<float>(problem.width * problem.height);
        for (int y = 0; y < problem.rootSize; ++y) {
            for (int x = 0; x < problem.width; ++x) {
                temperatures[index] = matrix[x][y];
                index++;
            }
        }
    }

    cout << "CPU " << myRank << " MESSAGE SIZE: " << message.size() << " TEMPS: " << temperatures.size() << endl;

    const int bufferSize = problem.width * problem.slaveSize * (worldSize - 1);
    vector<float> bugg(bufferSize);
    MPI_Gather(&message[0],
               message.size(),
               MPI_FLOAT,
               &bugg[0],
               problem.width * problem.slaveSize,
               MPI_FLOAT,
               ROOT_PROCESS,
               MPI_COMM_WORLD
    );

    for (auto fo: bugg) {
        cout << fo << ";";
    }



// TODO: Fill this array on processor with rank 0. It must have height * width elements and it contains the
// linearized matrix of temperatures in row-major order
// (see https://en.wikipedia.org/wiki/Row-_and_column-major_order)


//-----------------------\\

    double totalDuration = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    /*cout << "computational time: " << totalDuration << " s" <<
         endl;*/

    if (myRank == 0) {
        string outputFileName(argv[2]);
        writeOutput(myRank, width, height, outputFileName, temperatures
        );
    }

    MPI_Finalize();

    return 0;
}

