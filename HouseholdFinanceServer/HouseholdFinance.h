#ifndef HOUSEHOLD_FINANCE_H
#define HOUSEHOLD_FINANCE_H

#include <map>
#include <string>
#include <vector>

#include "Transaction.h"

class HouseholdFinance {
private:
    std::vector<Transaction> transactions;
    std::map<std::string, double> categoryBudgets;
    int nextId = 1;
    const std::string filename = "finance_data.txt";

    // אבטחה: פונקציה שמנקה את הקלט מתווים מסוכנים (מניעת הזרקות קוד לקובץ)
    std::string sanitizeInput(std::string input);
    double getCategoryTotalSpent(std::string cat);

public:
    HouseholdFinance();

    // הוספת תנועה חדשה עם מנגנון הגנה מובנה
    void addTransaction(std::string desc, double amt, std::string cat, bool isExp, std::string dt);

    void saveToFile();
    void loadFromFile();

    // הדפסת הדו"ח הפיננסי בעברית
    void printReport();

    // פיצ'ר אלגוריתמי חכם: חישוב צפי להגעה ליעד חיסכון
    void calculateSavingsGoal();

    void runMenu();
};

#endif // HOUSEHOLD_FINANCE_H
