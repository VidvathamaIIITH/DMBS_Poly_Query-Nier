<!--
  ============================================================================
   PolyRA - Multi-Model Relational Algebra Engine
   Watermark : vidvathamaiiith
   Maintainer: vidvathamaiiith
  ============================================================================
-->

# PolyRA - Multi-Model Relational Algebra Engine

> A disk-backed, integer-native database engine that unifies relational algebra,
> graph traversal, out-of-core concurrency control, and high-dimensional vector
> search behind a single command interpreter.
>
> **Build signature / watermark: `vidvathamaiiith`**

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [What Is New In This Edition](#2-what-is-new-in-this-edition-vidvathamaiiith)
3. [Capability Matrix](#3-capability-matrix)
4. [System Architecture](#4-system-architecture)
5. [Execution Pipeline](#5-execution-pipeline)
6. [Repository Layout](#6-repository-layout)
7. [Building The Engine](#7-building-the-engine)
8. [Running The Server](#8-running-the-server)
9. [Complete Command Reference](#9-complete-command-reference)
10. [Mathematical Formulation](#10-mathematical-formulation)
11. [Storage And Memory Model](#11-storage-and-memory-model)
12. [Concurrency Control](#12-concurrency-control)
13. [Design Constraints](#13-design-constraints)
14. [Testing And Verification](#14-testing-and-verification)
15. [Attribution, Watermark And License](#15-attribution-watermark-and-license)

---

## 1. Introduction

PolyRA is a compact, high-performance Relational Database Management System
written in C++17. It began as a minimal relational-algebra interpreter and has
since matured into a multi-model data platform. A single interactive shell now
serves four distinct data paradigms that share one buffer hierarchy, one
catalogue subsystem, and one query lifecycle:

- **Relational tables** operated on with classical relational-algebra operators.
- **Property graphs** (directed or undirected) queried with reachability and
  degree primitives.
- **Deterministic schedules** replayed under a strict concurrency-control
  protocol for transaction analysis.
- **Dense vector matrices** searched with nearest-neighbour similarity queries.

The engine is engineered for environments where main memory is scarce relative
to the working set. Rather than loading whole relations into RAM, PolyRA slices
every dataset into fixed-size blocks and streams them on demand through a custom
buffer manager. Every operator is written to work over cursors, so a query over
a five-million-row relation consumes the same steady-state memory as a query
over five rows.

This edition is maintained under the watermark **`vidvathamaiiith`**, which
appears in the runtime banner, in the header of every source and log file, and
throughout this document.

---

## 2. What Is New In This Edition (vidvathamaiiith)

In addition to the baseline requirements, this version preserves every original capability byte-for-byte in behaviour and
layers on four additive features. None of the pre-existing operators were
altered; each addition is a new, independently dispatched code path.

### 2.1 Polymorphic Catalogue Enumeration - `LIST GRAPHS` and `LIST MATRICES`

The original `LIST TABLES` command has been generalised into a family of
catalogue-introspection commands. `LIST GRAPHS` walks the graph catalogue and
reports each loaded graph together with its orientation; `LIST MATRICES` walks
the vector-matrix catalogue and reports each matrix together with its
dimensionality. The legacy `LIST TABLES` behaviour is unchanged.

### 2.2 Metadata Inspector - `DESCRIBE <relation>`

`DESCRIBE` is a schema and storage-statistics probe that is polymorphic across
all three catalogues. It resolves a name against the table, matrix, and graph
catalogues and prints the bookkeeping the engine already maintains: column
lists, cardinality, block count, rows-per-block, indexing state, persistence
state, matrix dimensionality, or graph node/edge counts. Because it reads only
catalogue metadata, it runs in constant time with respect to the underlying data
volume and never materialises a single row.

### 2.3 Deterministic Content Fingerprint - `CHECKSUM <table>`

`CHECKSUM` streams a table through its block cursor and folds every cell into a
64-bit FNV-1a fingerprint. The computation is fully out-of-core: rows are
consumed one block at a time through the existing buffer manager, so the
fingerprint of an arbitrarily large relation is produced with bounded memory.
The hash is order-sensitive - the row ordinal, column ordinal, and schema arity
are all mixed into the state - so two relations collide only when they are
identical in content and in row order. This makes `CHECKSUM` a cheap integrity
and equality probe: validate a join result against a reference table, confirm an
export round-trip, or detect silent corruption between runs.

### 2.4 Additional Distance Metric - `KNN ... METRIC MANHATTAN`

The K-Nearest-Neighbours operator now accepts a third distance metric,
`MANHATTAN` (the L1 / taxicab distance), alongside the original `COSINE` and
`EUCLIDEAN` metrics. The new metric is wired through the same block-streaming,
bounded-heap search path, so it inherits the engine's memory guarantees and
produces results in the identical `RecordID, Similarity_Score` table format. The
existing metrics are untouched.

A summary of the delta introduced by this edition:

| Feature | Command | Status |
|---|---|---|
| Graph catalogue listing | `LIST GRAPHS` | New |
| Matrix catalogue listing | `LIST MATRICES` | New |
| Relation metadata inspector | `DESCRIBE <relation>` | New |
| Table content fingerprint | `CHECKSUM <table>` | New |
| L1 nearest-neighbour metric | `KNN ... METRIC MANHATTAN` | New |
| All Phase 1 to Phase 4 operators | (unchanged) | Preserved |

---

## 3. Capability Matrix

| Domain | Operators |
|---|---|
| Data loading | `LOAD`, `LOAD GRAPH`, `LOAD VECTOR MATRIX`, `SOURCE` |
| Relational algebra | `SELECT`, `PROJECT`, `RENAME`, `CROSS`, `JOIN`, `DISTINCT`, `SORT`, `GROUP BY` |
| Graph processing | `DEGREE`, `PATH`, `PRINT GRAPH`, `EXPORT GRAPH` |
| Vector search | `KNN` with `COSINE`, `EUCLIDEAN`, `MANHATTAN` metrics |
| Catalogue and inspection | `LIST TABLES`, `LIST GRAPHS`, `LIST MATRICES`, `PRINT`, `DESCRIBE`, `CHECKSUM` |
| Persistence and lifecycle | `EXPORT`, `EXPORT GRAPH`, `CLEAR`, `INDEX` |
| Concurrency | `TRANSACTION` (Wait-Die schedule replay) |
| Session | `QUIT` |

---

## 4. System Architecture

PolyRA is composed of loosely coupled, individually testable subsystems. The
interpreter dispatches each command through three sequential stages, and the
executors interact with the data models exclusively through catalogues and
cursors.

### 4.1 Parsing Front End

- **Syntactic Parser** tokenises the raw command line on whitespace and commas
  using a single regular expression, then validates the shape of the request and
  populates a shared `ParsedQuery` structure. It performs no catalogue lookups.
- **Semantic Parser** validates the request against live catalogue state:
  existence of source relations, existence of referenced columns, non-existence
  of the target relation, and similar structural invariants. Only requests that
  pass both stages reach an executor.

### 4.2 Catalogue Subsystem

Three in-memory registries track everything currently loaded, each keyed by
name:

- `TableCatalogue` for relational tables.
- `GraphCatalogue` for property graphs.
- `MatrixCatalogue` for vector matrices.

The catalogues are the single source of truth about what exists; every executor
resolves names through them. Names may legitimately be shared across catalogues
(for example, a table and a graph may carry the same name), and the `DESCRIBE`
command deliberately reports every model a name resolves to.

### 4.3 Buffer Hierarchy

- `BufferManager` pages relational blocks in and out of a bounded pool using a
  first-in-first-out eviction policy, writing dirty blocks back to the temporary
  spill directory as needed.
- `MatrixBufferManager` is a specialised counterpart for vector pages. It
  enforces a hard ceiling on the number of resident matrix pages, guaranteeing a
  predictable memory envelope during similarity search regardless of matrix
  size, and exposes explicit pin and unpin semantics to the KNN executor.

### 4.4 Data Models

- `Table` owns schema, per-column statistics, block accounting, and row I/O.
- `Graph` represents a graph as a pair of node and edge tables, plus internally
  sorted shadow tables that accelerate traversal while the originals preserve
  CSV order for faithful printing and export.
- `Matrix` represents a dense collection of fixed-dimension float vectors,
  serialised into binary page files.

### 4.5 Executors

Each operator is implemented in its own translation unit under
`src/executor/`. Executors consume input through cursors and emit output either
to standard output or as a newly registered relation, never realising an entire
input dataset in memory.

---

## 5. Execution Pipeline

Every line entered at the prompt flows through the same lifecycle:

```
raw command
    -> tokenise (regex split on whitespace and commas)
    -> syntacticParse()   : shape validation, fills ParsedQuery
    -> semanticParse()    : catalogue and schema validation
    -> executeCommand()   : streaming execution over cursors
    -> result             : printed output and/or a new catalogue entry
```

If either parsing stage fails, a diagnostic (`SYNTAX ERROR` or a specific
`SEMANTIC ERROR: ...`) is reported and execution is skipped.

---

## 6. Repository Layout

```
DMBS_Poly_RA/
|-- include/                 Public headers (mirrors src/ structure)
|   |-- catalog/             Table, graph and matrix catalogue declarations
|   |-- executor/            Executor entry-point declarations
|   |-- models/              Table, graph and matrix model declarations
|   |-- parser/              Syntactic and semantic parser declarations
|   |-- storage/             Buffer manager, cursor and page declarations
|   |-- logger/              Logger declaration
|   `-- global.h             Global externs and shared includes
|-- src/                     Implementation
|   |-- catalog/             Catalogue implementations
|   |-- executor/            One translation unit per operator
|   |   |-- describe.cpp     DESCRIBE            
|   |   |-- checksum.cpp     CHECKSUM            
|   |   |-- list.cpp         LIST TABLES/GRAPHS/MATRICES
|   |   |-- knn.cpp          KNN (COSINE/EUCLIDEAN/MANHATTAN)
|   |   `-- ...              All remaining operators
|   |-- models/              Model implementations
|   |-- parser/              Parser implementations
|   |-- storage/             Storage implementations
|   |-- logger/              Logger implementation
|   `-- main.cpp             Server entry point and REPL
|-- data/                    Sample CSV datasets and scratch spill directory
|-- docs/                    Design notes, phase reports and command references
|-- scripts/                 Data generators and verification helpers
|-- tests/                   Automated test utilities
|-- Grammar.md               Formal grammar of the query language
|-- Makefile                 Top-level build definition
|-- LICENSE                  MIT license and modification notice
`-- README.md                This document
```

---

## 7. Building The Engine

### 7.1 Prerequisites

- A C++17 compiler (`g++` 9 or newer is recommended; this edition is validated
  against `g++` 14.2).
- GNU `make`.
- Approximately 2 GB of available memory for the larger sample workloads.

### 7.2 Standard Build

From the repository root:

```bash
make clean
make
```

The build compiles every translation unit under `src/` and links a single
`server` executable (`server.exe` on Windows). The Makefile automatically adds a
`-Duint="unsigned int"` definition on Windows so the codebase compiles cleanly
under the MinGW-w64 toolchain.

### 7.3 Windows / MSYS2 Toolchain Notes

When building under an MSYS2 UCRT64 toolchain that is invoked from a shell other
than the native MSYS2 shell (for example, Git Bash), two environment conditions
must hold for the linker to succeed:

1. **Writable temporary directory.** The temporary path (`TMP` / `TEMP`) must
   point at a writable location. If it resolves to a protected system directory,
   the compiler cannot stage intermediate files.
2. **Toolchain precedence on `PATH`.** The UCRT64 `bin` directory must precede
   any other MinGW runtime directories so that `ld` loads the runtime libraries
   that match its own build. If a foreign `ld` runtime is resolved first, the
   linker aborts before emitting a diagnostic.

Building from the dedicated MSYS2 UCRT64 shell satisfies both conditions
automatically.

---

## 8. Running The Server

Launch the interpreter and issue commands interactively:

```bash
./server
```

On startup the server prints its watermark banner:

```
============================================================
  PolyRA (vidvathamaiiith edition)
  Multi-Model Relational Algebra Engine
  Watermark: vidvathamaiiith
============================================================
```

Commands can be typed at the `>` prompt or piped in from a file:

```bash
./server < data/my_script.ra
```

Scripts stored as `data/<name>.ra` can also be replayed at runtime with
`SOURCE <name>`. All sample datasets live under `data/`; relations are loaded
from `data/<name>.csv`, query scripts from `data/<name>.ra`, and transient
spill blocks are written under `data/temp/`.

---

## 9. Complete Command Reference

Commands fall into two categories. **Assignment statements** create a new
relation and take the form `<result> <- <operator> ...`. **Non-assignment
statements** perform an action without producing a new relation.

### 9.1 Loading Data

| Command | Description |
|---|---|
| `LOAD <table>` | Loads `data/<table>.csv` into a relational table. |
| `LOAD GRAPH <name> <U\|D>` | Loads a graph from `<name> Nodes <U\|D>.csv` and `<name> Edges <U\|D>.csv`; `U` is undirected, `D` is directed. |
| `LOAD VECTOR MATRIX <name> <dim>` | Loads a headerless CSV of `dim`-dimensional float vectors from `data/<name>.csv`. |
| `SOURCE <script>` | Executes the query script `data/<script>.ra`. |

### 9.2 Catalogue And Inspection

| Command | Description |
|---|---|
| `LIST TABLES` | Lists every loaded or derived table. |
| `LIST GRAPHS` | Lists every loaded graph with its orientation. **(vidvathamaiiith)** |
| `LIST MATRICES` | Lists every loaded vector matrix with its dimensionality. **(vidvathamaiiith)** |
| `PRINT <table>` | Prints the first `PRINT_COUNT` rows of a table. |
| `PRINT GRAPH <graph>` | Prints the node and edge sets of a graph. |
| `DESCRIBE <relation>` | Reports schema and storage statistics for a table, matrix, or graph. **(vidvathamaiiith)** |
| `CHECKSUM <table>` | Reports a deterministic 64-bit content fingerprint of a table. **(vidvathamaiiith)** |

### 9.3 Relational Algebra

| Command | Description |
|---|---|
| `<R> <- SELECT <col> <op> <col\|int> FROM <table>` | Selection; `<op>` is one of `< > == != <= >= => =<`. |
| `<R> <- PROJECT <col>(, <col>)* FROM <table>` | Projection onto the listed columns. |
| `RENAME <fromCol> TO <toCol> FROM <table>` | Renames a column of an existing table in place. |
| `<R> <- CROSS <table1> <table2>` | Cartesian product; result columns are prefixed to disambiguate the two operands. |
| `<R> <- JOIN <t1>, <t2> ON <c1> == <c2>` | Equality join; an arithmetic form `ON <c1> <+\|-> <c2> == <const>` and optional `WHERE` / `PROJECT` clauses are also accepted. |
| `<R> <- DISTINCT <table>` | Duplicate-elimination projection over the source relation. |
| `SORT <table> BY <col> [<col> ...] IN <ASC\|DESC> [...] [TOP <n>] [BOTTOM <n>]` | In-place external merge sort supporting multiple sort keys and optional `TOP` / `BOTTOM` row limits. |
| `<r1> [<r2> ...] <- GROUP BY <c1> [<c2> ...] FROM <table> HAVING <AGG>(<c>) <op> <const\|AGG(c)> RETURN <AGG>(<c>) [...]` | Grouped aggregation with a mandatory `HAVING` predicate; `MAX`, `MIN`, `SUM`, `AVG`, and `COUNT` are supported, and the result-name, group-key, and return-aggregate lists must be of equal length. |

### 9.4 Graph Processing

| Command | Description |
|---|---|
| `DEGREE <graph> <nodeID>` | Reports the degree of a node, or `Node does not exist`. |
| `<R> <- PATH <graph> <src> <dest> [WHERE <conditions>]` | Tests reachability via breadth-first search over disk-backed edge blocks, with optional inline node/edge conditions. |
| `PRINT GRAPH <graph>` | Prints the graph. |
| `EXPORT GRAPH <graph>` | Writes the graph back to CSV. |

### 9.5 Vector Search

```
<R> <- KNN <matrix> QUERY_VEC [ f1, f2, ... ] TOP <X> METRIC <COSINE|EUCLIDEAN|MANHATTAN>
```

Performs a K-Nearest-Neighbours search over a loaded matrix. `QUERY_VEC` is the
probe vector, `TOP <X>` is the number of neighbours to return, and `METRIC`
selects the distance function. `MANHATTAN` is a vidvathamaiiith extension. The
result is a two-column table, `RecordID, Similarity_Score`, ordered from nearest
to farthest, where the score is the distance scaled by 10000 and cast to an
integer (smaller is closer).

### 9.6 Persistence And Lifecycle

| Command | Description |
|---|---|
| `EXPORT <table>` | Persists an in-memory table to `data/<table>.csv`. |
| `CLEAR <table>` | Removes a table and deletes its temporary blocks. |
| `INDEX ON <col> FROM <table> USING <BTREE\|HASH\|NOTHING>` | Declares an indexing strategy on a column. |

### 9.7 Concurrency

```
TRANSACTION <schedule_file>
```

Replays a deterministic operation schedule from `data/<schedule_file>` under the
Wait-Die protocol and writes an annotated trace to
`data/<schedule_file>_output.txt`. Each line of the schedule is one of
`BEGIN <T>`, `READ <T> <relation> <page>`, `WRITE <T> <relation> <page>`, or
`COMMIT <T>`.

### 9.8 Session

`QUIT` terminates the server.

---

## 10. Mathematical Formulation

### 10.1 Relational Operators

- Projection: pi over the requested attribute set, evaluated block by block.
- Selection: sigma over a binary predicate, evaluated per row.
- Join and Cross: nested-loop evaluation with block-level buffering.
- Sort and Group: external merge sort bounds resident memory; grouping folds
  aggregates incrementally.

### 10.2 Distance Metrics For KNN

For a query vector `A` and a candidate vector `B` of dimension `n`:

- Euclidean (L2):
  `d(A, B) = sqrt( sum_i (A_i - B_i)^2 )`
- Cosine distance:
  `d(A, B) = 1 - ( sum_i A_i B_i ) / ( sqrt(sum_i A_i^2) * sqrt(sum_i B_i^2) )`
- Manhattan (L1, vidvathamaiiith extension):
  `d(A, B) = sum_i | A_i - B_i |`

All three are minimised for the closest vector, so a single bounded max-heap of
size `X` selects the top neighbours uniformly across metrics.

### 10.3 CHECKSUM Fingerprint (vidvathamaiiith)

`CHECKSUM` computes a 64-bit FNV-1a hash. Starting from the FNV offset basis, the
state is seeded with the column count and then, for each row in cursor order and
each cell in column order, the row ordinal, the column ordinal, and the cell
value are folded byte by byte:

```
state <- FNV_OFFSET_BASIS
fold(column_count)
for each row r (ordinal i):
    fold(i)
    for each cell c (ordinal j):
        fold(j)
        fold(value)

fold(x): for each byte b of x: state <- (state XOR b) * FNV_PRIME
```

Mixing the ordinals makes the fingerprint sensitive to both content and row
order, so it doubles as an equality test between relations.

### 10.4 Wait-Die Concurrency

Transactions are timestamped at `BEGIN`. When transaction `T` requests a lock
held in a conflicting mode by transaction `H`:

- if `T` is older than every holder, `T` waits;
- otherwise `T` is aborted, rolled back, and later restarted with its original
  timestamp.

Because a waiting transaction is always older than the transaction it waits on,
no cycle of waits can form, which guarantees deadlock freedom.

---

## 11. Storage And Memory Model

Datasets are never assumed to fit in memory. On load, a relation is split into
fixed-size blocks written to the spill directory; the buffer manager keeps only
a bounded number resident and evicts under first-in-first-out order. Operators
read through cursors that advance block by block, so steady-state memory is a
function of the buffer pool size rather than the dataset size.

Vector matrices are handled by the dedicated matrix buffer manager, which caps
the number of resident vector pages independently of the relational pool. During
similarity search each page is explicitly pinned while it is scanned and unpinned
immediately afterward, so a search over a very large matrix proceeds within a
fixed memory envelope.

---

## 12. Concurrency Control

The `TRANSACTION` command drives a lock simulator that models shared and
exclusive locks at the granularity of `(relation, page)`. It maintains a lock
table, a per-transaction set of held pages, and a queue of blocked requests. The
simulator applies the Wait-Die rule described above, logs every lock request,
grant, wait, death, rollback, and commit, and deterministically restarts aborted
transactions in timestamp order. The complete annotated trace is written to disk
for inspection and comparison against expected output.

---

## 13. Design Constraints

- **Integer-native relations.** Relational cell data is represented as 32-bit
  integers, which keeps rows fixed-width, simplifies block arithmetic, and
  improves cache behaviour. Vector elements are parsed as floats for distance
  computation and their scores are cast to integers so that KNN results compose
  cleanly with the relational operators.
- **Bounded memory.** Both buffer managers operate under explicit ceilings;
  correctness never depends on a dataset fitting in memory.
- **Deterministic output.** Given the same input, every operator - including the
  transaction replayer and the checksum fingerprint - produces identical output
  across runs, which makes automated verification straightforward.

---

## 14. Testing And Verification

The `data/` directory ships with a range of sample workloads, from small
hand-checkable relations and graphs to stress datasets with up to one million
rows, along with reference transaction schedules and their expected traces. The
`scripts/` and `tests/` directories provide data generators and verification
helpers.

A representative manual smoke test of this edition:

```
LOAD student
DESCRIBE student
CHECKSUM student
LOAD GRAPH G1 D
DEGREE G1 1
LOAD VECTOR MATRIX vidva_vectors 4
R_MAN <- KNN vidva_vectors QUERY_VEC [ 1.0, 0.0, 0.0, 0.0 ] TOP 3 METRIC MANHATTAN
PRINT R_MAN
LIST TABLES
LIST GRAPHS
LIST MATRICES
QUIT
```

`DESCRIBE` reports the relation's schema and block statistics, `CHECKSUM`
returns a stable hexadecimal fingerprint, and the three `LIST` variants
enumerate their respective catalogues.

---

## 15. Attribution, Watermark And License

This project is maintained under the watermark **`vidvathamaiiith`**. The
watermark is embedded in the runtime startup banner, at the head of every source
and header file, at the top of every generated log file, and in the project
metadata.

PolyRA is distributed under the MIT License. The multi-model architecture and
the `DESCRIBE`, `CHECKSUM`, extended `LIST`, and Manhattan-metric KNN
functionality introduced in this edition are released under the same permissive
terms. See [LICENSE](LICENSE) for the complete text.
