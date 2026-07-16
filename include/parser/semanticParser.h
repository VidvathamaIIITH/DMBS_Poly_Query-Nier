#ifndef SEMANTICPARSER_H
#define SEMANTICPARSER_H

#include"syntacticParser.h"

bool semanticParse();

bool semanticParseTRANSACTION();
bool semanticParseSETBUFFER();
bool semanticParseCLEAR();
bool semanticParseCROSS();
bool semanticParseDISTINCT();
bool semanticParseEXPORT();
bool semanticParseINDEX();
bool semanticParseJOIN();
bool semanticParseLIST();
bool semanticParseLOAD();
bool semanticParsePRINT();
bool semanticParsePROJECTION();
bool semanticParseRENAME();
bool semanticParseSELECTION();
bool semanticParseSORT();
bool semanticParseSOURCE();

bool semanticParseLOAD_GRAPH();
bool semanticParsePRINT_GRAPH();
bool semanticParseEXPORT_GRAPH();
bool semanticParseDEGREE();
bool semanticParsePATH();
bool semanticParseGROUP();

bool semanticParseLOAD_MATRIX();
bool semanticParseKNN();

// vidvathamaiiith extension semantic parsers
bool semanticParseDESCRIBE();
bool semanticParseCHECKSUM();

#endif
