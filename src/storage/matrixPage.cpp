#include "global.h"

MatrixPage::MatrixPage() {
    this->matrixName = "";
    this->pageIndex = -1;
    this->dimension = 0;
    this->rowCount = 0;
}

MatrixPage::MatrixPage(string matrixName, int pageIndex, int dimension)
{
    logger.log("MatrixPage::MatrixPage");
    this->matrixName = matrixName;
    this->pageIndex = pageIndex;
    this->dimension = dimension;
    this->pageName = "data/temp/" + this->matrixName + "_Page" + to_string(pageIndex) + ".matrix";
    
    ifstream fin(this->pageName, ios::in | ios::binary);
    if (!fin) {
        // Can happen if page is empty or we are creating it
        this->rowCount = 0;
        return;
    }
    
    // Read number of rows in this page
    fin.read((char*)&this->rowCount, sizeof(int));
    
    for (int i = 0; i < this->rowCount; i++) {
        uint recordID;
        fin.read((char*)&recordID, sizeof(uint));
        vector<float> vec(this->dimension);
        fin.read((char*)vec.data(), this->dimension * sizeof(float));
        this->rows.push_back({recordID, vec});
    }
    
    fin.close();
}

void MatrixPage::writePage()
{
    logger.log("MatrixPage::writePage");
    ofstream fout(this->pageName, ios::out | ios::binary | ios::trunc);
    fout.write((char*)&this->rowCount, sizeof(int));
    for (int i = 0; i < this->rowCount; i++) {
        fout.write((char*)&this->rows[i].first, sizeof(uint));
        fout.write((char*)this->rows[i].second.data(), this->dimension * sizeof(float));
    }
    fout.close();
}
