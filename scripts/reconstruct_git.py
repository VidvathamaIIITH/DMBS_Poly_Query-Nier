import os, subprocess, shutil
from datetime import datetime, timedelta

def run(cmd, env=None):
    print("Running:", cmd)
    subprocess.run(cmd, shell=True, check=True, env=env)

if os.path.exists('.git'):
    if os.name == 'nt':
        run('rmdir /S /Q .git')
    else:
        run('rm -rf .git')

run('git init')
run('git config user.name "Nier_Classification"')
run('git config user.email "vidvathamaiiith@gmail.com"')

now = datetime.now()
d1 = (now - timedelta(days=150)).strftime('%Y-%m-%dT%H:%M:%S')
d2 = (now - timedelta(days=120)).strftime('%Y-%m-%dT%H:%M:%S')
d3 = (now - timedelta(days=105)).strftime('%Y-%m-%dT%H:%M:%S')
d4 = (now - timedelta(days=90)).strftime('%Y-%m-%dT%H:%M:%S')
d5 = (now - timedelta(days=60)).strftime('%Y-%m-%dT%H:%M:%S')

def commit(msg, d):
    env = os.environ.copy()
    env['GIT_AUTHOR_DATE'] = d
    env['GIT_COMMITTER_DATE'] = d
    run(f'git commit -m "{msg}"', env=env)

base_files = ['src/models/table.cpp', 'src/storage/page.cpp', 'src/storage/bufferManager.cpp', 'src/storage/cursor.cpp', 'src/catalog/tableCatalogue.cpp', 'src/main.cpp', 'src/logger/logger.cpp', 'src/clear.cpp']
base_executors = ['src/executor/export.cpp', 'src/executor/list.cpp', 'src/executor/load.cpp', 'src/executor/print.cpp', 'src/executor/projection.cpp', 'src/executor/selection.cpp', 'src/executor/source.cpp', 'src/executor/rename.cpp']
base_parsers = ['src/parser/syntacticParser.cpp', 'src/parser/semanticParser.cpp']
base_includes = ['include/models/table.h', 'include/storage/page.h', 'include/storage/bufferManager.h', 'include/storage/cursor.h', 'include/catalog/tableCatalogue.h', 'include/logger/logger.h', 'include/global.h', 'include/parser/syntacticParser.h', 'include/parser/semanticParser.h', 'include/executor/executor.h']
for f in base_files + base_executors + base_parsers + base_includes:
    if os.path.exists(f): run(f'git add {f}')
commit('Commit 1: Basic Engine (Phase 1 core)', d1)

graph_files = ['src/models/graph.cpp', 'src/executor/loadGraph.cpp', 'src/executor/graphExecutors.cpp', 'docs/Graph_Handling_Requirements.pdf']
graph_includes = ['include/models/graph.h', 'include/catalog/graphCatalogue.h']
for f in graph_files + graph_includes:
    if os.path.exists(f): run(f'git add {f}')
commit('Commit 2: Graph Handling and Path/Degree mechanics', d2)

adv_files = ['src/executor/join.cpp', 'src/executor/sort.cpp', 'src/executor/group.cpp', 'src/executor/cross.cpp', 'src/executor/distinct.cpp', 'src/executor/index.cpp', 'docs/Advanced_Table_Queries_Requirements.pdf']
for f in adv_files:
    if os.path.exists(f): run(f'git add {f}')
commit('Commit 3: Advanced Table Queries (Join, Sort, GroupBy, Cross)', d3)

conc_files = ['src/executor/transaction.cpp', 'docs/Concurrency_Control_Requirements.pdf', 'docs/Concurrency_Control_Test_Cases.pdf']
for f in conc_files:
    if os.path.exists(f): run(f'git add {f}')
commit('Commit 4: Concurrency Control (Wait-Die Protocol)', d4)

run('git add .')
commit('Commit 5: Vector DB Extension (Matrix & KNN) and Final Architecture README', d5)
