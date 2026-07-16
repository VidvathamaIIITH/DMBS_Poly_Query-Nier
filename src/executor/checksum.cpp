/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark: vidvathamaiiith
 * ============================================================================
 *
 *  vidvathamaiiith extension: the CHECKSUM command.
 *
 *  CHECKSUM streams a table through its block cursor and folds every cell into a
 *  deterministic 64-bit fingerprint using the FNV-1a hash. The computation is
 *  fully out-of-core: rows are consumed one block at a time through the existing
 *  buffer manager, so the fingerprint of an arbitrarily large relation is
 *  produced without ever materialising it in memory.
 *
 *  Because the hash is order-sensitive (the row ordinal and column ordinal are
 *  mixed into the state), two relations produce the same checksum only when they
 *  are byte-for-byte identical in both content and row order. This makes
 *  CHECKSUM a cheap integrity / equality probe: verify a JOIN result against a
 *  reference, confirm an EXPORT round-trip, or detect silent corruption.
 *
 *  SYNTAX: CHECKSUM <table_name>
 */

#include "global.h"

namespace
{
    const unsigned long long FNV_OFFSET_BASIS = 1469598103934665603ULL;
    const unsigned long long FNV_PRIME = 1099511628211ULL;

    inline void foldValue(unsigned long long &state, long long value)
    {
        // Feed the eight little-endian bytes of the value through FNV-1a.
        unsigned long long bits = (unsigned long long)value;
        for (int byteIndex = 0; byteIndex < 8; byteIndex++)
        {
            unsigned char octet = (unsigned char)(bits & 0xFF);
            state ^= (unsigned long long)octet;
            state *= FNV_PRIME;
            bits >>= 8;
        }
    }
}

bool syntacticParseCHECKSUM()
{
    logger.log("syntacticParseCHECKSUM");
    if (tokenizedQuery.size() != 2)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = CHECKSUM;
    parsedQuery.checksumRelationName = tokenizedQuery[1];
    return true;
}

bool semanticParseCHECKSUM()
{
    logger.log("semanticParseCHECKSUM");
    if (!tableCatalogue.isTable(parsedQuery.checksumRelationName))
    {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }
    return true;
}

void executeCHECKSUM()
{
    logger.log("executeCHECKSUM");
    Table table = *tableCatalogue.getTable(parsedQuery.checksumRelationName);

    unsigned long long state = FNV_OFFSET_BASIS;
    // Seed with the schema shape so tables of different arity never collide.
    foldValue(state, (long long)table.columnCount);

    Cursor cursor = table.getCursor();
    vector<int> row = cursor.getNext();
    long long rowOrdinal = 0;
    while (!row.empty())
    {
        foldValue(state, rowOrdinal);
        for (size_t columnIndex = 0; columnIndex < row.size(); columnIndex++)
        {
            foldValue(state, (long long)columnIndex);
            foldValue(state, (long long)row[columnIndex]);
        }
        rowOrdinal++;
        row = cursor.getNext();
    }

    // Emit as fixed-width, zero-padded hex for stable diffing across runs.
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%016llX", state);
    cout << "\nCHECKSUM " << parsedQuery.checksumRelationName << " : 0x" << buffer << endl;
    cout << "Rows Hashed: " << rowOrdinal << endl;
    return;
}
