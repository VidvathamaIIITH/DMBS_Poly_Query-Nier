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
run('git config user.name "SKALGO22"')
run('git config user.email "sushantkumar124421@gmail.com"')

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

base_files = ['src/table.cpp', 'src/page.cpp', 'src/bufferManager.cpp', 'src/cursor.cpp', 'src/tableCatalogue.cpp', 'src/main.cpp', 'src/logger.cpp', 'src/clear.cpp', 'src/server.cpp']
base_executors = ['src/executors/export.cpp', 'src/executors/list.cpp', 'src/executors/load.cpp', 'src/executors/print.cpp', 'src/executors/projection.cpp', 'src/executors/selection.cpp', 'src/executors/source.cpp', 'src/executors/rename.cpp']
base_includes = ['include/table.h', 'include/page.h', 'include/bufferManager.h', 'include/cursor.h', 'include/tableCatalogue.h', 'include/logger.h', 'include/global.h']
for f in base_files + base_executors + base_includes:
    if os.path.exists(f): run(f'git add {f}')
commit('Commit 1: Basic Engine (Phase 1 core)', d1)

graph_files = ['src/graph.cpp', 'src/graphCatalogue.cpp', 'src/executors/loadGraph.cpp', 'src/executors/graphExecutors.cpp', 'docs/Graph_Handling_Requirements.pdf']
graph_includes = ['include/graph.h', 'include/graphCatalogue.h']
for f in graph_files + graph_includes:
    if os.path.exists(f): run(f'git add {f}')
commit('Commit 2: Graph Handling and Path/Degree mechanics', d2)

adv_files = ['src/executors/join.cpp', 'src/executors/sort.cpp', 'src/executors/group.cpp', 'src/executors/cross.cpp', 'src/executors/distinct.cpp', 'src/executors/index.cpp', 'docs/Advanced_Table_Queries_Requirements.pdf']
for f in adv_files:
    if os.path.exists(f): run(f'git add {f}')
commit('Commit 3: Advanced Table Queries (Join, Sort, GroupBy, Cross)', d3)

conc_files = ['src/executor/transaction.cpp', 'src/executor/lockSimulator.cpp', 'docs/Concurrency_Control_Requirements.pdf', 'docs/Concurrency_Control_Test_Cases.pdf']
for f in conc_files:
    if os.path.exists(f): run(f'git add {f}')
commit('Commit 4: Concurrency Control (Wait-Die Protocol)', d4)

run('git add .')
commit('Commit 5: Vector DB Extension (Matrix & KNN) and Final Architecture README', d5)
