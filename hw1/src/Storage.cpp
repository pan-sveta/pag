#include <vector>
#include <numeric>
#include <algorithm>
#include <iterator>

#include "Utils.h"

typedef vector<pair<int, pair<int, int>>> graphT;

int calculateFrankenstein(const vector<int> &wordA, const vector<int> &wordB);

int kruskal(graphT &edges, const size_t& nodesCount);


using namespace std;


int main(int argc, char *argv[]) {
    auto programArguments = ProgramArguments::Parse(argc, argv);

    vector<vector<int>> records = readRecords(programArguments.mInputFilePath);

    const size_t nodesCount = records.size();

    auto start = high_resolution_clock::now();

    #pragma omp declare reduction (merge : graphT : omp_out.insert(omp_out.end(), omp_in.begin(), omp_in.end()))
    int i,j;
    graphT edges;
    #pragma omp parallel for schedule(static, 1) private(j) reduction(merge: edges)
    for (i = 0; i < nodesCount - 1; ++i) {
        for (j = i + 1; j < nodesCount; ++j) {
            int distance = calculateFrankenstein(records[i], records[j]);

            edges.push_back(make_pair(distance, make_pair(i, j)));
            edges.push_back(make_pair(distance, make_pair(j, i)));
        }
    }

    auto stopGraph = high_resolution_clock::now();
    auto durationGraph = duration_cast<milliseconds>(stopGraph - start);

    cout << durationGraph.count() << " ms to create graph" << endl;

    int treeCost = kruskal(edges, nodesCount);

    cout << treeCost << endl;

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << duration.count() << " ms to kruskal" << endl;

    writeCost(treeCost, programArguments.mOutputFilePath);

    return 0;
}

int calculateFrankenstein(const vector<int> &wordA, const vector<int> &wordB) {
    if (wordA.empty()) {
        return wordB.size();
    }

    if (wordB.empty()) {
        return wordA.size();
    }

    const int arr_size = wordA.size() + 1;
    int lineA[arr_size];
    int lineB[arr_size];

    for (int i = 0; i < arr_size; i++) {
        lineA[i] = i;
    }

    for (int i = 1; i <= wordB.size(); i++) {
        lineB[0] = i;
        const int letterB = wordB[i - 1];

        for (int j = 1; j <= wordA.size(); ++j) {
            const int letterA = wordA[j - 1];


            int cost;
            if (letterA == letterB)
                cost = 0;
            else
                cost = 1;

            const int diagonal = lineA[j - 1];
            const int left = lineB[j - 1];
            const int above = lineA[j];

            int cell = min(above + 1, min(left + 1, diagonal + cost));
            lineB[j] = cell;
        }

        for (int j = 0; j < arr_size; j++)
            lineA[j] = lineB[j];
    }

    return lineB[arr_size - 1];
}


int parent(int x, vector<int>& id)
{
    while(id[x] != x)
    {
        id[x] = id[id[x]];
        x = id[x];
    }

    return x;
}

int kruskal(graphT &edges, const size_t& nodesCount) {
    int minimumCost = 0;

    vector<int> id(nodesCount);
    iota(id.begin(),id.end(),0);

    sort(edges.begin(), edges.end());

    for (int i = 0; i < edges.size(); ++i) {
        int x = edges[i].second.first;
        int y = edges[i].second.second;
        int cost = edges[i].first;

        if (parent(x, id) != parent(y, id)) {
            minimumCost += cost;

            id[parent(x, id)] = id[parent(y, id)];
        }
    }

    return minimumCost;
}

