#include <vector>
#include <numeric>
#include <algorithm>
#include <iterator>
#include <queue>
#include <map>

#include "Utils.h"

typedef vector<vector<int>> graph;

int calculateFrankenstein(const vector<int> &wordA, const vector<int> &wordB);

int primPQ(graph &graph, const size_t& nodesCount);

using namespace std;


int main(int argc, char *argv[]) {
    auto programArguments = ProgramArguments::Parse(argc, argv);

    //Read records
    vector<vector<int>> records = readRecords(programArguments.mInputFilePath);
    const size_t nodesCount = records.size();

    //Begin time measurement
    auto start = high_resolution_clock::now();


    //Initialize adjacency matrix
    graph graph(nodesCount, vector<int>(nodesCount));

    //Execute the loop in parallel for each word
    int i,j;
    #pragma omp parallel for schedule(dynamic, 1)  private(j)
    for (i = 0; i < nodesCount - 1; ++i) {
        for (j = i + 1; j < nodesCount; ++j) {
            int distance = calculateFrankenstein(records[i], records[j]);
            graph[i][j] = distance;
            graph[j][i] = distance;
        }
    }

    //Measure time to graph preparation
    auto stopGraph = high_resolution_clock::now();
    auto durationGraph = duration_cast<milliseconds>(stopGraph - start);
    cout << durationGraph.count() << " ms to create graph" << endl;

    //Calculate the prim tree cost
    int treeCost = primPQ(graph, nodesCount);
    cout << treeCost << endl;

    //Measure time to completion
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << duration.count() << " ms to primPQ" << endl;

    //Write the result
    writeCost(treeCost, programArguments.mOutputFilePath);

    return 0;
}

int calculateFrankenstein(const vector<int> &wordA, const vector<int> &wordB) {
    //If word A is empty no point to calculate anything
    if (wordA.empty())
        return wordB.size();

    //If word B is empty no point to calculate anything
    if (wordB.empty())
        return wordA.size();

    //Store word A length and use it as an X dimension size
    const int lineLength = wordA.size() + 1;
    int lineA[lineLength];
    int lineB[lineLength];

    //Fill first line with 1,2,...,lineLength
    for (int i = 0; i < lineLength; i++) {
        lineA[i] = i;
    }

    //Go vertically
    for (int i = 1; i <= wordB.size(); i++) {
        lineB[0] = i;
        const int letterB = wordB[i - 1];

        //Go horizontally
        for (int j = 1; j <= wordA.size(); ++j) {
            const int letterA = wordA[j - 1];

            //Check if letters are equal
            int cost = letterA == letterB ? 0 : 1;

            //Get field from the table
            const int diagonal = lineA[j - 1];
            const int left = lineB[j - 1];
            const int above = lineA[j];

            //Calculate correct cost for current number
            int cell = min(above + 1, min(left + 1, diagonal + cost));
            lineB[j] = cell;
        }

        //Move lineB to lineA
        for (int j = 0; j < lineLength; j++)
            lineA[j] = lineB[j];
    }

    //Return last element
    return lineB[lineLength - 1];
}

typedef pair<int, int> weightedEdge;

//Priority queu Prim algorithm
int primPQ(graph &graph, const size_t& nodesCount) {
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

