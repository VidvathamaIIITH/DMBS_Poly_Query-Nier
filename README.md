# PolyRA: High-Performance Multi-Model Relational Algebra Engine

## 1. EXECUTIVE SUMMARY

**PolyRA** (formerly SimpleRA) is an elite-tier, integer-optimized Relational Database Management System (RDBMS) engineered for extreme performance and multi-model data representations. Originally conceived as a minimalist relational algebra engine, PolyRA has evolved into a robust infrastructure platform capable of seamlessly executing standard relational operations, advanced graph processing algorithms, and high-dimensional vector similarity searches within a unified memory and transaction architecture.

Designed for stringent performance and memory-constrained environments, PolyRA leverages a custom block-based Buffer Manager, external optimization algorithms (BTree, Hash), and a strict Wait-Die concurrency control protocol to deliver scalable multi-threaded transaction throughput with zero deadlock guarantees.

---

## 2. SYSTEM ARCHITECTURE

PolyRA's architecture is composed of strictly decoupled, highly cohesive subsystems designed for maintainability and extensibility.

### 2.1 Core Components
- **Syntactic & Semantic Parsers**: Employs a zero-copy lexical analysis strategy to tokenize input commands. The Semantic Parser validates structural integrity and schema constraints by cross-referencing the global Catalogues before generating execution plans.
- **Buffer Management (Unified Memory Hierarchy)**: 
  - `BufferManager`: Implements strict FIFO eviction policies for standard table operations, dynamically paging heavily-accessed blocks.
  - `MatrixBufferManager`: A specialized extension with a hard limit of 10 pages for multi-dimensional matrix operations, guaranteeing predictable memory ceilings during complex vector operations.
- **Transaction & Concurrency Control**:
  - `TransactionManager` orchestrates multi-threaded execution.
  - `LockSimulator` manages an intricate matrix of Shared (`S`) and Exclusive (`X`) locks mapped at the granularity of `(Table/Matrix, Page)`.
- **Catalogue Managers**: Maintains real-time schema and metadata registries (`TableCatalogue`, `GraphCatalogue`, `MatrixCatalogue`) globally synced across execution contexts.
- **Executors**: Granular processing routines directly implementing the mathematical operators over cursors without realizing entire datasets into memory.

---

## 3. CORE MECHANICS & MATHEMATICAL FORMULATION

### 3.1 Relational Algebra (Phase 1 & 3)
PolyRA supports standard RA operations with external memory algorithms:
- **Projection & Selection**: `π_{A,B}(R)`, `σ_{cond}(R)`. Executed iteratively over blocks.
- **Join & Cross**: `R ⨝ S`, `R × S`. Implements nested-loop join paradigms with block-level caching.
- **Sort & Group By**: External Merge Sort limits RAM exhaustion, while Hash-based Group By (`γ_{A, AGG(B)}(R)`) pipelines aggregates dynamically.

### 3.2 Graph Processing (Phase 2)
Treats graphs as specialized dual-table mappings (Nodes and Edges).
- **PATH Check**: `PATH(G, src, dest, [cond])`. Implements Breadth-First Search (BFS) directly over disk-backed Edge blocks. Supports inline evaluation of `NODE` and `EDGE` conditions.
- **DEGREE Check**: Fast lookup of out/in-degrees via direct catalog indices.

### 3.3 High-Dimensional Vector DB (Phase 4)
PolyRA treats vectors as `Matrix` constructs, partitioned into `MatrixPage` models.
- **K-Nearest Neighbors (KNN)**: 
  - Iterates over dense vectors evaluating distance:
    - **Euclidean**: $d(A, B) = \sqrt{\sum (A_i - B_i)^2}$
    - **Cosine**: $S_c(A, B) = \frac{\sum (A_i \times B_i)}{\sqrt{\sum A_i^2} \times \sqrt{\sum B_i^2}}$
  - Implements a priority Min-Heap tracking the `TOP <X>` elements. Distances are scaled ($10000 \times dist$) and returned as natively cast Integer Tables for downstream relational composition.

---

## 4. PERFORMANCE & CONSTRAINTS

- **Wait-Die Protocol**: Uses strict temporal timestamps. If a newer transaction (T_new) requests a lock held by an older one (T_old), T_new is aborted and restarted (Dies). If T_old requests from T_new, T_old waits. This guarantees deterministic progress and deadlock freedom.
- **Memory Ceiling**: Standard buffer is constrained block-wise; the `MatrixBufferManager` operates under an isolated 10-page maximum threshold, prioritizing active queries over historical caching.
- **Integer Exclusivity**: Standard relation data natively handles `int` types only, minimizing CPU cache-miss penalties. Vector elements parse as `float` purely for computation before distance scalar casting.

---

## 5. SETUP & TEST EXECUTION

### Prerequisites
- `g++` compiler supporting C++17.
- Minimum 2GB RAM (optimally configured for concurrent threads).
- `make` build system.

### Build Instructions
```bash
# Clean artifact directories
make clean

# Compile the PolyRA server executable
make
```

### Execution
The server reads `.ra` query files or interactive shell inputs.
```bash
./server
> SOURCE test_vector
```

### Testing Framework
PolyRA includes automated python test data synthesis tools:
```bash
# Generate high-dimensional vectors and RA scripts
python tests/test_vector.py

# Execute generated script via PolyRA
./server < data/test_vector.ra
```
