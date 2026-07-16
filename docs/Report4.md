# Simple-VectorDB Engine Extension (Phase 4) Report

## Overview
This report summarizes the implementation of the **Simple-VectorDB Engine Extension** as part of Phase 4. We extended the PolyQuery system to support dense vector storage and similarity search using K-Nearest Neighbors (KNN).

## Architecture Additions

### 1. Vector Matrix Storage
- **`Matrix` and `MatrixPage` Classes:** Vector data is structured into blocks called `matrix_page`. Each page contains an integer count representing the number of vectors, followed by serialized float arrays representing vector coordinates. Each vector is assigned an implicit `RecordID`.
- **`MatrixBufferManager`:** Extended the storage system to have a separate buffer pool specifically for matrices. It limits memory usage dynamically with a 10-page constraint, evicting the oldest unpinned matrix pages.

### 2. Parsers & Syntax
Extended the parser (`syntacticParser` and `semanticParser`) to handle the following novel syntax efficiently:
- `LOAD VECTOR MATRIX <matrix_name> <dimension>`: Loads and blockifies dense vector datasets.
- `<new_table> <- KNN <matrix_name> QUERY_VEC [f1, f2...] TOP <X> METRIC <COSINE|EUCLIDEAN>`: Parses query vectors directly from the string and initializes KNN search.

### 3. Execution Pipeline
- **`executeLOAD_MATRIX()`:** Iterates over a `.csv` matrix line-by-line, splitting and converting features to floats, and saving them to disk iteratively to avoid loading entire matrices into RAM.
- **`executeKNN()`:** Implemented an optimized scan-based K-Nearest Neighbors execution using min-heaps to find the closest vectors to `QUERY_VEC` sequentially, avoiding out-of-memory errors on large vector bases.
  - Supports both **EUCLIDEAN** and **COSINE** distance calculation routines using high-performance C++ `<cmath>` utilities.
  - Distance metrics are normalized/scaled up (`dist * 10000`) and cast to Integers, returning the top K matches as a fully compatible PolyQuery Table schema `(RecordID, Similarity_Score)`.

### 4. Concurrency Control
- Updated `TransactionManager` to perform Wait-Die deadlock prevention on Matrix objects natively.
- Ensures robust Thread-Safe reads by tracking matrix page locks with `matrix_page_<name>_<idx>` identifiers in the shared global `LockSimulator`.

## Testing
We built a Python generator (`tests/test_vector.py`) to synthesize random high-dimensional uniform dense vectors (e.g., 1000x50 dimensions). 
- Tested memory isolation via `MatrixBufferManager`.
- Validated correct similarity sorts and results table creation for both metric variations.
- Ran tests against multi-query scenarios confirming concurrency stability without breaking the preceding Phase 1-3 features.

All original PolyQuery functionalities remain unbroken and integrated natively.
