/*
 * ============================================================================
 *  PolyQuery - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#ifndef LOGGER_H
#define LOGGER_H

#include<iostream>
#include<bits/stdc++.h>
#include<sys/stat.h> 
#include<fstream>

using namespace std;

class Logger{

    string logFile = "log";
    ofstream fout;
    
    public:

    Logger();
    void log(string logString);
};

extern Logger logger;

#endif
