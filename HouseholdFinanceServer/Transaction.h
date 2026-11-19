#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

#include "Money.h"

// enum class (rather than bool isExpense) makes the code self-explanatory:
// TransactionType::Expense is clearer than `true` at a call site.
enum class TransactionType { Income, Expense };

// A single income or expense entry.
struct Transaction {
    int id = 0;
    std::string description;
    Cents amount = 0;               // always positive; `type` says whether it is income or expense
    std::string category;
    TransactionType type = TransactionType::Expense;
    std::string date;               // YYYY-MM-DD

    bool isExpense() const { return type == TransactionType::Expense; }

    // "2026-11-19" -> "2026-11". Used to group transactions by month.
    std::string month() const { return date.substr(0, 7); }
};

#endif // TRANSACTION_H
