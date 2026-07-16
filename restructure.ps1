$ErrorActionPreference = 'Stop'
cd C:\Users\vidva\OneDrive\Desktop\IIITH\Projects\nier_classification\DMBS_Poly_RA

mkdir docs -Force
mkdir scripts -Force

mkdir include/catalog -Force
mkdir include/executor -Force
mkdir include/logger -Force
mkdir include/models -Force
mkdir include/parser -Force
mkdir include/storage -Force

mkdir src/catalog -Force
mkdir src/executor -Force
mkdir src/logger -Force
mkdir src/models -Force
mkdir src/parser -Force
mkdir src/storage -Force

mv SimpleRA_Phase1.pdf docs/Graph_Handling_Requirements.pdf
mv DS26_Project_Phase2.pdf docs/Advanced_Table_Queries_Requirements.pdf
mv DS26_Project_Phase3.pdf docs/Concurrency_Control_Requirements.pdf
mv "Phase 3 Sample Test Cases.pdf" docs/Concurrency_Control_Test_Cases.pdf
mv phase1_requirements.txt docs/

mv src/generate_large_graph.py scripts/
mv src/test_generator.py scripts/
mv src/verify_order.py scripts/
mv src/run_tests.sh scripts/
mv src/phase3_verify_commands.txt scripts/
mv src/phase3_error_verify_commands.txt scripts/

mv src/tableCatalogue.h include/catalog/
mv src/graphCatalogue.h include/catalog/

mv src/executor.h include/executor/

mv src/logger.h include/logger/

mv src/table.h include/models/
mv src/graph.h include/models/

mv src/syntacticParser.h include/parser/
mv src/semanticParser.h include/parser/

mv src/bufferManager.h include/storage/
mv src/cursor.h include/storage/
mv src/page.h include/storage/

mv src/global.h include/

mv src/tableCatalogue.cpp src/catalog/

mv src/executor.cpp src/executor/
mv src/executors/* src/executor/

mv src/logger.cpp src/logger/

mv src/table.cpp src/models/
mv src/graph.cpp src/models/

mv src/syntacticParser.cpp src/parser/
mv src/semanticParser.cpp src/parser/

mv src/bufferManager.cpp src/storage/
mv src/cursor.cpp src/storage/
mv src/page.cpp src/storage/

mv src/server.cpp src/main.cpp

rmdir src/executors
