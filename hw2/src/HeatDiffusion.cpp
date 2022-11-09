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

float calculateIteration(vector<vector<float>> &matrix, const vector<vector<bool>> &isSpot, const Problem &problem, const int &myRank,
                         const int &worldSize) {
    float diff = 0;

    vector<float> lineAfterMine(0);
    vector<float> lineBeforeMine(0);

    if (myRank + 1 < worldSize) {
        /*cout << "CPU: " << myRank << " sending my last line to " << myRank + 1 << " receving lineAfterMine from "
             << myRank + 1 << endl;*/
        lineAfterMine = vector<float>(problem.width);

        MPI_Sendrecv(&matrix[matrix.size() - 1][0],
                     problem.width,
                     MPI_FLOAT,
                     myRank + 1,
                     1,
                     &lineAfterMine[0],
                     problem.width,
                     MPI_FLOAT,
                     myRank + 1,
                     2,
                     MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
    }

    if (myRank - 1 >= 0) {
        /*cout << "CPU: " << myRank << " sending my last line to " << myRank - 1 << " receving lineAfterMine from "
             << myRank - 1 << endl;*/
        lineBeforeMine = vector<float>(problem.width);

        MPI_Sendrecv(&matrix[0][0],
                     problem.width,
                     MPI_FLOAT,
                     myRank - 1,
                     2,
                     &lineBeforeMine[0],
                     problem.width,
                     MPI_FLOAT,
                     myRank - 1,
                     1,
                     MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
    }


    /*stringstream ss;
    ss << "CPU: " << myRank << " - LAM - " << lineAfterMine.size() << " - " << lineAfterMine.empty() << " - ";
    for (auto foo: lineAfterMine) {
        ss << foo << ";";
    }
    ss << endl;
    cout << ss.str();

    stringstream ss2;
    ss2 << "CPU: " << myRank << " - LBM - " << lineBeforeMine.size() << " - " << lineBeforeMine.empty() << " - ";
    for (auto foo: lineBeforeMine) {
        ss2 << foo << ";";
    }
    ss2 << endl;
    cout << ss2.str();*/


    //cout << "I HAVE SEND AND RECEIVE EVERYTHING!" << endl;

    MPI_Barrier(MPI_COMM_WORLD);

    for (int y = 0; y < matrix.size(); ++y) {
        for (int x = 0; x < problem.width; ++x) {
            if (isSpot[y][x])
                continue;

            float sum = 0;
            sum += matrix[y][x];

            float count = 1;

            //Left
            if (x > 0) {
                sum += matrix[y][x - 1];
                count++;
            }

            //Right
            if (x < problem.width - 1) {
                sum += matrix[y][x + 1];
                count++;
            }

            //Top
            if (y > 0) {
                sum += matrix[y - 1][x];
                count++;
            } else if (y == 0 && !lineBeforeMine.empty()) {
                sum += lineBeforeMine[x];
                count++;
            }

            //Top right
            if (y > 0 && x < problem.width - 1) {
                sum += matrix[y - 1][x + 1];
                count++;
            } else if (y == 0 && x < problem.width - 1 && !lineBeforeMine.empty()) {
                sum += lineBeforeMine[x + 1];
                count++;
            }

            //Top left
            if (y > 0 && x > 0) {
                sum += matrix[y - 1][x - 1];
                count++;
            } else if (y == 0 && x > 0 && !lineBeforeMine.empty()) {
                sum += lineBeforeMine[x - 1];
                count++;
            }

            //Bottom
            if (y < matrix.size() - 1) {
                sum += matrix[y + 1][x];
                count++;
            } else if (y == matrix.size() - 1 && !lineAfterMine.empty()) {
                sum += lineAfterMine[x];
                count++;
            }

            //Bottom right
            if (y < matrix.size() - 1 && x < problem.width - 1) {
                sum += matrix[y + 1][x + 1];
                count++;
            } else if (y == matrix.size() - 1 && x < problem.width - 1 && !lineAfterMine.empty()) {
                sum += lineAfterMine[x + 1];
                count++;
            }

            //Bottom left
            if (y < matrix.size() - 1 && x > 0) {
                sum += matrix[y + 1][x - 1];
                count++;
            } else if (y == matrix.size() - 1 && x > 0 && !lineAfterMine.empty()) {
                sum += lineAfterMine[x - 1];
                count++;
            }

            /*cout << "X: " << x << "Y: " << y << " CPU: " << myRank << " SUM: " << sum << endl;
            cout << "X: " << x << "Y: " << y << " CPU: " << myRank << " count: " << count << endl;*/

            float newTemperature = sum / count;

            diff = max(abs(matrix[y][x] - newTemperature), diff);

            matrix[y][x] = newTemperature;
        }
    }

    //cout << "diff: " << diff << endl;

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
    vector<vector<bool>> isHotspot;
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
        matrix = vector<vector<float>>(problem.rootSize, vector<float>(problem.width, 128));
        isHotspot = vector<vector<bool>>(problem.rootSize, vector<bool>(problem.width, false));

        //Fill spots
        for (auto spot: assignedSpots) {
            matrix[spot.mY - (problem.rootSize + (myRank - 1) * problem.slaveSize)][spot.mX] = spot.mTemperature;
            isHotspot[spot.mY - (problem.rootSize + (myRank - 1) * problem.slaveSize)][spot.mX] = true;
        }
    } else {
        assignedSpots = receiveSpots(MPI_SPOT_TYPE);
        printMe(assignedSpots, myRank);

        matrix = vector<vector<float>>(problem.slaveSize, vector<float>(problem.width, 128));
        isHotspot = vector<vector<bool>>(problem.slaveSize, vector<bool>(problem.width, false));

        for (auto spot: assignedSpots) {
            matrix[spot.mY - (problem.rootSize + (myRank - 1) * problem.slaveSize)][spot.mX] = spot.mTemperature;
            isHotspot[spot.mY - (problem.rootSize + (myRank - 1) * problem.slaveSize)][spot.mX] = true;
        }
    }

    float maxDif;
    do {
        float myDiff = calculateIteration(matrix, isHotspot, problem, myRank, worldSize);
        vector<float> diffs(worldSize);

        MPI_Allgather(&myDiff,
                      1,
                      MPI_FLOAT,
                      &diffs[0],
                      1,
                      MPI_FLOAT,
                      MPI_COMM_WORLD);

        maxDif = *max_element(diffs.begin(), diffs.end());

        if (myRank == ROOT_PROCESS) {
            /*for (auto dif: diffs)
                cout << dif << ";";
            cout << endl;*/
            //cout << "STEP MAX DIF: " << maxDif << endl;
        }

    } while (maxDif >= 0.0001);

    if (myRank == ROOT_PROCESS)
        cout << "FINAL MAX DIF: " << maxDif << endl;

    vector<float> temperatures(problem.width * problem.height);

    vector<float> message;
    if (myRank == ROOT_PROCESS) {
        message = vector<float>(problem.width * problem.rootSize);
        int index = 0;
        for (int y = 0; y < problem.rootSize; ++y) {
            for (int x = 0; x < problem.width; ++x) {
                message[index] = matrix[y][x];
                index++;
            }
        }
    } else {
        message = vector<float>(problem.width * problem.slaveSize);
        int index = 0;
        for (int y = 0; y < problem.slaveSize; ++y) {
            for (int x = 0; x < problem.width; ++x) {
                message[index] = matrix[y][x];
                index++;
            }
        }
    }

    cout << "CPU " << myRank << " MESSAGE SIZE: " << message.size() << endl;

//    if (myRank == 1){
//        cout << "MESSAGE " << myRank << endl;
//        for (auto fo: message) {
//            cout << fo << ";";
//        }
//    }

    //vector<float> buff(problem.width * problem.slaveSize * (worldSize));
    MPI_Gather(&message[0],
               message.size(),
               MPI_FLOAT,
               &temperatures[0],
               message.size(),
               MPI_FLOAT,
               ROOT_PROCESS,
               MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    /*if (myRank == ROOT_PROCESS) {
        if (myRank == ROOT_PROCESS)
            for (auto fo: temperatures) {
                cout << fo << ";";
            }
    }*/

    //cout << "DONE";

// TODO: Fill this array on processor with rank 0. It must have height * width elements and it contains the
// linearized matrix of temperatures in row-major order
// (see https://en.wikipedia.org/wiki/Row-_and_column-major_order)


//-----------------------\\

    double totalDuration = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    cout << "computational time: " << totalDuration << " s" <<   endl;

    if (myRank == 0) {
        string outputFileName(argv[2]);
        writeOutput(myRank, width, height, outputFileName, temperatures
        );
    }

    MPI_Finalize();

    return 0;
}

