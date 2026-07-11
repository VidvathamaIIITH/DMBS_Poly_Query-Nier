# Phase 2 Report

## 1. SETBUFFER K

### Syntax Implemented
`SETBUFFER K`

### Parsing and Validation
- Added a dedicated parser path for `SETBUFFER`.
- Validates query length and numeric argument format.
- Semantic validation enforces `2 <= K <= 10`.

### Execution Logic
- Updates global `BLOCK_COUNT` for the current session.
- Already loaded pages in memory are left unchanged; future page accesses follow the updated buffer budget.

### Detailed Command Flow
- Step 1: Syntactic parser checks token count is exactly 2 (`SETBUFFER` and `K`).
- Step 2: `K` is matched against numeric regex; non-numeric input is rejected as syntax error.
- Step 3: Semantic parser checks range `2 <= K <= 10`.
- Step 4: Executor writes `K` into global `BLOCK_COUNT`.
- Step 5: No data pages are rewritten; the value only controls later memory behavior in `SORT`, `JOIN`, and `GROUP BY`.

### Error Handling
- Invalid syntax prints `SYNTAX ERROR`.
- Out-of-range values print `SEMANTIC ERROR`.

### Block Access Notes
- No data-file access is performed directly by this command.
- This command affects all later block usage by updating available in-memory block slots.

### Example
- Input: `SETBUFFER 6`
- Effect: Buffer budget for future block operations becomes 6 for this session.
- Output: No success message is printed.

## 2. SORT

### Syntax Implemented
`SORT <table-name> BY <col1>, <col2>, ... IN <ASC|DESC>, <ASC|DESC>, ... [TOP X] [BOTTOM Y]`

### Parsing and Validation
- Supports any number of sort columns and matching sort directions.
- Supports optional `TOP X` and optional `BOTTOM Y` in any order.
- Validates table and all sort columns exist.
- Validates row bounds (`X`, `Y`) against table row count and disallows overlap.

### Execution Logic
- In-place table sorting on temp pages (original CSV stays unchanged until export).
- Uses external sorting pipeline:
  - initial run generation,
  - multi-pass merge with fan-in based on `BLOCK_COUNT`.
- Stable tie behavior maintained by:
  - stable in-memory run sorting,
  - deterministic merge tie-breaking using source run order.
- `TOP/BOTTOM` behavior:
  - `TOP X` rows sorted independently,
  - `BOTTOM Y` rows sorted independently,
  - middle rows preserved in original order.

### Detailed Command Flow
- Step 1: Parse sort keys and sort directions, and optional `TOP`/`BOTTOM` bounds.
- Step 2: Validate table existence, column existence, and bounds consistency.
- Step 3: Compute the active row segment to sort:
  - full table if no bounds,
  - top prefix for `TOP`,
  - bottom suffix for `BOTTOM`,
  - two disjoint segments for `TOP ... BOTTOM ...`.
- Step 4: Create sorted runs using available in-memory blocks.
- Step 5: Merge runs repeatedly with fan-in `BLOCK_COUNT - 1` until one final run remains.
- Step 6: Rewrite only the targeted rows in table temp pages while preserving untouched rows.
- Step 7: Update table metadata/page mapping for consistent cursor reads.

### Error Handling
- Missing table: `SEMANTIC ERROR: Relation doesn't exist`
- Missing sort column: `SEMANTIC ERROR: Column doesn't exist in relation`

### Block Access Notes
- Run generation/merge reads and writes through `Cursor`, `Page`, and `BufferManager`.
- Intermediate runs are persisted as temp pages; metadata tracked for page-safe readback.

### Example
- Input: `SORT EMPLOYEE BY Salary IN DESC TOP 3`
- Effect: Only the first 3 rows are sorted by `Salary` descending; remaining rows stay in original order.
- Output: Silent on success; use `PRINT EMPLOYEE` to inspect result.

## 3. HASH JOIN

### Syntax Implemented
`Result <- JOIN <table1>, <table2> ON <join-condition> [WHERE <selection-condition>] [PROJECT <attribute-list>];`

### Supported Join Conditions
- Attribute equality: `A.a == B.b`
- Arithmetic form: `A.a + B.b == number` and `A.a - B.b == number`

### Parsing and Validation
- Parses optional single `WHERE` clause (`A.a op number`).
- Parses optional `PROJECT` list preserving output column order.
- Validates table existence and qualified column tokens.

### Execution Logic
- Implements partition-hash join flow:
  - partition both relations into `BLOCK_COUNT - 1` partitions,
  - build hash table on left partition,
  - probe with right partition.
- Supports both condition types with residual verification.
- Applies optional `WHERE` post join-condition check.
- Applies optional projection; otherwise outputs all columns from table1 then table2.
- If no rows satisfy conditions, resultant table is not created.

### Detailed Command Flow
- Step 1: Parse base join expression and optional `WHERE` and `PROJECT` clauses.
- Step 2: Validate relation names and qualified attributes (`Relation.Column`).
- Step 3: Create `BLOCK_COUNT - 1` partitions for each input relation using hash partitioning.
- Step 4: For each partition pair, load left partition rows into in-memory hash buckets.
- Step 5: Scan right partition rows and probe matching candidates.
- Step 6: Recheck full join predicate (including arithmetic forms) before emitting row.
- Step 7: Apply `WHERE` filter if present.
- Step 8: Apply `PROJECT` column selection if present, then write to output blocks.
- Step 9: Register result relation only if at least one output row exists.

### Error Handling
- Missing table: `SEMANTIC ERROR: Relation doesn't exist`
- Missing column: `SEMANTIC ERROR: Column doesn't exist in relation`

### Block Access Notes
- Uses temp-page partition files and block-based readers.
- Partition metadata is explicitly registered for safe page-level access.

### Example
- Input: `R <- JOIN EMPLOYEE, DEPARTMENT ON EMPLOYEE.Dno == DEPARTMENT.Dnumber PROJECT EMPLOYEE.Ssn, DEPARTMENT.Dname;`
- Effect: Produces relation `R` with only projected columns for matching join rows.
- Output: Silent on success; use `PRINT R` to verify.

## 4. GROUP BY

### Syntax Implemented
`Result1, Result2 <- GROUP BY <group-attr1>, <group-attr2> FROM <table-name> HAVING <aggregate-expression> RETURN <return-aggregate1>, <return-aggregate2>;`

### Parsing and Validation
- Supports multiple result relations and multiple grouping attributes.
- Supports HAVING forms:
  - aggregate vs constant,
  - aggregate vs aggregate.
- Supports aggregate functions: `MAX`, `MIN`, `COUNT`, `SUM`, `AVG`.
- Validates:
  - source table exists,
  - each group-by column exists,
  - HAVING aggregate columns exist,
  - RETURN aggregate columns exist,
  - result names are not pre-existing tables.

### Execution Logic
- Processes each grouping attribute independently.
- Externally sorts the source table on the current grouping attribute using temp pages.
- Streams the sorted rows and aggregates one group at a time, so only the current group's aggregate state is kept in memory.
- Produces one result table per grouping attribute with exactly two columns:
  - group key,
  - corresponding return aggregate.
- Evaluates HAVING per group before output.
- Return column naming:
  - `COUNT(*)` -> `COUNT`
  - otherwise `<AGG><Column>` (e.g., `MAXSalary`).
- If no groups qualify for a result relation, that table is not created.

### Detailed Command Flow
- Step 1: Parse result relation names, grouping attributes, source relation, `HAVING`, and `RETURN` aggregates.
- Step 2: Validate all referenced columns and ensure destination relation names are free.
- Step 3: For each grouping attribute, perform external sort on that attribute.
- Step 4: Stream sorted rows and detect group boundaries by key change.
- Step 5: Maintain aggregate state per current group (`SUM`, `COUNT`, `MIN`, `MAX`, and derived `AVG`).
- Step 6: Evaluate `HAVING` when a group closes.
- Step 7: If `HAVING` passes, emit row `<group-key, return-aggregate>`.
- Step 8: Finalize output relation only if at least one row is emitted.

### Error Handling
- Missing table: `SEMANTIC ERROR: Relation doesn't exist`
- Missing group-by column: `SEMANTIC ERROR: Group By column doesn't exist in relation`
- Missing having column: `SEMANTIC ERROR: Having column doesn't exist in relation`
- Missing return column: `SEMANTIC ERROR: Return column doesn't exist in relation`

### Block Access Notes
- Grouping uses temp-page external sort plus a streaming scan over the sorted run.
- Result writing is page-based via standard table blockification.

### Example
- Input: `R1 <- GROUP BY Dno FROM EMPLOYEE HAVING COUNT(*) > 1 RETURN AVG(Salary);`
- Effect: Creates one row per `Dno` group with average salary for groups meeting `COUNT(*) > 1`.
- Output: Silent on success; use `PRINT R1` to inspect rows.

## Assumptions
- Tokenization rules follow existing engine behavior (comma-separated tokens are already split by regex delimiter).
- Aggregate values are stored and emitted as integer results; `AVG` is rounded up.
- Qualified attribute format for JOIN clauses is strictly `Relation.Column`.
- When output relation has zero rows (JOIN/GROUP result), relation is not inserted into catalogue.
- Existing graph-related functionality from Phase 1 remains unchanged in this phase report.

## Contribution
- Team Member 1: Parser and semantic validation for `SETBUFFER`, `SORT`, `JOIN`, and `GROUP BY`, plus executor dispatch integration.
- Team Member 2: Execution pipelines for `SORT`, `JOIN`, and `GROUP BY`, runtime testing on provided datasets, and detailed report write-up.
