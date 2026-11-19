#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include <filesystem>
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
//
// The files may come from another computer (through a synced folder), so loading
// treats them as untrusted input: every field is validated and range-checked, and
// a bad line is skipped instead of being trusted.
class FileStorage {
public:
    // std::filesystem::path (not std::string) so that folder names with non-English
    // characters work on Windows too, where paths are stored as wide strings.
    FileStorage(std::filesystem::path transactionsPath, std::filesystem::path budgetsPath);

    LoadResult loadTransactions(std::vector<Transaction>& out) const;
    bool saveTransactions(const std::vector<Transaction>& transactions) const;

    LoadResult loadBudgets(std::map<std::string, Cents>& out) const;
    bool saveBudgets(const std::map<std::string, Cents>& budgets) const;

private:
    std::filesystem::path transactionsPath_;
    std::filesystem::path budgetsPath_;
};

#endif // FILE_STORAGE_H
