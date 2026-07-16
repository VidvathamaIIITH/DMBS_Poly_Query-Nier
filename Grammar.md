# Relational Algebra Grammar

## Grammar

```
Statement -> relation_name <- assignment_statement
           | non_assignment_statement

assignment_statement -> cross_product_statement
                      | distinct_statement
                      | join_statement
                      | projection_statement
                      | selection_statement
                      | sort_statement
                       
non_assignment_statement -> clear_statement 
                           | index_statement
                           | list_statement
                           | load_statement
                           | print_statement
                           | quit_statement
                           | rename_statement
                           | source_statement
                           | describe_statement   /* vidvathamaiiith extension */
                           | checksum_statement   /* vidvathamaiiith extension */

cross_product_statement -> CROSS relation_name relation_name

distinct_statement -> DISTINCT relation_name

join_statement -> JOIN relation_name, relation_name ON column_name bin_op column_name

projection_statement -> PROJECT projection_list FROM relation_name

projection_list -> projection_list, column_name 
                 | column_name

selection_statement -> SELECT condition FROM relation_name

condition -> column_name binop column_name 
           | column_name binop int_literal

binop -> > | < | == | != | <= | >= | => | =< 

sort_statement -> SORT relation_name BY column_name IN sorting_order

sorting_order -> ASC | DESC

clear_statement -> CLEAR relation_name

index_statement -> INDEX ON column_name FROM relation_name USING indexing_strategy

indexing_strategy -> HASH | BTREE | NOTHING;

list_statement -> LIST list_target

list_target -> TABLES | GRAPHS | MATRICES   /* GRAPHS, MATRICES are extensions */

load_statement -> LOAD relation_name

print_statement -> PRINT relation_name

quit_statement -> QUIT

rename_statement -> RENAME column_name TO column_name FROM relation_name

source_statement -> SOURCE file_name

/* ---- vidvathamaiiith  ---- */

describe_statement -> DESCRIBE relation_name
/* relation_name may resolve to a table, a vector matrix, or a graph */

checksum_statement -> CHECKSUM relation_name
/* relation_name must resolve to a table */

```
