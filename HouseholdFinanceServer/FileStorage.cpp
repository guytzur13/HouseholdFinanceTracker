#include "FileStorage.h"

#include <fstream>
#include <set>
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

// Parses a whole number in the range [1, max]. Unlike std::stoll alone, this rejects
// trailing garbage such as "12abc" and never throws. The upper limit matters: a file
// edited by hand or by another program could hold a huge number, and adding two such
// numbers would overflow (undefined behavior in C++).
bool parseNumberInRange(const std::string& text, long long max, long long& out) {
    try {
        size_t pos = 0;
        long long value = std::stoll(text, &pos);
        if (pos != text.size() || value <= 0 || value > max) return false;
        out = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// Spreadsheet programs (Excel, Numbers, Google Sheets) treat a cell that starts with
// = + - @ as a formula. If someone opens the data file in one of them, a description
// such as "=HYPERLINK(...)" would run as a formula ("CSV injection"). Prefixing such
// text with an apostrophe makes spreadsheets show it as plain text. The apostrophe is
// also escaped itself, so that unescape(escape(x)) == x for every x.
std::string escapeForCsv(const std::string& text) {
    if (!text.empty() && std::string("=+-@'\t").find(text[0]) != std::string::npos) {
        return "'" + text;
    }
    return text;
}

std::string unescapeFromCsv(const std::string& text) {
    if (text.size() > 1 && text[0] == '\'') return text.substr(1);
    return text;
}

bool parseTransactionLine(const std::string& line, Transaction& out) {
    std::vector<std::string> fields = splitCsvLine(line);
    if (fields.size() != 6) return false;

    long long id = 0;
    long long amount = 0;
    if (!parseNumberInRange(fields[0], kMaxTransactionId, id)) return false;
    if (!parseNumberInRange(fields[3], kMaxAmount, amount)) return false;
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
    out.category = unescapeFromCsv(fields[4]);
    out.description = unescapeFromCsv(fields[5]);
    return true;
}

// Writes `content` to `path` safely: write a temp file, then rename it over the original.
// std::filesystem::rename replaces an existing file on every platform; the C function
// std::rename does not on Windows, which would make every save after the first one fail.
bool writeFileAtomically(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::path tempPath = path;
    tempPath += ".tmp";
    {
        std::ofstream file(tempPath, std::ios::binary);
        if (!file) return false;
        file << content;
        if (!file) return false;
    }  // the file is closed (and flushed) here, when `file` goes out of scope

    // Personal financial data: only the owner may read or write the file
    // (on macOS and Linux; Windows handles access through the user's folder permissions).
    std::error_code error;
    std::filesystem::permissions(tempPath,
                                 std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::replace, error);
    error.clear();  // not being able to change permissions is not a reason to lose the save

    std::filesystem::rename(tempPath, path, error);
    return !error;
}

} // namespace

FileStorage::FileStorage(std::filesystem::path transactionsPath, std::filesystem::path budgetsPath)
    : transactionsPath_(std::move(transactionsPath)), budgetsPath_(std::move(budgetsPath)) {}

LoadResult FileStorage::loadTransactions(std::vector<Transaction>& out) const {
    LoadResult result;
    std::ifstream file(transactionsPath_);
    if (!file) return result;
    result.fileFound = true;

    out.clear();
    std::set<int> seenIds;
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line == kTransactionsHeader) continue;

        Transaction transaction;
        // A line is skipped if it is malformed, repeats an id (edit and delete look
        // transactions up by id, so ids must be unique), or exceeds the size limit.
        if (parseTransactionLine(line, transaction) && seenIds.insert(transaction.id).second &&
            out.size() < kMaxTransactions) {
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
                << escapeForCsv(sanitizeText(t.category)) << ","
                << escapeForCsv(sanitizeText(t.description)) << "\n";
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
        if (fields.size() == 2 && !fields[0].empty() &&
            parseNumberInRange(fields[1], kMaxAmount, amount)) {
            out[unescapeFromCsv(fields[0])] = amount;
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
        content << escapeForCsv(sanitizeText(category)) << "," << amount << "\n";
    }
    return writeFileAtomically(budgetsPath_, content.str());
}
