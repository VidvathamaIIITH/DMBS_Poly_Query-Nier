#include"global.h"

void executeCommand(){

    switch(parsedQuery.queryType){
        case TRANSACTION: executeTRANSACTION(); break;
        case SETBUFFER: executeSETBUFFER(); break;
        case CLEAR: executeCLEAR(); break;
        case CROSS: executeCROSS(); break;
        case DISTINCT: executeDISTINCT(); break;
        case EXPORT: executeEXPORT(); break;
        case INDEX: executeINDEX(); break;
        case JOIN: executeJOIN(); break;
        case LIST: executeLIST(); break;
        case LOAD: executeLOAD(); break;
        case PRINT: executePRINT(); break;
        case PROJECTION: executePROJECTION(); break;
        case RENAME: executeRENAME(); break;
        case SELECTION: executeSELECTION(); break;
        case SORT: executeSORT(); break;
        case SOURCE: executeSOURCE(); break;
        case LOAD_GRAPH: executeLOAD_GRAPH(); break;
        case PRINT_GRAPH: executePRINT_GRAPH(); break;
        case EXPORT_GRAPH: executeEXPORT_GRAPH(); break;
        case DEGREE: executeDEGREE(); break;
        case PATH: executePATH(); break;
        case GROUP: executeGROUP(); break;
        case LOAD_MATRIX: executeLOAD_MATRIX(); break;
        case KNN: executeKNN(); break;
        default: cout<<"PARSING ERROR"<<endl;
    }

    return;
}

void printRowCount(int rowCount){
    cout<<"\n\nRow Count: "<<rowCount<<endl;
    return;
}
