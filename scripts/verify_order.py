import sys
import csv

def verify_order(graph_name):
    # Read original CSVs
    nodes_csv = f"../data/{graph_name} Nodes D.csv"
    edges_csv = f"../data/{graph_name} Edges D.csv"
    
    server_output = sys.stdin.read()
    lines = server_output.strip().split('\n')
    
    # Find start of print output
    # Look for "Loaded Graph" then "Node Count"
    # Or simplified: check if the node/edge lines appear in order
    
    # Parse CSVs
    original_nodes = []
    with open(nodes_csv, 'r') as f:
        reader = csv.reader(f)
        header = next(reader)
        for row in reader:
            original_nodes.append(", ".join(row)) # space after comma match server output format?
            
    original_edges = []
    with open(edges_csv, 'r') as f:
        reader = csv.reader(f)
        header = next(reader)
        for row in reader:
             original_edges.append(", ".join(row))

    # Parse server output to find printed nodes and edges
    # Extract lines that look like data
    # This is tricky because server output mixes logs and print output
    # But usually PRINT output is clean lines of numbers
    
    # Simple check: sequence of lines in output must match sequence in CSV
    
    def find_sequence(lines, sequence, name):
        match_idx = 0
        for line in lines:
            # Server output might have different spacing
            # Normalize spaces
            clean_line = ",".join([x.strip() for x in line.split(',')])
            clean_target = ",".join([x.strip() for x in sequence[match_idx].split(',')])
            
            if clean_line == clean_target:
                match_idx += 1
                if match_idx == len(sequence):
                    return True
        return False

    nodes_found = find_sequence(lines, original_nodes, "Nodes")
    # For edges, finding sequence is harder if there are many log lines in between (unlikely for PRINT)
    edges_found = find_sequence(lines, original_edges, "Edges")
    
    if nodes_found and edges_found:
        print("ORDER_VERIFIED")
    else:
        print("ORDER_FAILED")
        if not nodes_found: print("Nodes order mismatch")
        if not edges_found: print("Edges order mismatch")

if __name__ == "__main__":
    verify_order("T26")
