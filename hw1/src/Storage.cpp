#include <vector>
#include <numeric>
#include <algorithm>
#include <iterator>
#include <queue>
#include <map>

#include "Utils.h"

typedef vector<vector<int>> graph;

int calculateFrankenstein(const vector<int> &wordA, const vector<int> &wordB);

int kruskal(graph &graph, const size_t& nodesCount);

using namespace std;


int main(int argc, char *argv[]) {
    auto programArguments = ProgramArguments::Parse(argc, argv);

    vector<vector<int>> records = readRecords(programArguments.mInputFilePath);
    const size_t nodesCount = records.size();

    auto start = high_resolution_clock::now();

    graph graph(nodesCount, vector<int>(nodesCount));
    int i,j;
    #pragma omp parallel for schedule(static, 1) private(j)
    for (i = 0; i < nodesCount - 1; ++i) {
        for (j = i + 1; j < nodesCount; ++j) {
            int distance = calculateFrankenstein(records[i], records[j]);
            graph[i][j] = distance;
            graph[j][i] = distance;
        }
    }

    auto stopGraph = high_resolution_clock::now();
    auto durationGraph = duration_cast<milliseconds>(stopGraph - start);

    cout << durationGraph.count() << " ms to create graph" << endl;

    int treeCost = kruskal(graph, nodesCount);

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

typedef pair<int, int> weightedEdge;
int kruskal(graph &graph, const size_t& nodesCount) {
    priority_queue<weightedEdge, vector <weightedEdge>, greater<>> pq;

    int src = 0;

    vector<int> key(nodesCount, INT32_MAX);
    vector<int> parent(nodesCount, -1);
    vector<bool> inMST(nodesCount, false);

    pq.push(make_pair(0, src));
    key[src] = 0;

    while (!pq.empty())
    {
        int u = pq.top().second;
        pq.pop();

        if(inMST[u]){
            continue;
        }

        inMST[u] = true;

        for (int v = 0; v < nodesCount; ++v)
        {
            if (u == v)
                continue;

            int weight = graph[u][v];

            if (!inMST[v] && key[v] > weight)
            {
                key[v] = weight;
                pq.push(make_pair(key[v], v));
                parent[v] = u;
            }
        }
    }

    int sum = 0;

    for (int i = 1; i < nodesCount; ++i){
        sum += graph[parent[i]][i];
    }

    return sum;
}

