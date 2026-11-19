#include "FinanceManager.h"

#include <algorithm>
#include <set>
#include <stdexcept>

#include "Validation.h"

void FinanceManager::validate(const Transaction& transaction) {
    if (transaction.description.empty()) {
        throw std::invalid_argument("description must not be empty");
    }
    if (transaction.category.empty()) {
        throw std::invalid_argument("category must not be empty");
    }
    if (transaction.amount <= 0) {
        throw std::invalid_argument("amount must be greater than 0");
    }
    if (!isValidDate(transaction.date)) {
        throw std::invalid_argument("date must be a valid YYYY-MM-DD date");
    }
}

int FinanceManager::addTransaction(const std::string& description, Cents amount,
                                   const std::string& category, TransactionType type,
                                   const std::string& date) {
    Transaction transaction;
    transaction.description = sanitizeText(description);
    transaction.amount = amount;
    transaction.category = sanitizeText(category);
    transaction.type = type;
    transaction.date = date;

    validate(transaction);  // throws before anything is changed, so a failed add has no effect

    transaction.id = nextId_++;
    transactions_.push_back(transaction);
    return transaction.id;
}

bool FinanceManager::updateTransaction(const Transaction& updated) {
    Transaction cleaned = updated;
    cleaned.description = sanitizeText(updated.description);
    cleaned.category = sanitizeText(updated.category);
    validate(cleaned);

    for (Transaction& existing : transactions_) {
        if (existing.id == cleaned.id) {
            existing = cleaned;
            return true;
        }
    }
    return false;
}

bool FinanceManager::removeTransaction(int id) {
    auto it = std::find_if(transactions_.begin(), transactions_.end(),
                           [id](const Transaction& t) { return t.id == id; });
    if (it == transactions_.end()) return false;
    transactions_.erase(it);
    return true;
}

const Transaction* FinanceManager::findTransaction(int id) const {
    for (const Transaction& t : transactions_) {
        if (t.id == id) return &t;
    }
    return nullptr;
}

std::vector<Transaction> FinanceManager::transactionsForMonth(const std::string& month) const {
    std::vector<Transaction> result;
    for (const Transaction& t : transactions_) {
        if (t.month() == month) result.push_back(t);
    }
    // Dates are YYYY-MM-DD, so comparing them as strings sorts them chronologically.
    std::sort(result.begin(), result.end(), [](const Transaction& a, const Transaction& b) {
        if (a.date != b.date) return a.date < b.date;
        return a.id < b.id;
    });
    return result;
}

std::vector<std::string> FinanceManager::months() const {
    std::set<std::string> unique;  // std::set keeps values sorted and without duplicates
    for (const Transaction& t : transactions_) {
        unique.insert(t.month());
    }
    return std::vector<std::string>(unique.begin(), unique.end());
}

MonthlySummary FinanceManager::summarizeMonth(const std::string& month) const {
    MonthlySummary summary;
    summary.month = month;
    for (const Transaction& t : transactions_) {
        if (t.month() != month) continue;
        if (t.isExpense()) {
            summary.expenses += t.amount;
            summary.spentByCategory[t.category] += t.amount;
        } else {
            summary.income += t.amount;
        }
    }
    return summary;
}

std::vector<BudgetStatus> FinanceManager::budgetStatus(const std::string& month) const {
    MonthlySummary summary = summarizeMonth(month);
    std::vector<BudgetStatus> result;
    for (const auto& [category, budget] : budgets_) {
        BudgetStatus status;
        status.category = category;
        status.budget = budget;
        auto it = summary.spentByCategory.find(category);
        status.spent = (it != summary.spentByCategory.end()) ? it->second : 0;
        result.push_back(status);
    }
    return result;
}

bool FinanceManager::isOverBudget(const std::string& category, const std::string& month) const {
    auto budgetIt = budgets_.find(category);
    if (budgetIt == budgets_.end()) return false;  // no budget set for this category

    MonthlySummary summary = summarizeMonth(month);
    auto spentIt = summary.spentByCategory.find(category);
    Cents spent = (spentIt != summary.spentByCategory.end()) ? spentIt->second : 0;
    return spent > budgetIt->second;
}

void FinanceManager::setBudget(const std::string& category, Cents amount) {
    std::string cleanCategory = sanitizeText(category);
    if (cleanCategory.empty()) {
        throw std::invalid_argument("category must not be empty");
    }
    if (amount <= 0) {
        throw std::invalid_argument("budget must be greater than 0");
    }
    budgets_[cleanCategory] = amount;
}

SavingsForecast FinanceManager::forecastSavings(Cents goal) const {
    SavingsForecast forecast;
    std::vector<std::string> allMonths = months();
    forecast.monthsOfData = static_cast<int>(allMonths.size());
    if (allMonths.empty() || goal <= 0) return forecast;

    Cents totalNet = 0;
    for (const std::string& month : allMonths) {
        totalNet += summarizeMonth(month).net();
    }
    forecast.averageMonthlySavings = totalNet / forecast.monthsOfData;

    if (forecast.averageMonthlySavings <= 0) return forecast;  // not saving -> goal unreachable

    forecast.reachable = true;
    // Integer ceiling division: a goal of 10,000 at 3,000 per month needs 4 months, not 3.
    forecast.monthsNeeded = static_cast<int>(
        (goal + forecast.averageMonthlySavings - 1) / forecast.averageMonthlySavings);
    return forecast;
}

void FinanceManager::loadTransactions(std::vector<Transaction> transactions) {
    transactions_ = std::move(transactions);
    int maxId = 0;
    for (const Transaction& t : transactions_) {
        maxId = std::max(maxId, t.id);
    }
    nextId_ = maxId + 1;  // new transactions never reuse an existing id
}

void FinanceManager::loadBudgets(std::map<std::string, Cents> budgets) {
    budgets_ = std::move(budgets);
}
