#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ConsoleApp.h"
#include "FileStorage.h"
#include "FinanceManager.h"

namespace {

const std::string kTransactionsFile = "transactions.csv";
const std::string kBudgetsFile = "budgets.csv";

std::map<std::string, Cents> defaultBudgets() {
    return {
        {"אוכל", 2000 * 100},
        {"מגורים", 6000 * 100},
        {"בילויים", 1500 * 100},
        {"כללי", 1000 * 100},
    };
}

// Sample data for the very first run, so the reports have something to show.
void addSampleData(FinanceManager& manager) {
    using T = TransactionType;
    manager.addTransaction("משכורת א", 10000 * 100, "שכר", T::Income, "2026-10-01");
    manager.addTransaction("משכורת ב", 9000 * 100, "שכר", T::Income, "2026-10-01");
    manager.addTransaction("שכירות", 4500 * 100, "מגורים", T::Expense, "2026-10-02");
    manager.addTransaction("סופרמרקט", 1250 * 100, "אוכל", T::Expense, "2026-10-08");
    manager.addTransaction("משכורת א", 10000 * 100, "שכר", T::Income, "2026-11-01");
    manager.addTransaction("משכורת ב", 9000 * 100, "שכר", T::Income, "2026-11-01");
    manager.addTransaction("שכירות", 4500 * 100, "מגורים", T::Expense, "2026-11-02");
    manager.addTransaction("סופרמרקט", 2340 * 100 + 50, "אוכל", T::Expense, "2026-11-10");
}

} // namespace

int main() {
    FileStorage storage(kTransactionsFile, kBudgetsFile);
    FinanceManager manager;

    std::vector<Transaction> transactions;
    LoadResult transactionsResult = storage.loadTransactions(transactions);
    manager.loadTransactions(std::move(transactions));

    std::map<std::string, Cents> budgets;
    LoadResult budgetsResult = storage.loadBudgets(budgets);
    if (!budgetsResult.fileFound) {
        budgets = defaultBudgets();
        storage.saveBudgets(budgets);
    }
    manager.loadBudgets(std::move(budgets));

    int skipped = transactionsResult.skippedLines + budgetsResult.skippedLines;
    if (skipped > 0) {
        std::cout << "[!] אזהרה: דולגו " << skipped << " שורות פגומות בקבצי הנתונים." << std::endl;
    }

    if (!transactionsResult.fileFound) {
        addSampleData(manager);
        storage.saveTransactions(manager.transactions());
        std::cout << "הרצה ראשונה: נוצרו נתוני דוגמה." << std::endl;
    }

    ConsoleApp app(manager, storage);
    app.run();
    return 0;
}
