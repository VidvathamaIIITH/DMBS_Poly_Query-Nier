# Phase 2 Command Reference

This document is a practical reference for the implemented Phase 2 commands:

- `SETBUFFER K`
- `SORT ...`
- `Result <- JOIN ...`
- `Result1, Result2, ... <- GROUP BY ...`

All example outputs below were verified against the current implementation by running the commands on the existing sample CSV files in `data/`.

## General Notes

- Phase 2 commands operate on tables, not graphs.
- `LOAD` must be run before any command that uses a table.
- `SETBUFFER`, `SORT`, `JOIN`, and `GROUP BY` are silent on success. To inspect the result of `SORT`, `JOIN`, or `GROUP BY`, run `PRINT` on the affected table afterward.
- `SORT` modifies only temp pages. The original CSV in `data/` is unchanged unless `EXPORT` is run later.
- `JOIN` and `GROUP BY` do not create a result table if the result is empty.
- The implementation accepts the trailing semicolon shown in the spec for `JOIN`; using it is safe.
- `AVG` is rounded up.

## 1. SETBUFFER K

### Purpose

Sets the number of buffers available for subsequent commands in the current session.

### Syntax

```text
SETBUFFER K
```

### Valid Range

```text
2 <= K <= 10
```

### Behavior

- Default buffer count at session start is `10`.
- The command may be issued multiple times during a session.
- Already loaded pages remain in memory; future accesses follow the new buffer budget.
- The command itself does not print anything when successful.

### Success Example

#### Input

```text
LOAD EMPLOYEE
SETBUFFER 6
PRINT EMPLOYEE
QUIT
```

#### Output

```text
Loaded Table. Column Count: 6 Row Count: 8

Ssn, Bdate, Sex, Salary, Super_ssn, Dno
1, 551208, 1, 25000, 7, 4
2, 650109, 1, 25000, 6, 5
3, 690329, 0, 25000, 7, 4
4, 271110, 0, 30000, 6, 5
5, 620915, 0, 38000, 6, 5
6, 551208, 0, 40000, 8, 5
7, 551208, 1, 43000, 8, 4
8, 720731, 0, 55000, 0, 1

Row Count: 8
```

Note: There is no output line for `SETBUFFER 6`. The table prints normally afterward.

### Failure Example

#### Input

```text
SETBUFFER 1
QUIT
```

#### Output

```text
SEMANTIC ERROR
```

## 2. SORT

### Purpose

Sorts a loaded table in place using external sorting on temp pages.

### Syntax

```text
SORT <table-name> BY <col1>, <col2>, ... IN <ASC|DESC>, <ASC|DESC>, ... [TOP X] [BOTTOM Y]
```

### Supported Variants

- Full-table sort
- Multi-key sort
- `TOP X`
- `BOTTOM Y`
- `TOP X BOTTOM Y`

### Key Rules

- Sort keys are prioritized left to right.
- If two rows tie on all sort keys, original order is preserved.
- `TOP X` sorts only the first `X` rows.
- `BOTTOM Y` sorts only the last `Y` rows.
- If both are present, the top and bottom ranges are sorted independently.
- Rows outside those ranges remain in original order.

### Variant A: Full-table Sort

#### Input

```text
LOAD EMPLOYEE
SORT EMPLOYEE BY Salary IN DESC
PRINT EMPLOYEE
QUIT
```

#### Output

```text
Loaded Table. Column Count: 6 Row Count: 8

Ssn, Bdate, Sex, Salary, Super_ssn, Dno
8, 720731, 0, 55000, 0, 1
7, 551208, 1, 43000, 8, 4
6, 551208, 0, 40000, 8, 5
5, 620915, 0, 38000, 6, 5
4, 271110, 0, 30000, 6, 5
1, 551208, 1, 25000, 7, 4
2, 650109, 1, 25000, 6, 5
3, 690329, 0, 25000, 7, 4

Row Count: 8
```

### Variant B: `TOP` and `BOTTOM` Together

#### Input

```text
LOAD A
SORT A BY b IN DESC TOP 3 BOTTOM 2
PRINT A
QUIT
```

#### Output

```text
Loaded Table. Column Count: 5 Row Count: 10

a, b, c, d, e
3, 13, 23, 33, 43
2, 12, 22, 32, 42
1, 11, 21, 31, 41
4, 14, 24, 34, 44
5, 15, 25, 35, 45
6, 16, 26, 36, 46
7, 17, 27, 37, 47
8, 18, 28, 38, 48
10, 20, 30, 40, 50
9, 19, 29, 39, 49

Row Count: 10
```

Explanation:

- Top 3 rows are sorted by `b` in descending order.
- Bottom 2 rows are sorted independently by `b` in descending order.
- Middle rows remain unchanged.

### Failure Example

#### Input

```text
LOAD EMPLOYEE
SORT EMPLOYEE BY MissingCol IN ASC
QUIT
```

#### Output

```text
Loaded Table. Column Count: 6 Row Count: 8

SEMANTIC ERROR: Column doesn't exist in relation
```

## 3. HASH JOIN

### Purpose

Joins two loaded tables using partition-hash join.

### Syntax

```text
Result <- JOIN <table1>, <table2> ON <join-condition> [WHERE <selection-condition>] [PROJECT <attribute-list>];
```

### Supported Join Conditions

#### Equality join

```text
A.a == B.b
```

#### Arithmetic join

```text
A.a + B.b == number
A.a - B.b == number
```

### Supported Optional Clauses

#### WHERE

```text
WHERE A.a op number
```

Where `op` is one of:

```text
==, !=, >, >=, <, <=
```

#### PROJECT

- Keeps only the listed columns.
- Output column order matches the `PROJECT` clause order.
- Without `PROJECT`, all columns from the first table are output first, followed by all columns from the second table.

### Variant A: Equality Join with `WHERE` and `PROJECT`

#### Input

```text
LOAD EMPLOYEE
LOAD DEPARTMENT
R <- JOIN EMPLOYEE, DEPARTMENT ON EMPLOYEE.Dno == DEPARTMENT.Dnumber WHERE EMPLOYEE.Salary > 30000 PROJECT EMPLOYEE.Ssn, EMPLOYEE.Salary, DEPARTMENT.Mgr_ssn;
PRINT R
QUIT
```

#### Output

```text
Loaded Table. Column Count: 6 Row Count: 8
Loaded Table. Column Count: 3 Row Count: 3

Ssn, Salary, Mgr_ssn
8, 55000, 8
7, 43000, 7
6, 40000, 6
5, 38000, 6

Row Count: 4
```

### Variant B: Arithmetic Join with `PROJECT`

#### Input

```text
LOAD A
LOAD B
J <- JOIN A, B ON A.a + B.b == 5 PROJECT A.a, A.c, B.b;
PRINT J
QUIT
```

#### Output

```text
Loaded Table. Column Count: 5 Row Count: 10
Loaded Table. Column Count: 2 Row Count: 4

a, c, b
1, 21, 4
2, 22, 3
3, 23, 2
4, 24, 1

Row Count: 4
```

### Variant C: Empty-result Join

#### Input

```text
LOAD A
LOAD B
E <- JOIN A, B ON A.a + B.b == 100 PROJECT A.a;
PRINT E
QUIT
```

#### Output

```text
Loaded Table. Column Count: 5 Row Count: 10
Loaded Table. Column Count: 2 Row Count: 4

SEMANTIC ERROR: Relation doesn't exist
```

Explanation:

- The join produces no rows.
- Therefore table `E` is not created.
- `PRINT E` then fails because the relation does not exist.

### Failure Example

#### Input

```text
LOAD EMPLOYEE
LOAD DEPARTMENT
R <- JOIN EMPLOYEE, DEPARTMENT ON EMPLOYEE.BadCol == DEPARTMENT.Dnumber;
QUIT
```

#### Output

```text
Loaded Table. Column Count: 6 Row Count: 8
Loaded Table. Column Count: 3 Row Count: 3

SEMANTIC ERROR: Column doesn't exist in relation
```

## 4. GROUP BY

### Purpose

Produces one or more grouped result tables using a `HAVING` condition and corresponding `RETURN` aggregates.

### Syntax

```text
Result1, Result2, ... <- GROUP BY <group-attr1>, <group-attr2>, ... FROM <table-name> HAVING <aggregate-expression> RETURN <return-aggregate1>, <return-aggregate2>, ...
```

### Supported `HAVING` Forms

#### Aggregate vs constant

```text
AVG(Salary) > 30000
COUNT(*) == 2
```

#### Aggregate vs aggregate

```text
AVG(Salary) > AVG(Expenses)
MAX(A) <= MIN(B)
```

### Supported Aggregates

```text
MAX, MIN, COUNT, SUM, AVG
```

### Mapping Rule

If the command is:

```text
R1, R2 <- GROUP BY A, B FROM T HAVING ... RETURN COUNT(C), SUM(D)
```

Then:

- `R1` groups by `A` and returns `COUNT(C)`
- `R2` groups by `B` and returns `SUM(D)`

Each result table has exactly two columns:

- the grouping attribute
- the corresponding return aggregate

### Return Column Naming

- `RETURN MAX(Salary)` creates column `MAXSalary`
- `RETURN AVG(Salary)` creates column `AVGSalary`
- `RETURN COUNT(*)` creates column `COUNT`

### Variant A: Single Result Table, Constant in `HAVING`

#### Input

```text
LOAD EMPLOYEE
G <- GROUP BY Dno FROM EMPLOYEE HAVING AVG(Salary) > 30000 RETURN COUNT(*)
PRINT G
QUIT
```

#### Output

```text
Loaded Table. Column Count: 6 Row Count: 8

Dno, COUNT
1, 1
4, 3
5, 4

Row Count: 3
```

### Variant B: Multiple Result Tables, Aggregate-vs-aggregate `HAVING`

#### Input

```text
LOAD EMPLOYEE
G1, G2 <- GROUP BY Dno, Sex FROM EMPLOYEE HAVING AVG(Salary) > AVG(Super_ssn) RETURN MAX(Salary), COUNT(*)
PRINT G1
PRINT G2
QUIT
```

#### Output

```text
Loaded Table. Column Count: 6 Row Count: 8

Dno, MAXSalary
1, 55000
4, 43000
5, 40000

Row Count: 3

Sex, COUNT
0, 5
1, 3

Row Count: 2
```

### Failure Example

#### Input

```text
LOAD EMPLOYEE
G <- GROUP BY BadCol FROM EMPLOYEE HAVING COUNT(*) > 0 RETURN COUNT(*)
QUIT
```

#### Output

```text
Loaded Table. Column Count: 6 Row Count: 8

SEMANTIC ERROR: Group By column doesn't exist in relation
```

## Exact Error Strings

For automated testing, the exact strings below matter.

### SETBUFFER

```text
SEMANTIC ERROR
```

### SORT

```text
SEMANTIC ERROR: Relation doesn't exist
SEMANTIC ERROR: Column doesn't exist in relation
```

### JOIN

```text
SEMANTIC ERROR: Relation doesn't exist
SEMANTIC ERROR: Column doesn't exist in relation
```

### GROUP BY

```text
SEMANTIC ERROR: Relation doesn't exist
SEMANTIC ERROR: Group By column doesn't exist in relation
SEMANTIC ERROR: Having column doesn't exist in relation
SEMANTIC ERROR: Return column doesn't exist in relation
```

## Automated Testing Notes

These points are important if an automated script is going to evaluate the implementation.

### Session discipline

- Prefer running each independent test in a fresh `./server` session.
- Reload all required tables at the beginning of each test.
- This avoids false failures caused by leftover result tables from previous tests.

### Silent-success commands

- `SETBUFFER` does not print success output.
- `SORT` does not print success output.
- `JOIN` does not print success output.
- `GROUP BY` does not print success output.

So automated tests should normally use `PRINT` after the command if table contents must be checked.

### Empty-result behavior

- Empty `JOIN` result: output table is not created.
- Empty `GROUP BY` result for a particular target table: that target table is not created.

### Result content checks

- `SORT` should be validated by printing the sorted table.
- `JOIN` should be validated by printing the result table.
- `GROUP BY` should be validated by printing each produced result table.

### Important semantic checks

- `AVG` uses round-up behavior.
- `TOP X` and `BOTTOM Y` are sorted independently.
- Rows outside `TOP/BOTTOM` ranges remain in original order.
- `COUNT(*)` output column name must be exactly `COUNT`.

## Suggested Test Buckets

Before submission, run at least one query from each bucket below.

- `SETBUFFER` valid value
- `SETBUFFER` invalid value
- `SORT` full-table single-key sort
- `SORT` multi-key sort
- `SORT` with `TOP`
- `SORT` with `BOTTOM`
- `SORT` with `TOP` and `BOTTOM` together
- `SORT` invalid table / invalid column
- `JOIN` equality join
- `JOIN` arithmetic join with `+`
- `JOIN` arithmetic join with `-`
- `JOIN` with `WHERE`
- `JOIN` with `PROJECT`
- `JOIN` empty result
- `JOIN` invalid column
- `GROUP BY` single result table
- `GROUP BY` multiple result tables
- `GROUP BY` `HAVING` aggregate vs constant
- `GROUP BY` `HAVING` aggregate vs aggregate
- `GROUP BY` `RETURN COUNT(*)`
- `GROUP BY` invalid group-by column
- `GROUP BY` invalid having column
- `GROUP BY` invalid return column