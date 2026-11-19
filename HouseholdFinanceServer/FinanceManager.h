#ifndef FINANCE_MANAGER_H
#define FINANCE_MANAGER_H

#include <map>
#include <string>
#include <vector>

#include "Transaction.h"

// Totals for one month.
struct MonthlySummary {
    std::string month;                          // YYYY-MM
    Cents income = 0;
    Cents expenses = 0;
    std::map<std::string, Cents> spentByCategory;

    Cents net() const { return income - expenses; }
};

// How much was spent in a category this month compared to its budget.
struct BudgetStatus {
    std::string category;
    Cents budget = 0;
    Cents spent = 0;

    bool isOverBudget() const { return spent > budget; }
};

// Result of the savings-goal calculation.
struct SavingsForecast {
    bool reachable = false;          // false if the household does not save money on average
    Cents averageMonthlySavings = 0;
    int monthsOfData = 0;            // how many months the average is based on
    int monthsNeeded = 0;            // only meaningful when reachable is true
};

// The core business logic of the application.
//
// This class deliberately does no input/output (no cin/cout, no files).
// That separation means it can be unit tested on its own, and the same logic
// could later sit behind a different interface (a GUI or a web server)
// without changing this class.
class FinanceManager {
public:
    // Adds a transaction and returns its new id.
    // Throws std::invalid_argument if any field is invalid.
    int addTransaction(const std::string& description, Cents amount,
                       const std::string& category, TransactionType type,
                       const std::string& date);

    // Replaces the transaction that has the same id. Returns false if no such id.
    // Throws std::invalid_argument if any field is invalid.
    bool updateTransaction(const Transaction& updated);

    // Returns false if no transaction has this id.
    bool removeTransaction(int id);

    // Returns nullptr if no transaction has this id.
    const Transaction* findTransaction(int id) const;

    const std::vector<Transaction>& transactions() const { return transactions_; }

    // Transactions of one month (YYYY-MM), sorted by date.
    std::vector<Transaction> transactionsForMonth(const std::string& month) const;

    // All months that have at least one transaction, oldest first.
    std::vector<std::string> months() const;

    MonthlySummary summarizeMonth(const std::string& month) const;

    // Spending against budget for every budgeted category in the given month.
    std::vector<BudgetStatus> budgetStatus(const std::string& month) const;

    bool isOverBudget(const std::string& category, const std::string& month) const;

    // Sets the monthly budget of a category. Throws std::invalid_argument if amount <= 0.
    void setBudget(const std::string& category, Cents amount);
    const std::map<std::string, Cents>& budgets() const { return budgets_; }

    // Estimates how many months it takes to save `goal`, based on the
    // average net savings per month across all months that have data.
    SavingsForecast forecastSavings(Cents goal) const;

    // Used when loading data from storage.
    void loadTransactions(std::vector<Transaction> transactions);
    void loadBudgets(std::map<std::string, Cents> budgets);

private:
    // Throws std::invalid_argument with a description of the first invalid field.
    static void validate(const Transaction& transaction);

    std::vector<Transaction> transactions_;
    std::map<std::string, Cents> budgets_;   // category -> monthly budget
    int nextId_ = 1;
};

#endif // FINANCE_MANAGER_H
