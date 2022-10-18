#include <vector>
#include <numeric>

#include "Utils.h"

int calculateFrankenstein(const vector<int> &lineA, const vector<int> &lineB);

using namespace std;

int main(int argc, char *argv[]) {
    auto programArguments = ProgramArguments::Parse(argc, argv);

    // The input records, e.g., records[0] is the first record in the input file.
    vector<vector<int>> records = readRecords(programArguments.mInputFilePath);

    vector<vector<int>> graph(records.size(), vector<int>(records.size()));

    #pragma omp parallel for collapse(2)
    for (int i = 0; i < records.size() - 1; ++i) {
        for (int j = i + 1; j < records.size(); ++j) {
            int distance = calculateFrankenstein(records[i], records[j]);

            graph[i][j] = distance;
            graph[j][i] = distance;
        }
    }

    // TODO: fill the treeCost variable with the MST of the records' edit distances graph.
    int treeCost = 0;

    cout << treeCost << endl;
    writeCost(treeCost, programArguments.mOutputFilePath);
}

int calculateFrankenstein(const vector<int> &lineA, const vector<int> &lineB) {
    if (lineA.empty()) {
        return lineB.size();
    }

    if (lineB.empty()) {
        return lineA.size();
    }

    std::vector<std::vector<int>> matrix(lineA.size() + 1, std::vector<int>(lineB.size()+1));

    //Fill vertical
    iota(matrix[0].begin(), matrix[0].end(), 0);

    //Fill horizontal
    for (int i = 0; i <= lineA.size(); i++) {
        matrix[i][0] = i;
    }


    for (int pointerA = 1; pointerA <= lineA.size(); pointerA++) {

        const int aMember = lineA[pointerA - 1];

        for (int pointerB = 1; pointerB <= lineB.size(); pointerB++) {

            const char bMember = lineB[pointerB - 1];

            int cost;
            if (aMember == bMember) {
                cost = 0;
            } else {
                cost = 1;
            }

            const int diagonal = matrix[pointerA - 1][pointerB - 1];
            const int left = matrix[pointerA][pointerB - 1];
            const int above = matrix[pointerA - 1][pointerB];

            int cell = min(above + 1, min(left + 1, diagonal + cost));

            //Check transposition
            if (pointerA > 2 && pointerB > 2) {
                int transposition = matrix[pointerA - 2][pointerB - 2] + 1;

                if (lineA[pointerA - 2] != bMember)
                    transposition++;

                if (aMember != lineB[pointerB - 2])
                    transposition++;

                if (cell > transposition)
                    cell = transposition;
            }

            matrix[pointerA][pointerB] = cell;
        }
    }

    return matrix[lineA.size()][lineB.size()];
}
