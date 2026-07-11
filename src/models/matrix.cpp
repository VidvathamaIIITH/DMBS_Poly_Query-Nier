#include "global.h"
#include <cmath>
#include <sys/stat.h>

Matrix::Matrix(string matrixName, int dimension)
{
    logger.log("Matrix::Matrix");
    this->matrixName = matrixName;
    this->dimension = dimension;
    this->sourceFileName = "data/" + matrixName + ".csv";
}

bool Matrix::blockify()
{
    logger.log("Matrix::blockify");
    ifstream fin(this->sourceFileName, ios::in);
    if (!fin) {
        cout << "Error: File not found: " << this->sourceFileName << endl;
        return false;
    }

    string line, word;
    this->rowCount = 0;
    
    int slotSize = 4 + (this->dimension * 4);
    int pageSizeBytes = BLOCK_SIZE * 1024;
    // We reserve sizeof(int) bytes for the rowCount in the page header
    this->maxRowsPerBlock = (pageSizeBytes - sizeof(int)) / slotSize;
    if (this->maxRowsPerBlock == 0) this->maxRowsPerBlock = 1; // Fallback
    
    uint currentRecordID = 0;
    int pageIndex = 0;
    
    while (true)
    {
        vector<pair<uint, vector<float>>> pageRows;
        
        for (int i = 0; i < this->maxRowsPerBlock; i++)
        {
            if (!getline(fin, line)) break;
            
            stringstream s(line);
            vector<float> row;
            for (int d = 0; d < this->dimension; d++)
            {
                if (!getline(s, word, ',')) word = "0.0";
                row.push_back(stof(word));
            }
            pageRows.push_back({currentRecordID++, row});
            this->rowCount++;
        }
        
        if (pageRows.empty()) break;
        
        this->rowsPerBlockCount.push_back(pageRows.size());
        
        // Write the page
        string pageName = "data/temp/" + this->matrixName + "_Page" + to_string(pageIndex) + ".matrix";
        ofstream fout(pageName, ios::out | ios::binary | ios::trunc);
        int rowsInThisPage = pageRows.size();
        fout.write((char*)&rowsInThisPage, sizeof(int));
        for (auto& pr : pageRows) {
            fout.write((char*)&pr.first, sizeof(uint));
            fout.write((char*)pr.second.data(), this->dimension * sizeof(float));
        }
        fout.close();
        
        pageIndex++;
    }
    fin.close();
    
    this->blockCount = pageIndex;

    return true;
}

bool Matrix::load()
{
    logger.log("Matrix::load");
    if (this->blockify()) {
        return true;
    }
    return false;
}

void Matrix::unload()
{
    logger.log("Matrix::unload");
    string matrixFilePath = "data/temp/" + this->matrixName + ".matrix";
    remove(matrixFilePath.c_str());
}

void Matrix::makePermanent()
{
    logger.log("Matrix::makePermanent");
    // Matrix is not modified in-place typically, or if it is, we would copy it to data/
    // Not strictly required for basic LOAD.
}

bool Matrix::isPermanent()
{
    logger.log("Matrix::isPermanent");
    return true; // Not implemented for now
}
