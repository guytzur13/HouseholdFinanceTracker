#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include <map>
#include <string>
#include <vector>

#include "Transaction.h"

// Information about a load operation, so the caller can tell the user what happened.
struct LoadResult {
    bool fileFound = false;
    int skippedLines = 0;   // malformed lines that were ignored
};

// Saves and loads the data as simple CSV text files.
//
// transactions.csv:  id,date,type,amount_agorot,category,description
// budgets.csv:       category,amount_agorot
//
// Saving is "atomic": data is first written to a temporary file, which then
// replaces the real file in one step. If the program crashes in the middle
// of writing, the previous data file is still intact.
class FileStorage {
public:
    FileStorage(std::string transactionsPath, std::string budgetsPath);

    LoadResult loadTransactions(std::vector<Transaction>& out) const;
    bool saveTransactions(const std::vector<Transaction>& transactions) const;

    LoadResult loadBudgets(std::map<std::string, Cents>& out) const;
    bool saveBudgets(const std::map<std::string, Cents>& budgets) const;

private:
    std::string transactionsPath_;
    std::string budgetsPath_;
};

#endif // FILE_STORAGE_H
