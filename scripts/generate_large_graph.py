#!/usr/bin/env python3
"""
Large Graph Generator for PolyRA Stress Testing

Generates graphs with configurable node/edge counts for testing
the memory-efficient graph operations.
"""

import os
import sys
import time
import random

DATA_DIR = "../data"

def generate_large_graph(name: str, graph_type: str, num_nodes: int, num_edges: int):
    """
    Generate a large sparse graph.
    
    Args:
        name: Graph name (e.g., "LARGE")
        graph_type: "D" for directed, "U" for undirected
        num_nodes: Number of nodes
        num_edges: Number of edges
    """
    suffix = graph_type
    node_file = f"{DATA_DIR}/{name} Nodes {suffix}.csv"
    edge_file = f"{DATA_DIR}/{name} Edges {suffix}.csv"
    
    print(f"Generating Large Graph: {name}")
    print(f"  Type: {'Directed' if graph_type == 'D' else 'Undirected'}")
    print(f"  Nodes: {num_nodes:,}")
    print(f"  Edges: {num_edges:,}")
    print(f"  Output: {node_file}, {edge_file}")
    print()
    
    start_time = time.time()
    
    # Generate Nodes
    print("Generating nodes...")
    with open(node_file, 'w') as f:
        f.write("NodeID,A1\n")
        for i in range(num_nodes):
            f.write(f"{i},{i % 2}\n")
            if i > 0 and i % 100000 == 0:
                elapsed = time.time() - start_time
                rate = i / elapsed
                eta = (num_nodes - i) / rate if rate > 0 else 0
                print(f"  Nodes: {i:,}/{num_nodes:,} ({i*100/num_nodes:.1f}%) - ETA: {eta:.0f}s")
    
    node_time = time.time() - start_time
    print(f"  Nodes generated in {node_time:.2f}s")
    
    # Generate Edges (using random sampling for sparse graphs)
    print("Generating edges...")
    edge_start = time.time()
    
    with open(edge_file, 'w') as f:
        f.write("Source_NodeID,Destination_NodeID,Weight,B1\n")
        
        edges_written = 0
        seen = set()
        attempts = 0
        max_attempts = num_edges * 20  # Limit to prevent infinite loops
        
        while edges_written < num_edges and attempts < max_attempts:
            src = random.randint(0, num_nodes - 1)
            dst = random.randint(0, num_nodes - 1)
            
            if src != dst and (src, dst) not in seen:
                weight = random.randint(1, 100)
                attr = random.randint(0, 1)
                f.write(f"{src},{dst},{weight},{attr}\n")
                seen.add((src, dst))
                edges_written += 1
                
                if edges_written % 100000 == 0:
                    elapsed = time.time() - edge_start
                    rate = edges_written / elapsed if elapsed > 0 else 1
                    eta = (num_edges - edges_written) / rate if rate > 0 else 0
                    print(f"  Edges: {edges_written:,}/{num_edges:,} ({edges_written*100/num_edges:.1f}%) - ETA: {eta:.0f}s")
            
            attempts += 1
    
        if edges_written < num_edges:
            print(f"  Warning: Only generated {edges_written:,} edges (target: {num_edges:,})")
    
    edge_time = time.time() - edge_start
    total_time = time.time() - start_time
    
    # Get file sizes
    node_size = os.path.getsize(node_file)
    edge_size = os.path.getsize(edge_file)
    
    print()
    print("Generation Complete!")
    print(f"  Node file: {node_size / (1024*1024):.2f} MB")
    print(f"  Edge file: {edge_size / (1024*1024):.2f} MB")
    print(f"  Total time: {total_time:.2f}s")
    
    return node_file, edge_file

def create_test_script(name: str, graph_type: str, num_nodes: int):
    """Create a test script for the large graph."""
    script = f"""LOAD GRAPH {name} {graph_type}
PRINT GRAPH {name}
DEGREE {name} 0
DEGREE {name} {num_nodes // 2}
DEGREE {name} {num_nodes - 1}
RES_{name} <- PATH {name} 0 {num_nodes // 2} WHERE B1(E) == 1
EXPORT GRAPH {name}
"""
    script_file = f"{DATA_DIR}/test_{name}_large.ra"
    with open(script_file, 'w') as f:
        f.write(script)
    print(f"Test script: {script_file}")
    return script_file

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python generate_large_graph.py <name> <type> <nodes> [edges]")
        print("  name:  Graph name (e.g., LARGE)")
        print("  type:  D for directed, U for undirected")
        print("  nodes: Number of nodes")
        print("  edges: Number of edges (default: 2 * nodes)")
        print()
        print("Examples:")
        print("  python generate_large_graph.py LARGE D 1000000 2000000")
        print("  python generate_large_graph.py MEDIUM D 100000 500000")
        sys.exit(1)
    
    name = sys.argv[1]
    graph_type = sys.argv[2].upper()
    num_nodes = int(sys.argv[3])
    num_edges = int(sys.argv[4]) if len(sys.argv) > 4 else num_nodes * 2
    
    if graph_type not in ["D", "U"]:
        print("Error: Type must be D or U")
        sys.exit(1)
    
    node_file, edge_file = generate_large_graph(name, graph_type, num_nodes, num_edges)
    script_file = create_test_script(name, graph_type, num_nodes)
