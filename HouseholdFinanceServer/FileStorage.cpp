#include "FileStorage.h"

#include <cstdio>   // std::rename
#include <fstream>
#include <sstream>
#include <utility>

#include "Validation.h"

namespace {

const char* kTransactionsHeader = "id,date,type,amount_agorot,category,description";
const char* kBudgetsHeader = "category,amount_agorot";

// Splits a CSV line on commas. Free-text fields never contain commas,
// because sanitizeText() removes them before data is stored.
std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) {
        fields.push_back(trim(field));
    }
    return fields;
}

// Parses a whole positive number. Unlike std::stoll alone, this rejects
// trailing garbage such as "12abc" and never throws.
bool parsePositiveNumber(const std::string& text, long long& out) {
    try {
        size_t pos = 0;
        long long value = std::stoll(text, &pos);
        if (pos != text.size() || value <= 0) return false;
        out = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parseTransactionLine(const std::string& line, Transaction& out) {
    std::vector<std::string> fields = splitCsvLine(line);
    if (fields.size() != 6) return false;

    long long id = 0;
    long long amount = 0;
    if (!parsePositiveNumber(fields[0], id)) return false;
    if (!parsePositiveNumber(fields[3], amount)) return false;
    if (!isValidDate(fields[1])) return false;

    if (fields[2] == "expense") {
        out.type = TransactionType::Expense;
    } else if (fields[2] == "income") {
        out.type = TransactionType::Income;
    } else {
        return false;
    }

    if (fields[4].empty() || fields[5].empty()) return false;

    out.id = static_cast<int>(id);
    out.date = fields[1];
    out.amount = amount;
    out.category = fields[4];
    out.description = fields[5];
    return true;
}

// Writes `content` to `path` safely: write a temp file, then rename it over the original.
bool writeFileAtomically(const std::string& path, const std::string& content) {
    std::string tempPath = path + ".tmp";
    {
        std::ofstream file(tempPath);
        if (!file) return false;
        file << content;
        if (!file) return false;
    }  // the file is closed (and flushed) here, when `file` goes out of scope
    return std::rename(tempPath.c_str(), path.c_str()) == 0;
}

} // namespace

FileStorage::FileStorage(std::string transactionsPath, std::string budgetsPath)
    : transactionsPath_(std::move(transactionsPath)), budgetsPath_(std::move(budgetsPath)) {}

LoadResult FileStorage::loadTransactions(std::vector<Transaction>& out) const {
    LoadResult result;
    std::ifstream file(transactionsPath_);
    if (!file) return result;
    result.fileFound = true;

    out.clear();
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line == kTransactionsHeader) continue;

        Transaction transaction;
        if (parseTransactionLine(line, transaction)) {
            out.push_back(transaction);
        } else {
            result.skippedLines++;  // a corrupted line should not crash the app or lose the rest
        }
    }
    return result;
}

bool FileStorage::saveTransactions(const std::vector<Transaction>& transactions) const {
    std::ostringstream content;
    content << kTransactionsHeader << "\n";
    for (const Transaction& t : transactions) {
        content << t.id << ","
                << t.date << ","
                << (t.isExpense() ? "expense" : "income") << ","
                << t.amount << ","
                << sanitizeText(t.category) << ","
                << sanitizeText(t.description) << "\n";
    }
    return writeFileAtomically(transactionsPath_, content.str());
}

LoadResult FileStorage::loadBudgets(std::map<std::string, Cents>& out) const {
    LoadResult result;
    std::ifstream file(budgetsPath_);
    if (!file) return result;
    result.fileFound = true;

    out.clear();
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line == kBudgetsHeader) continue;

        std::vector<std::string> fields = splitCsvLine(line);
        long long amount = 0;
        if (fields.size() == 2 && !fields[0].empty() && parsePositiveNumber(fields[1], amount)) {
            out[fields[0]] = amount;
        } else {
            result.skippedLines++;
        }
    }
    return result;
}

bool FileStorage::saveBudgets(const std::map<std::string, Cents>& budgets) const {
    std::ostringstream content;
    content << kBudgetsHeader << "\n";
    for (const auto& [category, amount] : budgets) {
        content << sanitizeText(category) << "," << amount << "\n";
    }
    return writeFileAtomically(budgetsPath_, content.str());
}
