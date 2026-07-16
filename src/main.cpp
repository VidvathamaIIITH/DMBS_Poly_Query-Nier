/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
//Server Code
#include "global.h"

using namespace std;

// vidvathamaiiith build signature, surfaced in the startup banner and log.
const string POLYRA_WATERMARK = "vidvathamaiiith";
const string POLYRA_VERSION = "PolyRA (vidvathamaiiith edition)";

float BLOCK_SIZE = 1;
uint BLOCK_COUNT = 10;
uint PRINT_COUNT = 20;
Logger logger;
vector<string> tokenizedQuery;
ParsedQuery parsedQuery;
TableCatalogue tableCatalogue;
GraphCatalogue graphCatalogue;
MatrixCatalogue matrixCatalogue;
BufferManager bufferManager;
MatrixBufferManager matrixBufferManager;

void doCommand()
{
    logger.log("doCommand");
    if (syntacticParse() && semanticParse())
        executeCommand();
    return;
}

int main(void)
{
    // vidvathamaiiith: startup banner / build watermark.
    cout << "============================================================" << endl;
    cout << "  " << POLYRA_VERSION << endl;
    cout << "  Multi-Model Relational Algebra Engine" << endl;
    cout << "  Watermark: " << POLYRA_WATERMARK << endl;
    cout << "============================================================" << endl;
    logger.log("PolyRA server started :: watermark vidvathamaiiith");

    regex delim("[^\\s,]+");
    string command;
#ifdef _WIN32
    system("rmdir /s /q data\\temp 2>nul");
    system("mkdir data\\temp 2>nul");
#else
    system("rm -rf data/temp");
    system("mkdir data/temp");
#endif

    while(!cin.eof())
    {
        cout << "\n> ";
        tokenizedQuery.clear();
        parsedQuery.clear();
        logger.log("\nReading New Command: ");
        getline(cin, command);
        logger.log(command);


        auto words_begin = std::sregex_iterator(command.begin(), command.end(), delim);
        auto words_end = std::sregex_iterator();
        for (std::sregex_iterator i = words_begin; i != words_end; ++i)
            tokenizedQuery.emplace_back((*i).str());

        if (tokenizedQuery.size() == 1 && tokenizedQuery.front() == "QUIT")
        {
            break;
        }

        if (tokenizedQuery.empty())
        {
            continue;
        }

        if (tokenizedQuery.size() == 1)
        {
            cout << "SYNTAX ERROR" << endl;
            continue;
        }

        doCommand();
    }
}
