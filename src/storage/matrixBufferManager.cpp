#include "global.h"

MatrixBufferManager::MatrixBufferManager()
{
    logger.log("MatrixBufferManager::MatrixBufferManager");
}

bool MatrixBufferManager::inPool(string pageName)
{
    logger.log("MatrixBufferManager::inPool");
    for (auto& page : this->pages)
    {
        if (page.pageName == pageName)
            return true;
    }
    return false;
}

MatrixPage MatrixBufferManager::getFromPool(string pageName)
{
    logger.log("MatrixBufferManager::getFromPool");
    for (int i = 0; i < this->pages.size(); i++)
    {
        if (this->pages[i].pageName == pageName)
        {
            MatrixPage page = this->pages[i];
            // If not pinned, move to back to signify recent use (LRU). 
            // The original buffer manager did FIFO. Let's do FIFO for unpinned by just keeping it there, 
            // or we can remove it and push it to the back for LRU.
            this->pages.erase(this->pages.begin() + i);
            this->pages.push_back(page);
            return page;
        }
    }
    return MatrixPage();
}

MatrixPage MatrixBufferManager::insertIntoPool(string matrixName, int pageIndex, int dimension)
{
    logger.log("MatrixBufferManager::insertIntoPool");
    MatrixPage page(matrixName, pageIndex, dimension);
    
    // Eviction
    while (this->pages.size() >= BLOCK_COUNT) {
        bool evicted = false;
        // Find the oldest unpinned page
        for (auto it = this->pages.begin(); it != this->pages.end(); ++it) {
            if (this->pinnedPages.find(it->pageName) == this->pinnedPages.end()) {
                it->writePage(); // Ensure it's written back
                this->pages.erase(it);
                evicted = true;
                break;
            }
        }
        
        if (!evicted) {
            // All pages are pinned! Cannot evict. Break to avoid infinite loop.
            // This is a severe boundary condition.
            cout << "WARNING: Buffer Pool Full and all pages pinned!" << endl;
            break;
        }
    }
    
    this->pages.push_back(page);
    return page;
}

MatrixPage MatrixBufferManager::getPage(string matrixName, int pageIndex, int dimension)
{
    logger.log("MatrixBufferManager::getPage");
    string pageName = "data/temp/" + matrixName + "_Page" + to_string(pageIndex) + ".matrix";
    if (this->inPool(pageName)) {
        return this->getFromPool(pageName);
    } else {
        return this->insertIntoPool(matrixName, pageIndex, dimension);
    }
}

void MatrixBufferManager::pinPage(string matrixName, int pageIndex)
{
    string pageName = "data/temp/" + matrixName + "_Page" + to_string(pageIndex) + ".matrix";
    this->pinnedPages.insert(pageName);
}

void MatrixBufferManager::unpinPage(string matrixName, int pageIndex)
{
    string pageName = "data/temp/" + matrixName + "_Page" + to_string(pageIndex) + ".matrix";
    this->pinnedPages.erase(pageName);
}

void MatrixBufferManager::writePage(string pageName, const vector<pair<uint, vector<float>>>& rows)
{
    // Write directly or update pool
    for (auto& page : this->pages) {
        if (page.pageName == pageName) {
            page.getRows() = rows;
            page.writePage();
            return;
        }
    }
    // If not in pool, just error or ignore
}

void MatrixBufferManager::writePage(string matrixName, int pageIndex, int dimension, const vector<pair<uint, vector<float>>>& rows, int rowCount)
{
    logger.log("MatrixBufferManager::writePage");
    MatrixPage page(matrixName, pageIndex, dimension);
    page.getRows() = rows;
    page.writePage();
}

void MatrixBufferManager::deleteFile(string matrixName, int pageIndex)
{
    logger.log("MatrixBufferManager::deleteFile");
    string pageName = "data/temp/" + matrixName + "_Page" + to_string(pageIndex) + ".matrix";
    remove(pageName.c_str());
}

void MatrixBufferManager::deleteFile(string fileName)
{
    logger.log("MatrixBufferManager::deleteFile");
    string pageName = "data/temp/" + fileName;
    remove(pageName.c_str());
}
