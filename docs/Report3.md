# Phase 3 Report

## 1. Concurrency Control Technique Chosen

### Selected Technique
Strict Two-Phase Locking (Strict 2PL) with Wait-Die deadlock prevention.

### Reasoning for Choosing This Technique
- It fits naturally into the existing page-access flow (cursor and buffer manager) because lock checks can be done immediately before each page read/write.
- Wait-Die avoids deadlock detection overhead and keeps implementation focused on ordering decisions at conflict time.
- The sample test-case document contains locking output traces, which are directly aligned with this technique and made verification straightforward.

## 2. Detailed Implementation

### Command Integration
Added a new command path for:
TRANSACTION <input_file>

Integrated through existing parser -> semantic -> executor flow:
- syntactic parser dispatch and parsed field for input file
- semantic parser dispatch and input-file existence validation
- executor dispatch to transaction handler

### Core Module
A dedicated transaction executor implements schedule simulation at page granularity.

Implemented features:
- Schedule parsing for BEGIN, READ, WRITE, COMMIT.
- Per-transaction state:
  - timestamp
  - active/waiting/aborted/committed status
  - deferred commit state
  - replay program for restart
- Per-page lock table:
  - shared owners
  - exclusive owner
- Conflict handling using Wait-Die:
  - older transaction waits
  - younger transaction aborts and rolls back
- Strictness:
  - locks are released only on COMMIT/ABORT
- Restart handling:
  - aborted transactions restart with fresh timestamp
- Output generation in data directory with required naming:
  - inputFileName_output.txt

### Page-Flow Preservation
All read/write operations preserve existing engine flow:
- READ touches pages via Cursor.
- WRITE touches pages via BufferManager.

### Error Handling Implemented
- SEMANTIC ERROR: Relation doesn't exist
- SEMANTIC ERROR: Page doesn't exist
- SEMANTIC ERROR: File doesn't exist

## 3. Verification

### 3.1 Sample Test Case Verification
Verified using the provided sample schedules from the sample test-case PDF:
- data/phase3_sample1.txt -> data/phase3_sample1_output.txt
- data/phase3_sample2.txt -> data/phase3_sample2_output.txt

Observed behavior matches expected locking semantics:
- lock acquisition ordering
- wait-die decisions
- abort/rollback behavior
- restart behavior
- strict lock release on commit/abort

Unlock order was made deterministic by preserving lock-acquisition order per transaction.

Line-by-line comparison against the locking output in the sample PDF:
- Sample 1: 30/30 lines exact match.
- Sample 2: 41/41 lines exact match.

### 3.2 Mandatory Error Checks
Verified on dedicated schedules:
- data/phase3_error_relation.txt -> data/phase3_error_relation_output.txt
  - produces: SEMANTIC ERROR: Relation doesn't exist
- data/phase3_error_page.txt -> data/phase3_error_page_output.txt
  - produces: SEMANTIC ERROR: Page doesn't exist

### 3.3 Output Naming Rule Check
Verified output files are created as:
inputFileName_output.txt
Examples:
- phase3_sample1.txt -> phase3_sample1_output.txt
- phase3_namecheck.txt -> phase3_namecheck_output.txt

## Assumptions

- Only one protocol is required by Phase 3 (either Strict 2PL + Wait-Die or Strict Timestamp Ordering). This implementation covers the locking option fully.
- Schedule lines are whitespace-tokenized and expected in one of: BEGIN/READ/WRITE/COMMIT formats.
- If an operation refers to an invalid table/page, the specified semantic error is logged and the schedule processing continues.
- Transaction restart ordering is deterministic using original timestamp priority among aborted transactions.
- Existing non-transaction query behavior is preserved.

## Contribution

- Team Member 1:
  - Parser/semantic/executor integration for TRANSACTION command.
  - Input-file validation and command dispatch wiring.

- Team Member 2:
  - Lock manager simulation logic (Strict 2PL + Wait-Die).
  - Restart/rollback handling and output generation.
  - Sample-case and error-case verification.
