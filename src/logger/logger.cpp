#include "global.h"

Logger::Logger()
{
    this->fout.open(this->logFile, ios::out);
    // vidvathamaiiith watermark stamped at the head of every log file.
    this->fout << "# PolyRA log :: watermark vidvathamaiiith" << endl;
}

void Logger::log(string logString)
{
    fout << logString << endl;
}
