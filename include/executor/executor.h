#ifndef EXECUTOR_H
#define EXECUTOR_H

#include"semanticParser.h"

void executeCommand();

void executeTRANSACTION();
void executeSETBUFFER();
void executeCLEAR();
void executeCROSS();
void executeDISTINCT();
void executeEXPORT();
void executeINDEX();
void executeJOIN();
void executeLIST();
void executeLOAD();
void executePRINT();
void executePROJECTION();
void executeRENAME();
void executeSELECTION();
void executeSORT();
void executeSOURCE();

void executeLOAD_GRAPH();
void executePRINT_GRAPH();
void executeEXPORT_GRAPH();
void executeDEGREE();
void executePATH();
void executeGROUP();

void executeLOAD_MATRIX();
void executeKNN();

// vidvathamaiiith extension executors
void executeDESCRIBE();
void executeCHECKSUM();

bool evaluateBinOp(int value1, int value2, BinaryOperator binaryOperator);
void printRowCount(int rowCount);

#endif
