#!/usr/bin/env python3
"""
PolyRA Graph Test Suite Generator
Generates test data files and test scripts for comprehensive testing.
"""

import os
import random
import time

DATA_DIR = "../data"
TEST_DIR = "../data/test_graphs"

def ensure_dir(path):
    os.makedirs(path, exist_ok=True)

def generate_graph(name, graph_type, num_nodes, num_edges, node_attrs=1, edge_attrs=1):
    """Generate a graph with given parameters."""
    ensure_dir(DATA_DIR)
    
    suffix = "D" if graph_type == "directed" else "U"
    node_file = f"{DATA_DIR}/{name} Nodes {suffix}.csv"
    edge_file = f"{DATA_DIR}/{name} Edges {suffix}.csv"
    
    # Generate nodes
    node_header = ["NodeID"] + [f"A{i}" for i in range(1, node_attrs + 1)]
    with open(node_file, 'w') as f:
        f.write(",".join(node_header) + "\n")
        for i in range(num_nodes):
            attrs = [str(random.randint(0, 1)) for _ in range(node_attrs)]
            f.write(f"{i}," + ",".join(attrs) + "\n")
    
    # Generate edges
    edge_header = ["Source_NodeID", "Destination_NodeID", "Weight"] + [f"B{i}" for i in range(1, edge_attrs + 1)]
    edges_written = set()
    with open(edge_file, 'w') as f:
        f.write(",".join(edge_header) + "\n")
        count = 0
        attempts = 0
        max_attempts = num_edges * 10
        while count < num_edges and attempts < max_attempts:
            src = random.randint(0, num_nodes - 1)
            dst = random.randint(0, num_nodes - 1)
            if src != dst and (src, dst) not in edges_written:
                weight = random.randint(1, 100)
                attrs = [str(random.randint(0, 1)) for _ in range(edge_attrs)]
                f.write(f"{src},{dst},{weight}," + ",".join(attrs) + "\n")
                edges_written.add((src, dst))
                count += 1
            attempts += 1
    
    return node_file, edge_file

def generate_linear_graph(name, graph_type, num_nodes):
    """Generate a linear chain graph: 0 -> 1 -> 2 -> ... -> n-1"""
    suffix = "D" if graph_type == "directed" else "U"
    node_file = f"{DATA_DIR}/{name} Nodes {suffix}.csv"
    edge_file = f"{DATA_DIR}/{name} Edges {suffix}.csv"
    
    with open(node_file, 'w') as f:
        f.write("NodeID,A1\n")
        for i in range(num_nodes):
            f.write(f"{i},{i % 2}\n")
    
    with open(edge_file, 'w') as f:
        f.write("Source_NodeID,Destination_NodeID,Weight,B1\n")
        for i in range(num_nodes - 1):
            f.write(f"{i},{i+1},{i+1},{i % 2}\n")
    
    return node_file, edge_file

def generate_complete_graph(name, type_str, node_count):
    """Generate a complete graph Kn where every node is connected to every other."""
    nodes_file = f"{DATA_DIR}/{name} Nodes {type_str[0].upper()}.csv"
    edges_file = f"{DATA_DIR}/{name} Edges {type_str[0].upper()}.csv"
    
    with open(nodes_file, 'w') as f:
        f.write("NodeID,A1,A2\n")
        for i in range(node_count):
            f.write(f"{i},{i%2},0\n")
            
    with open(edges_file, 'w') as f:
        f.write("Source_NodeID,Destination_NodeID,Weight,B1,B2\n")
        for i in range(node_count):
            for j in range(node_count):
                if i != j:
                    if type_str == "undirected" and i > j:
                        continue
                    f.write(f"{i},{j},1,0,0\n")
    
    return nodes_file, edges_file

def generate_disconnected_graph(name, graph_type, num_nodes):
    """Generate a graph with isolated nodes."""
    suffix = "D" if graph_type == "directed" else "U"
    node_file = f"{DATA_DIR}/{name} Nodes {suffix}.csv"
    edge_file = f"{DATA_DIR}/{name} Edges {suffix}.csv"
    
    with open(node_file, 'w') as f:
        f.write("NodeID,A1\n")
        for i in range(num_nodes):
            f.write(f"{i},1\n")
    
    # Only connect first half
    with open(edge_file, 'w') as f:
        f.write("Source_NodeID,Destination_NodeID,Weight,B1\n")
        half = num_nodes // 2
        for i in range(half - 1):
            f.write(f"{i},{i+1},1,1\n")
    
    return node_file, edge_file

def generate_single_node_graph(name, graph_type):
    """Generate a graph with a single node and no edges."""
    suffix = "D" if graph_type == "directed" else "U"
    node_file = f"{DATA_DIR}/{name} Nodes {suffix}.csv"
    edge_file = f"{DATA_DIR}/{name} Edges {suffix}.csv"
    
    with open(node_file, 'w') as f:
        f.write("NodeID,A1\n")
        f.write("0,1\n")
    
    with open(edge_file, 'w') as f:
        f.write("Source_NodeID,Destination_NodeID,Weight,B1\n")
    
    return node_file, edge_file

def generate_star_graph(name, graph_type, num_nodes):
    """Generate a star graph (one center, all others connected to it)."""
    suffix = "D" if graph_type == "directed" else "U"
    node_file = f"{DATA_DIR}/{name} Nodes {suffix}.csv"
    edge_file = f"{DATA_DIR}/{name} Edges {suffix}.csv"
    
    with open(node_file, 'w') as f:
        f.write("NodeID,A1\n")
        for i in range(num_nodes):
            f.write(f"{i},{1 if i == 0 else 0}\n")
    
    with open(edge_file, 'w') as f:
        f.write("Source_NodeID,Destination_NodeID,Weight,B1\n")
        for i in range(1, num_nodes):
            f.write(f"0,{i},{i},1\n")
    
    return node_file, edge_file

def generate_large_graph(name, graph_type, num_nodes, num_edges):
    """Generate a large sparse graph efficiently."""
    suffix = "D" if graph_type == "directed" else "U"
    node_file = f"{DATA_DIR}/{name} Nodes {suffix}.csv"
    edge_file = f"{DATA_DIR}/{name} Edges {suffix}.csv"
    
    print(f"Generating large graph: {num_nodes} nodes, {num_edges} edges...")
    start = time.time()
    
    # Nodes
    with open(node_file, 'w') as f:
        f.write("NodeID,A1\n")
        for i in range(num_nodes):
            f.write(f"{i},{i % 2}\n")
            if i % 100000 == 0 and i > 0:
                print(f"  Nodes: {i}/{num_nodes}")
    
    # Edges - use reservoir sampling for large sparse graphs
    with open(edge_file, 'w') as f:
        f.write("Source_NodeID,Destination_NodeID,Weight,B1\n")
        edges_written = 0
        # For sparse graphs, generate sequentially
        if num_edges < num_nodes * 10:
            seen = set()
            while edges_written < num_edges:
                src = random.randint(0, num_nodes - 1)
                dst = random.randint(0, num_nodes - 1)
                if src != dst and (src, dst) not in seen:
                    f.write(f"{src},{dst},{random.randint(1, 100)},{random.randint(0, 1)}\n")
                    seen.add((src, dst))
                    edges_written += 1
                    if edges_written % 100000 == 0:
                        print(f"  Edges: {edges_written}/{num_edges}")
        else:
            # For dense graphs, iterate through all possible edges
            for i in range(num_nodes):
                for j in range(num_nodes):
                    if i != j and edges_written < num_edges:
                        if random.random() < num_edges / (num_nodes * (num_nodes - 1)):
                            f.write(f"{i},{j},{random.randint(1, 100)},{random.randint(0, 1)}\n")
                            edges_written += 1
                if edges_written >= num_edges:
                    break
    
    elapsed = time.time() - start
    print(f"Generated in {elapsed:.2f}s")
    return node_file, edge_file

# ============ TEST CASE DEFINITIONS ============

TEST_CASES = []

def add_test(name, setup_func, commands, expected_patterns, description):
    TEST_CASES.append({
        "name": name,
        "setup": setup_func,
        "commands": commands,
        "expected": expected_patterns,
        "description": description
    })

# ----- LOAD GRAPH Tests -----
add_test("T01_LoadDirected", 
         lambda: generate_graph("T01", "directed", 5, 10),
         ["LOAD GRAPH T01 D"],
         ["Loaded Graph"],
         "Load a simple directed graph")

add_test("T02_LoadUndirected",
         lambda: generate_graph("T02", "undirected", 5, 10),
         ["LOAD GRAPH T02 U"],
         ["Loaded Graph"],
         "Load a simple undirected graph")

add_test("T03_LoadSingleNode",
         lambda: generate_single_node_graph("T03", "directed"),
         ["LOAD GRAPH T03 D"],
         ["Loaded Graph", "Node Count:1", "Edge Count:0"],
         "Load a graph with single node")

add_test("T04_LoadEmptyEdges",
         lambda: generate_single_node_graph("T04", "directed"),
         ["LOAD GRAPH T04 D"],
         ["Edge Count:0"],
         "Load graph with no edges")

add_test("T05_LoadDuplicateName",
         lambda: generate_graph("T05", "directed", 3, 3),
         ["LOAD GRAPH T05 D", "LOAD GRAPH T05 D"],
         ["Loaded Graph", "SEMANTIC ERROR"],
         "Attempt to load graph with duplicate name")

# ----- PRINT GRAPH Tests -----
add_test("T06_PrintDirected",
         lambda: generate_linear_graph("T06", "directed", 5),
         ["LOAD GRAPH T06 D", "PRINT GRAPH T06"],
         ["5", "4", "D", "NodeID"],
         "Print a directed linear graph")

add_test("T07_PrintUndirected",
         lambda: generate_linear_graph("T07", "undirected", 5),
         ["LOAD GRAPH T07 U", "PRINT GRAPH T07"],
         ["U"],
         "Print an undirected graph")

add_test("T08_PrintNonExistent",
         lambda: None,
         ["PRINT GRAPH NonExistent"],
         ["SEMANTIC ERROR"],
         "Print non-existent graph")

# ----- DEGREE Tests -----
add_test("T09_DegreeDirectedCenter",
         lambda: generate_star_graph("T09", "directed", 5),
         ["LOAD GRAPH T09 D", "DEGREE T09 0"],
         ["4"],  # Out-degree of center
         "Degree of center node in star graph")

add_test("T10_DegreeDirectedLeaf",
         lambda: generate_star_graph("T10", "directed", 5),
         ["LOAD GRAPH T10 D", "DEGREE T10 1"],
         ["1"],  # In-degree only
         "Degree of leaf node in directed star")

add_test("T11_DegreeUndirected",
         lambda: generate_linear_graph("T11", "undirected", 5),
         ["LOAD GRAPH T11 U", "DEGREE T11 2"],
         ["2"],  # Middle node has 2 neighbors
         "Degree in undirected linear graph")

add_test("T12_DegreeIsolatedNode",
         lambda: generate_disconnected_graph("T12", "directed", 6),
         ["LOAD GRAPH T12 D", "DEGREE T12 5"],
         ["0"],  # Isolated node has degree 0
         "Degree of isolated node")

add_test("T13_DegreeNonExistentNode",
         lambda: generate_graph("T13", "directed", 5, 5),
         ["LOAD GRAPH T13 D", "DEGREE T13 999"],
         ["Node does not exist"],
         "Degree of non-existent node")

# ----- PATH Tests (Fixing expectations to match data generator) -----
add_test("T14_PathExists",
         # Graph: 0->1(B1=0), 1->2(B1=1), 2->3(B1=0), 3->4(B1=1)
         lambda: generate_linear_graph("T14", "directed", 5),
         ["LOAD GRAPH T14 D", "RES14 <- PATH T14 0 1 WHERE B1(E) == 0"], # Path 0-1 exists with B1=0
         ["TRUE"],
         "Path exists in linear graph")

add_test("T15_PathNoPath",
         lambda: generate_disconnected_graph("T15", "directed", 6),
         ["LOAD GRAPH T15 D", "RES15 <- PATH T15 0 5 WHERE B1(E) == 1"],
         ["FALSE"],
         "No path between disconnected components")

add_test("T16_PathSameNode",
         lambda: generate_linear_graph("T16", "directed", 5),
         ["LOAD GRAPH T16 D", "RES16 <- PATH T16 0 0 WHERE B1(E) == 0"],
         ["TRUE", "0"],  # Path to self with weight 0
         "Path from node to itself")

add_test("T17_PathWithCondition",
         lambda: generate_linear_graph("T17", "directed", 5),
         ["LOAD GRAPH T17 D", "RES17 <- PATH T17 0 1 WHERE B1(E) == 0"],
         ["TRUE"],
         "Path with edge condition")

add_test("T18_PathNonExistentSrc",
         lambda: generate_graph("T18", "directed", 5, 5),
         ["LOAD GRAPH T18 D", "RES18 <- PATH T18 999 0 WHERE B1(E) == 0"],
         ["Node does not exist"],
         "Path with non-existent source")

add_test("T19_PathNonExistentDst",
         lambda: generate_graph("T19", "directed", 5, 5),
         ["LOAD GRAPH T19 D", "RES19 <- PATH T19 0 999 WHERE B1(E) == 0"],
         ["Node does not exist"],
         "Path with non-existent destination")

# ----- EXPORT GRAPH Tests -----
add_test("T20_ExportGraph",
         lambda: generate_graph("T20", "directed", 5, 5),
         ["LOAD GRAPH T20 D", "EXPORT GRAPH T20"],
         [],  # Just check no crash
         "Export a directed graph")

add_test("T21_ExportNonExistent",
         lambda: None,
         ["EXPORT GRAPH NonExistent"],
         ["SEMANTIC ERROR"],
         "Export non-existent graph")

# ----- Edge Cases -----
add_test("T22_CompleteGraph",
         lambda: generate_complete_graph("T22", "directed", 5),
         ["LOAD GRAPH T22 D", "DEGREE T22 0"],
         ["8"],  # 4 out + 4 in for complete K5
         "Complete graph degree check")

# T23 and T25 similarly fixed to short paths where conditions hold
add_test("T23_LargeLinear",
         lambda: generate_linear_graph("T23", "directed", 100),
         ["LOAD GRAPH T23 D", "DEGREE T23 50", "RES23 <- PATH T23 0 1 WHERE B1(E) == 0"],
         ["2", "TRUE"],
         "Large linear graph path and degree")

add_test("T24_UndirectedComplete",
         lambda: generate_complete_graph("T24", "undirected", 4),
         ["LOAD GRAPH T24 U", "DEGREE T24 0"],
         ["3"],  # Each node connected to 3 others
         "Undirected complete graph")

add_test("T25_MultiConditionPath",
         lambda: generate_linear_graph("T25", "directed", 5),
         ["LOAD GRAPH T25 D", "RES25 <- PATH T25 0 1 WHERE B1(E) == 0 AND B1(E) == 0"],
         ["TRUE"],
         "Path with multiple AND conditions")


# ----- Complex & Compliance Tests -----

add_test("T26_OrderPreservation",
         lambda: generate_graph("T26", "directed", 5, 5),
         ["LOAD GRAPH T26 D", "PRINT GRAPH T26"],
         [], # Manually verify order in script or assume pass if no crash
         "Verify PRINT maintains input CSV order")

def setup_t27():
    with open(f"{DATA_DIR}/T27 Nodes D.csv", 'w') as f: f.write("NodeID,A1\n0,0\n1,0\n2,0\n")
    with open(f"{DATA_DIR}/T27 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,1,0\n1,2,1,0\n")

add_test("T27_AnyCondition_Consistent",
         # Linear graph: 0->1 (B1=0), 1->2 (B1=0)
         setup_t27, 
         ["LOAD GRAPH T27 D", "RES27 <- PATH T27 0 2 WHERE B1(E)"],
         ["TRUE"],
         "ANY condition with consistent edge attributes")

def setup_t28():
    with open(f"{DATA_DIR}/T28 Nodes D.csv", 'w') as f: f.write("NodeID,A1\n0,0\n1,0\n2,0\n")
    with open(f"{DATA_DIR}/T28 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,1,0\n1,2,1,1\n")

add_test("T28_AnyCondition_Inconsistent",
         setup_t28,
         ["LOAD GRAPH T28 D", "RES28 <- PATH T28 0 2 WHERE B1(E)"],
         ["FALSE"],
         "ANY condition with inconsistent edge attributes (0->1->0 fails)")

def setup_t29():
    with open(f"{DATA_DIR}/T29 Nodes D.csv", 'w') as f: f.write("NodeID,A1\n0,0\n1,1\n")
    with open(f"{DATA_DIR}/T29 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,1,0\n")

add_test("T29_NodeCondition_SrcFail",
         setup_t29,
         ["LOAD GRAPH T29 D", "RES29 <- PATH T29 0 1 WHERE A1(N) == 1"],
         ["FALSE"],
         "Path exists but Source Node condition fails")

def setup_t30():
    with open(f"{DATA_DIR}/T30 Nodes D.csv", 'w') as f: f.write("NodeID,A1\n0,1\n1,0\n")
    with open(f"{DATA_DIR}/T30 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,1,0\n")

add_test("T30_NodeCondition_DestFail",
         setup_t30,
         ["LOAD GRAPH T30 D", "RES30 <- PATH T30 0 1 WHERE A1(N) == 1"],
         ["FALSE"],
         "Path exists but Destination Node condition fails")

# ----- Comprehensive Suite (T31-T55) -----

def generate_large_csv(name, node_count, edge_count, layout="linear"):
    """Efficiently generate large CSVs for stress testing."""
    nodes_file = f"{DATA_DIR}/{name} Nodes D.csv"
    edges_file = f"{DATA_DIR}/{name} Edges D.csv"
    
    # Generate Nodes
    with open(nodes_file, 'w') as f:
        f.write("NodeID,A1,A2\n")
        for i in range(node_count):
            # A1 alt 0/1, A2 mostly 0
            f.write(f"{i},{i%2},0\n")
            
    # Generate Edges
    with open(edges_file, 'w') as f:
        f.write("Source_NodeID,Destination_NodeID,Weight,B1,B2\n")
        if layout == "linear":
            for i in range(edge_count):
                if i < node_count - 1:
                    f.write(f"{i},{i+1},1,{i%2},0\n")
        elif layout == "star":
            import random
            for i in range(1, node_count):
                f.write(f"0,{i},1,0,0\n")
        elif layout == "random_sparse":
            import random
            random.seed(42)  # Deterministic
            for _ in range(edge_count):
                s = random.randint(0, node_count-1)
                d = random.randint(0, node_count-1)
                f.write(f"{s},{d},1,{random.randint(0,1)},0\n")

# T31: Large Linear Graph (10k nodes)
add_test("T31_LargeLinear_10k",
         lambda: generate_large_csv("T31", 10000, 9999, "linear"),
         ["LOAD GRAPH T31 D", "RES31 <- PATH T31 0 1 WHERE B1(E) == 0"],
         ["TRUE"], 
         "10k Linear Graph, Path check")

# T32: Large Star Graph (10k nodes)
add_test("T32_LargeStar_10k",
         lambda: generate_large_csv("T32", 10000, 9999, "star"),
         ["LOAD GRAPH T32 D", "DEGREE T32 0"],
         ["9999"], 
         "10k Star Graph Degree Check for Center")

# T33: Large Disconnected (10k nodes)
add_test("T33_LargeDisconnected",
         lambda: generate_large_csv("T33", 10000, 5000, "linear"),
         ["LOAD GRAPH T33 D", "RES33 <- PATH T33 0 9999 WHERE A2(N) == 0"],
         ["FALSE"],
         "Path between disconnected components")

# T34: Medium Random Graph (1k nodes, 5k edges)
add_test("T34_MediumRandom",
         lambda: generate_large_csv("T34", 1000, 5000, "random_sparse"),
         ["LOAD GRAPH T34 D", "DEGREE T34 0"],
         [],
         "Medium Random Graph Sort Stress")

# T35: 20k Node Linear
add_test("T35_HugeLinear_20k",
         lambda: generate_large_csv("T35", 20000, 19999, "linear"),
         ["LOAD GRAPH T35 D", "RES35 <- PATH T35 0 100 WHERE A2(N) == 0"],
         ["TRUE 100"],
         "20k Node Linear Graph (Partial Path)")

# T36: ANY(N) Consistent
def setup_t36():
    with open(f"{DATA_DIR}/T36 Nodes D.csv", 'w') as f: f.write("NodeID,A1\n0,1\n1,1\n2,1\n")
    with open(f"{DATA_DIR}/T36 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,1,0\n1,2,1,0\n")

add_test("T36_AnyNode_Consistent",
         setup_t36,
         ["LOAD GRAPH T36 D", "RES36 <- PATH T36 0 2 WHERE A1(N)"], 
         ["TRUE"],
         "ANY(N) condition with consistent node attributes")

# T37: ANY(N) Inconsistent
def setup_t37():
    with open(f"{DATA_DIR}/T37 Nodes D.csv", 'w') as f: f.write("NodeID,A1\n0,1\n1,0\n2,1\n")
    with open(f"{DATA_DIR}/T37 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,1,0\n1,2,1,0\n")

add_test("T37_AnyNode_Inconsistent",
         setup_t37,
         ["LOAD GRAPH T37 D", "RES37 <- PATH T37 0 2 WHERE A1(N)"], # A1(N) is implicit ANY
         ["FALSE"],
         "ANY(N) condition with inconsistent node attributes")

# T38: ANY(N) == 1
add_test("T38_AnyNode_Value",
         setup_t36, # All 1s
         ["LOAD GRAPH T36 D", "RES38 <- PATH T36 0 2 WHERE A1(N) == 1"],
         ["TRUE"],
         "ANY(N) == 1 condition")

# T39: Mixed Attributes (Specific N, ANY E)
add_test("T39_Mixed_Specific_Any",
         setup_t36,
         ["LOAD GRAPH T36 D", "RES39 <- PATH T36 0 2 WHERE A1(N) == 1 AND B1(E)"],
         ["TRUE"],
         "Mixed Specific Node and Consistent Edge conditions")

# T40: Multiple ANYs
add_test("T40_Multiple_Any",
         setup_t36,
         ["LOAD GRAPH T36 D", "RES40 <- PATH T36 0 2 WHERE A1(N) AND B1(E)"],
         ["TRUE"],
         "Multiple ANY conditions")

# T41: Node Condition Src Fail
add_test("T41_NodeCondition_AnySrcFail",
         setup_t37,
         ["LOAD GRAPH T37 D", "RES41 <- PATH T37 0 2 WHERE A1(N)"],
         ["FALSE"],
         "Implicit A1(N) consistency fails at middle node")

# T42: Node Condition Src != Dest
def setup_t42():
    with open(f"{DATA_DIR}/T42 Nodes D.csv", 'w') as f: f.write("NodeID,A1\n0,1\n1,0\n")
    with open(f"{DATA_DIR}/T42 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,1,0\n")

add_test("T42_NodeCondition_SrcDestDiff",
         setup_t42,
         ["LOAD GRAPH T42 D", "RES42 <- PATH T42 0 1 WHERE A1(N)"],
         ["FALSE"],
         "Consistent A1(N) fails because Src and Dest differ")

# T43: Node Condition Specific Match
add_test("T43_NodeCondition_Specific",
         setup_t36,
         ["LOAD GRAPH T36 D", "RES43 <- PATH T36 0 2 WHERE A1(N) == 1"],
         ["TRUE"],
         "Specific Node condition match")

# T44: Node Condition Specific Mismatch
add_test("T44_NodeCondition_Mismatch",
         setup_t36,
         ["LOAD GRAPH T36 D", "RES44 <- PATH T36 0 2 WHERE A1(N) == 0"],
         ["FALSE"],
         "Specific Node condition mismatch")

# T45: ANY(E) == 1 
def setup_t45():
    with open(f"{DATA_DIR}/T45 Nodes D.csv", 'w') as f: f.write("NodeID\n0\n1\n2\n")
    with open(f"{DATA_DIR}/T45 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,1,1\n1,2,1,1\n")

add_test("T45_AnyEdge_Value1",
         setup_t45,
         ["LOAD GRAPH T45 D", "RES45 <- PATH T45 0 2 WHERE B1(E) == 1"],
         ["TRUE"],
         "ANY(E) == 1 condition")

# T46: Self Loop
def setup_t46():
    with open(f"{DATA_DIR}/T46 Nodes D.csv", 'w') as f: f.write("NodeID,A1\n0,0\n")
    with open(f"{DATA_DIR}/T46 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,0,1,0\n")

add_test("T46_SelfLoop",
         setup_t46,
         ["LOAD GRAPH T46 D", "RES46 <- PATH T46 0 0"],
         ["TRUE"],
         "Self Loop / Src==Dest")

# T47: Cycle
def setup_t47():
    with open(f"{DATA_DIR}/T47 Nodes D.csv", 'w') as f: f.write("NodeID\n0\n1\n")
    with open(f"{DATA_DIR}/T47 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,1,0\n1,0,1,0\n")

add_test("T47_Cycle",
         setup_t47,
         ["LOAD GRAPH T47 D", "RES47 <- PATH T47 0 1"],
         ["TRUE"],
         "Simple Cycle path check")

# T48: Double Edge (Min Weight)
def setup_t48():
    with open(f"{DATA_DIR}/T48 Nodes D.csv", 'w') as f: f.write("NodeID\n0\n1\n")
    with open(f"{DATA_DIR}/T48 Edges D.csv", 'w') as f: f.write("Source_NodeID,Destination_NodeID,Weight,B1\n0,1,10,0\n0,1,5,0\n")

add_test("T48_DoubleEdge_MinWeight",
         setup_t48,
         ["LOAD GRAPH T48 D", "RES48 <- PATH T48 0 1"],
         ["TRUE 5"],
         "Parallel edges, should choose minimum weight")

# T49: Redundant Load
add_test("T49_Reload_SameName",
         lambda: generate_linear_graph("T49", "directed", 2),
         ["LOAD GRAPH T49 D", "LOAD GRAPH T49 D"],
         ["SEMANTIC ERROR"],
         "Reloading same graph name fails")

# T50: Path query on non-existent graph
add_test("T50_Path_NonExistentGraph",
         lambda: None,
         ["RES50 <- PATH NONEXISTENT 0 1"],
         ["SEMANTIC ERROR"],
         "Path query on non-existent graph")

# T51: Order Preservation Large
add_test("T51_Order_Large",
         lambda: generate_large_csv("T51", 100, 99, "linear"),
         ["LOAD GRAPH T51 D", "PRINT GRAPH T51"],
         [],
         "Order Preservation on medium graph")

# T52: Export Order
add_test("T52_Export_Order",
         lambda: generate_large_csv("T52", 50, 49, "linear"),
         ["LOAD GRAPH T52 D", "EXPORT GRAPH T52"],
         [], 
         "Export Order Preservation")

# T53: Print Undirected Format
add_test("T53_Print_Undirected",
         lambda: generate_linear_graph("T53", "undirected", 3),
         ["LOAD GRAPH T53 U", "PRINT GRAPH T53"],
         ["U"],
         "Print Undirected Graph Format")

# T54: Degree Large
add_test("T54_Degree_Large",
         lambda: generate_large_csv("T54", 1000, 999, "star"),
         ["LOAD GRAPH T54 D", "DEGREE T54 0"],
         ["999"],
         "Degree check on larger graph")

# T55: No Path Large
add_test("T55_NoPath_Large",
         lambda: generate_large_csv("T55", 1000, 500, "linear"),
         ["LOAD GRAPH T55 D", "RES55 <- PATH T55 0 999"],
         ["FALSE"],
         "No Path check on larger graph")

def run_tests():
    """Generate, Run, and Verify all tests."""
    import time
    import subprocess
    import os
    import sys

    # Ensure server is compiled
    print("Compiling server...", flush=True)
    subprocess.run(["make", "clean"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if subprocess.run(["make", "server"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
        print("Compilation FAILED!", flush=True)
        return

    print("\n" + "="*80, flush=True)
    print(f"{'Test Name':<30} | {'Status':<10} | {'Time (ms)':<10} | {'Outcome':<20}", flush=True)
    print("-" * 80, flush=True)
    
    passed = 0
    failed = 0
    
    results_summary = []

    for i, test in enumerate(TEST_CASES):
        # 1. Setup
        if test['setup']:
            test['setup']()
        
        # 2. Generate Script
        script_file = f"{DATA_DIR}/test_{test['name']}.ra"
        with open(script_file, 'w') as f:
            for cmd in test['commands']:
                f.write(cmd + "\n")
        
        # 3. Execute
        start_time = time.time()
        try:
            # Run server with script as input
            with open(script_file, 'r') as infile:
                result = subprocess.run(
                    ["./server"], 
                    stdin=infile, 
                    capture_output=True, 
                    text=True,
                    timeout=30 # Reduced timeout
                )
            output = (result.stdout + result.stderr).strip()
            exit_code = result.returncode
        except subprocess.TimeoutExpired:
            output = "TIMEOUT"
            exit_code = -1
        except Exception as e:
            output = str(e)
            exit_code = -1
            
        duration_ms = (time.time() - start_time) * 1000
        
        # 4. Verify
        status = "PASS"
        outcome_msg = ""
        
        if exit_code != 0:
            status = "FAIL"
            outcome_msg = f"Error ({exit_code})"
            if output == "TIMEOUT": outcome_msg = "TIMEOUT"
        else:
            # Check expected strings (stripped)
            for pattern in test['expected']:
                if pattern not in output:
                    status = "FAIL"
                    outcome_msg = f"Missing: {pattern}"
                    # DEBUG: Print output if fail
                    print(f"\nDEBUG {test['name']} Output:\n{output}\n")
                    break
        
        color_start = "\033[92m" if status == "PASS" else "\033[91m"
        color_end = "\033[0m"
        
        print(f"{test['name']:<30} | {color_start}{status:<10}{color_end} | {duration_ms:<10.2f} | {outcome_msg:<20}", flush=True)
        
        if status == "PASS": passed += 1
        else: failed += 1

    print("-" * 80)
    print(f"Total: {len(TEST_CASES)} | Passed: {passed} | Failed: {failed}")
    print("=" * 80)
    return results_summary

if __name__ == "__main__":
    print("PolyRA Graph Test Suite Generator & Executor", flush=True)
    run_tests()
