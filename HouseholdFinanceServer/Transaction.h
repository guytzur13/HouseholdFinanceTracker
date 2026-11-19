#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

// מבנה נתונים שמייצג הוצאה או הכנסה בודדת
struct Transaction {
    int id;
    std::string description;
    double amount;
    std::string category;
    bool isExpense;
    std::string date;
};

#endif // TRANSACTION_H
