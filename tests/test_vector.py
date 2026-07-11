import random
import os

# Create data dir if not exists
os.makedirs("data", exist_ok=True)

# Generate a dense matrix 1000x50
D = 50
N = 1000

with open("data/test_matrix.csv", "w") as f:
    for i in range(N):
        row = [str(round(random.uniform(0, 1), 4)) for _ in range(D)]
        f.write(",".join(row) + "\n")

# Generate a test_vector.ra
query_vector_str = "[" + ", ".join([str(round(random.uniform(0, 1), 4)) for _ in range(D)]) + "]"

with open("data/test_vector.ra", "w") as f:
    f.write(f"LOAD VECTOR MATRIX test_matrix {D}\n")
    f.write(f"RES_EUCLIDEAN <- KNN test_matrix QUERY_VEC {query_vector_str} TOP 5 METRIC EUCLIDEAN\n")
    f.write(f"RES_COSINE <- KNN test_matrix QUERY_VEC {query_vector_str} TOP 5 METRIC COSINE\n")
    f.write("PRINT RES_EUCLIDEAN\n")
    f.write("PRINT RES_COSINE\n")
    f.write("QUIT\n")

print("Generated test_matrix.csv and test_vector.ra")
