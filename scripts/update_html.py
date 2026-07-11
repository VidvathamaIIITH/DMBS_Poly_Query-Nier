import sys

with open('docs/Overview.html', 'r', encoding='utf-8') as f:
    content = f.read()

header_idx = content.find('<div id="p">')
if header_idx == -1:
    print("Could not find <div id='p'>")
    sys.exit(1)

header = content[:header_idx + len('<div id="p">')]

slides = [
    """
<h1>PolyRA</h1>
<p><em>A minimalist, integer-only Relational Database Management System with advanced query and graph capabilities.</em></p>
<h2><em>Data Systems, Monsoon 2020</em></h2>
    """,
    """
<h3>Important Features</h3>
<ul>
<li>Relational Algebra Operators (Select, Project, Rename, Cross, Join)</li>
<li>Integers Only Data</li>
<li>Aggregate operations &amp; Grouping (<code>SUM</code>, <code>AVG</code>, <code>MIN</code>, <code>MAX</code>, <code>GROUP BY</code>)</li>
<li>External Sorting &amp; Indexing Optimization</li>
<li>Graph Processing (Directed/Undirected, Path algorithms, Degree)</li>
<li>Transaction Management with Concurrency Control (Wait-Die protocol)</li>
<li>Multi-threaded Execution</li>
</ul>
    """,
    """
<h3>Commands</h3>
<p>There are 2 kinds of commands in this database:</p>
<ul>
<li>Assignment statements (create a new relation/table)</li>
<li>Non-assignment statements (do not create a new relation)</li>
</ul>
    """,
    """
<h2>Non Assignment Statements</h2>
<p>Non-assignment statements do not create a new table (except <code>LOAD</code> which loads an existing entity into memory).</p>
<ul>
<li>LOAD / LOAD GRAPH</li>
<li>LIST TABLES / LIST GRAPHS</li>
<li>PRINT / PRINT GRAPH</li>
<li>RENAME</li>
<li>EXPORT / EXPORT GRAPH</li>
<li>CLEAR</li>
<li>INDEX</li>
<li>QUIT</li>
</ul>
    """,
    """
<h3>LOAD &amp; SOURCE</h3>
<p>Syntax:</p>
<pre><code>LOAD &lt;table_name&gt;
LOAD GRAPH &lt;graph_name&gt; &lt;U/D&gt;
SOURCE &lt;query_name&gt;
</code></pre>
<ul>
<li><code>LOAD</code>: Loads a table from <code>data/&lt;table_name&gt;.csv</code></li>
<li><code>LOAD GRAPH</code>: Loads graph nodes and edges from CSVs (U for Undirected, D for Directed).</li>
<li><code>SOURCE</code>: Executes a script of queries from <code>data/&lt;query_name&gt;.ra</code></li>
</ul>
    """,
    """
<h3>PRINT &amp; LIST</h3>
<p>Syntax:</p>
<pre><code>PRINT &lt;table_name&gt;
PRINT GRAPH &lt;graph_name&gt;
LIST TABLES
</code></pre>
<ul>
<li><code>PRINT</code>: Displays the first few rows (up to <code>PRINT_COUNT</code>).</li>
<li><code>LIST TABLES</code>: Shows all tables loaded or created in the current session.</li>
</ul>
    """,
    """
<h3>RENAME</h3>
<p>Syntax:</p>
<pre><code>RENAME &lt;toColumnName&gt; TO &lt;fromColumnName&gt; FROM &lt;table_name&gt;
</code></pre>
<ul>
<li>Renames a specific column in an existing table.</li>
</ul>
    """,
    """
<h3>EXPORT &amp; CLEAR</h3>
<p>Syntax:</p>
<pre><code>EXPORT &lt;table_name&gt;
EXPORT GRAPH &lt;graph_name&gt;
CLEAR &lt;table_name&gt;
</code></pre>
<ul>
<li><code>EXPORT</code>: Writes the in-memory changes back to disk as a CSV file.</li>
<li><code>CLEAR</code>: Removes a table from the system's memory.</li>
</ul>
    """,
    """
<h3>INDEX</h3>
<p>Syntax:</p>
<pre><code>INDEX ON &lt;columnName&gt; FROM &lt;table_name&gt; USING &lt;indexing_strategy&gt;
</code></pre>
<p>Where <code>&lt;indexing_strategy&gt;</code> could be:</p>
<ul>
<li><code>BTREE</code> - BTree indexing on column</li>
<li><code>HASH</code> - Index via a hashmap</li>
<li><code>NOTHING</code> - Removes index if present </li>
</ul>
    """,
    """
<h2>Assignment Statements</h2>
<ul>
<li>All assignment statements lead to the creation of a new table. </li>
<li>Every statement is of the form <code>&lt;new_table_name&gt; &lt;- &lt;assignment_statement&gt;</code></li>
<li>The newly created table can be operated on or exported.</li>
</ul>
    """,
    """
<h3>SELECTION &amp; PROJECTION</h3>
<p>Syntax</p>
<pre><code>&lt;new_table&gt; &lt;- SELECT &lt;condition&gt; FROM &lt;table_name&gt;
&lt;new_table&gt; &lt;- PROJECT &lt;col1&gt;(,&lt;colN&gt;)* FROM &lt;table_name&gt;
</code></pre>
<ul>
<li><strong>SELECT</strong>: Filters rows based on <code>&lt;condition&gt;</code> (e.g. <code>A &gt; 5</code>, <code>A == B</code>).</li>
<li><strong>PROJECT</strong>: Keeps only the specified columns.</li>
</ul>
    """,
    """
<h3>JOIN &amp; CROSS</h3>
<p>Syntax</p>
<pre><code>&lt;new_table&gt; &lt;- JOIN &lt;table1&gt;, &lt;table2&gt; ON &lt;column1&gt; &lt;bin_op&gt; &lt;column2&gt;
&lt;new_table&gt; &lt;- CROSS &lt;table1&gt; &lt;table2&gt;
</code></pre>
<ul>
<li><strong>JOIN</strong>: Performs a relational join on two tables.</li>
<li><strong>CROSS</strong>: Performs a cartesian product of two tables.</li>
</ul>
    """,
    """
<h3>GROUP BY &amp; AGGREGATE</h3>
<p>Syntax</p>
<pre><code>&lt;new_table&gt; &lt;- GROUP BY &lt;col&gt; FROM &lt;table&gt; RETURN MAX(&lt;col1&gt;), MIN(&lt;col2&gt;)
</code></pre>
<ul>
<li>Groups rows by the specified column and returns aggregate computations. Supports <code>MAX</code>, <code>MIN</code>, <code>SUM</code>, <code>AVG</code>.</li>
</ul>
    """,
    """
<h3>SORT</h3>
<p>Syntax</p>
<pre><code>&lt;new_table&gt; &lt;- SORT &lt;table_name&gt; BY &lt;column_name&gt; IN &lt;sorting_order&gt;
</code></pre>
<ul>
<li>Sorts a table using external sorting techniques for handling large datasets that exceed memory limits.</li>
<li><code>&lt;sorting_order&gt;</code> can be <code>ASC</code> or <code>DESC</code>.</li>
</ul>
    """,
    """
<h3>PATH (Graph Algorithms)</h3>
<p>Syntax</p>
<pre><code>&lt;new_table&gt; &lt;- PATH &lt;graph_name&gt; &lt;src_NodeID&gt; &lt;dest_NodeID&gt; [WHERE &lt;conditions&gt;]
</code></pre>
<ul>
<li>Computes whether a path exists between two nodes in a given graph.</li>
<li>Supports filtering paths based on specific edge/node constraints.</li>
</ul>
    """,
    """
<h3>Transaction Management</h3>
<ul>
<li>Supports multi-threaded execution of queries using <code>TRANSACTION</code>.</li>
<li>Implements a strict <strong>Wait-Die</strong> deadlock prevention scheme.</li>
<li>Manages shared and exclusive locks dynamically for safe concurrent data access.</li>
</ul>
    """,
    """
<h3>Internals</h3>
<ul>
<li><strong>Buffer Manager</strong>: Manages memory limits, evicts pages (FIFO).</li>
<li><strong>Table / Graph Catalogue</strong>: Indexes entities currently loaded.</li>
<li><strong>Cursors</strong>: Read from tables seamlessly block-by-block.</li>
<li><strong>Executors</strong>: Implement the core logic for each operator.</li>
</ul>
    """,
    """
<h3>Command Execution Flow</h3>
<p><img src="flow.png" alt="" /></p>
<p>Query goes through Syntactic Parser -&gt; Semantic Parser -&gt; Executor.</p>
    """,
    """
<h2>Architecture Evolution</h2>
<ul>
<li><strong>Phase 1</strong>: Basic Relational Algebra Operators</li>
<li><strong>Phase 2</strong>: Graph Handling &amp; Advanced Queries (External Sort, Group By)</li>
<li><strong>Phase 3</strong>: Transaction Management &amp; Concurrency Control</li>
</ul>
    """,
    """
<h2>References</h2>
<ul>
<li>Base framework derived from <a href="https://github.com/PolyRA/PolyRA">PolyRA</a></li>
<li>Extended with advanced functionality by Team 20.</li>
</ul>
    """
]

new_html = header
for i, slide in enumerate(slides):
    new_html += f'<svg data-marpit-svg="" viewBox="0 0 1280 720"><foreignObject width="1280" height="720"><section id="{i+1}" data-class="invert" data-background-color="black" data-theme="gaia" class="invert" style="--class:invert;--background-color:black;--theme:gaia;background-color:black;background-image:none;">'
    new_html += slide
    new_html += '</section></foreignObject></svg>'

new_html += '</div></body></html>'

with open('docs/Overview.html', 'w', encoding='utf-8') as f:
    f.write(new_html)

print("Updated Overview.html successfully.")
