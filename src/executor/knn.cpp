#include "global.h"
#include <queue>
#include <cmath>
#include <algorithm>

using namespace std;

// Heap element
struct KnnResult {
    uint recordId;
    float distance;
    
    // Max-heap: the "largest" distance should be at the top so we can pop it
    bool operator<(const KnnResult& other) const {
        return distance < other.distance;
    }
};

float computeCosine(const vector<float>& A, const vector<float>& B) {
    float dot = 0.0f, normA = 0.0f, normB = 0.0f;
    for (int i = 0; i < A.size(); i++) {
        dot += A[i] * B[i];
        normA += A[i] * A[i];
        normB += B[i] * B[i];
    }
    if (normA == 0 || normB == 0) return 0.0f; // Prevent div by 0
    // Cosine similarity is higher for closer vectors.
    // To make it a "distance" where lower is closer (for our heap), we can do 1 - cosine
    return 1.0f - (dot / (sqrt(normA) * sqrt(normB)));
}

float computeEuclidean(const vector<float>& A, const vector<float>& B) {
    float distSq = 0.0f;
    for (int i = 0; i < A.size(); i++) {
        float diff = A[i] - B[i];
        distSq += diff * diff;
    }
    return sqrt(distSq);
}

void executeKNN()
{
    logger.log("executeKNN");
    Matrix* matrix = matrixCatalogue.getMatrix(parsedQuery.matrixName);
    if (!matrix) {
        cout << "Error: Matrix not loaded." << endl;
        return;
    }
    
    if (parsedQuery.queryVector.size() != matrix->dimension) {
        cout << "Error: Query vector dimension mismatch." << endl;
        return;
    }
    
    // Priority queue to act as max-heap of size X
    priority_queue<KnnResult> pq;
    
    for (int pageIndex = 0; pageIndex < matrix->blockCount; pageIndex++)
    {
        // Pin page to ensure buffer manager limits
        matrixBufferManager.pinPage(matrix->matrixName, pageIndex);
        MatrixPage page = matrixBufferManager.getPage(matrix->matrixName, pageIndex, matrix->dimension);
        
        auto& rows = page.getRows();
        for (auto& row : rows)
        {
            float dist = 0.0f;
            if (parsedQuery.knnMetric == COSINE) {
                dist = computeCosine(parsedQuery.queryVector, row.second);
            } else {
                dist = computeEuclidean(parsedQuery.queryVector, row.second);
            }
            
            if (pq.size() < parsedQuery.knnTopX) {
                pq.push({row.first, dist});
            } else if (dist < pq.top().distance) {
                pq.pop();
                pq.push({row.first, dist});
            }
        }
        
        // Unpin after done processing
        matrixBufferManager.unpinPage(matrix->matrixName, pageIndex);
    }
    
    // Extract results from heap
    vector<KnnResult> results;
    while (!pq.empty()) {
        results.push_back(pq.top());
        pq.pop();
    }
    // Reverse because we popped largest to smallest distance
    reverse(results.begin(), results.end());
    
    // Construct Result Table
    Table* resultTable = new Table(parsedQuery.selectionResultRelationName, {"RecordID", "Similarity_Score"});
    
    for (auto& res : results) {
        vector<int> rowData;
        rowData.push_back((int)res.recordId);
        
        // Scale distance by 10000 to store as int
        // Wait, if it was cosine, distance = 1 - similarity. Let's output distance.
        rowData.push_back((int)(res.distance * 10000.0f));
        
        resultTable->writeRow<int>(rowData);
    }
    
    // Blockify and insert
    resultTable->blockify();
    tableCatalogue.insertTable(resultTable);
    
    cout << "KNN executed successfully. Results stored in " << parsedQuery.selectionResultRelationName << endl;
}
