#include "HouseholdFinance.h"
#include "InputUtils.h"

#include <algorithm>
#include <fstream>
#include <iomanip> // ספרייה לעיצוב והגבלת ספרות אחרי הנקודה
#include <iostream>
#include <sstream>
#include <stdexcept>

// אבטחה: פונקציה שמנקה את הקלט מתווים מסוכנים (מניעת הזרקות קוד לקובץ)
std::string HouseholdFinance::sanitizeInput(std::string input) {
    input.erase(std::remove(input.begin(), input.end(), ','), input.end());
    input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
    input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
    return input;
}

double HouseholdFinance::getCategoryTotalSpent(std::string cat) {
    double total = 0;
    for (const auto& t : transactions) {
        if (t.isExpense && t.category == cat) {
            total += t.amount;
        }
    }
    return total;
}

HouseholdFinance::HouseholdFinance() {
    // הגדרת תקציבים בעברית
    categoryBudgets["אוכל"] = 2000.0;
    categoryBudgets["מגורים"] = 6000.0;
    categoryBudgets["בילויים"] = 1500.0;
    categoryBudgets["כללי"] = 1000.0;
}

// הוספת תנועה חדשה עם מנגנון הגנה מובנה
void HouseholdFinance::addTransaction(std::string desc, double amt, std::string cat, bool isExp, std::string dt) {
    std::string cleanDesc = sanitizeInput(desc);
    std::string cleanCat = sanitizeInput(cat);
    std::string cleanDate = sanitizeInput(dt);

    if (amt <= 0) {
        std::cout << "[X] שגיאה: הסכום חייב להיות גדול מ-0!" << std::endl;
        return;
    }

    Transaction t = { nextId++, cleanDesc, amt, cleanCat, isExp, cleanDate };
    transactions.push_back(t);
    std::cout << "[V] נוסף בהצלחה: " << cleanDesc << " (" << amt << " ש\"ח)" << std::endl;

    // בדיקת חריגה מהתקציב
    if (isExp && categoryBudgets.find(cleanCat) != categoryBudgets.end()) {
        double totalSpent = getCategoryTotalSpent(cleanCat);
        double limit = categoryBudgets[cleanCat];
        if (totalSpent > limit) {
            std::cout << "\n[!!! אזהרת תקציב !!!] חריגה מהתקציב בקטגוריית '" << cleanCat << "'!" << std::endl;
            std::cout << "בוזבז עד כה: " << totalSpent << " מתוך " << limit << " ש\"ח!\n" << std::endl;
        }
    }

    saveToFile();
}

void HouseholdFinance::saveToFile() {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    for (const auto& t : transactions) {
        file << t.id << "," << t.description << "," << t.amount << ","
             << t.category << "," << t.isExpense << "," << t.date << "\n";
    }
    file.close();
}

void HouseholdFinance::loadFromFile() {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    transactions.clear();
    std::string line;
    int maxId = 0;
    int skippedLines = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string idStr, desc, amtStr, cat, isExpStr, date;
        std::getline(ss, idStr, ',');
        std::getline(ss, desc, ',');
        std::getline(ss, amtStr, ',');
        std::getline(ss, cat, ',');
        std::getline(ss, isExpStr, ',');
        std::getline(ss, date, ',');
        if (idStr.empty()) continue;

        // שורה פגומה בקובץ (למשל אם מישהו ערך אותו ידנית) לא תפיל את התוכנה,
        // אלא תדולג עם הודעה למשתמש
        try {
            int id = std::stoi(idStr);
            double amt = std::stod(amtStr);
            if (amt <= 0 || (isExpStr != "0" && isExpStr != "1")) {
                throw std::invalid_argument("bad field");
            }
            bool isExp = (isExpStr == "1");
            transactions.push_back({id, desc, amt, cat, isExp, date});
            if (id > maxId) maxId = id;
        } catch (const std::exception&) {
            skippedLines++;
        }
    }
    nextId = maxId + 1;
    file.close();

    if (skippedLines > 0) {
        std::cout << "[!] אזהרה: דולגו " << skippedLines
                  << " שורות פגומות בקובץ הנתונים." << std::endl;
    }
}

// הדפסת הדו"ח הפיננסי בעברית
void HouseholdFinance::printReport() {
    double totalIncome = 0;
    double totalExpense = 0;

    std::cout << "\n=========================================" << std::endl;
    std::cout << "--- דו\"ח כספי מאובטח ---" << std::endl;
    std::cout << "=========================================" << std::endl;

    for (const auto& t : transactions) {
        if (t.isExpense) {
            totalExpense += t.amount;
            std::cout << "[-] הוצאה: " << t.description << " | " << t.amount << " ש\"ח [" << t.category << "]" << std::endl;
        } else {
            totalIncome += t.amount;
            std::cout << "[+] הכנסה: " << t.description << " | " << t.amount << " ש\"ח" << std::endl;
        }
    }

    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "סך כל ההכנסות: " << totalIncome << " ש\"ח" << std::endl;
    std::cout << "סך כל ההוצאות: " << totalExpense << " ש\"ח" << std::endl;
    std::cout << "נטו שנשאר:     " << (totalIncome - totalExpense) << " ש\"ח" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;

    std::cout << "--- סטטוס תקציבים ---" << std::endl;
    for (const auto& pair : categoryBudgets) {
        double spent = getCategoryTotalSpent(pair.first);
        std::cout << "* " << pair.first << ": " << spent << " / " << pair.second << " ש\"ח";
        if (spent > pair.second) std::cout << " (חרגתם!)";
        std::cout << std::endl;
    }
    std::cout << "=========================================\n" << std::endl;
}

// פיצ'ר אלגוריתמי חכם: חישוב צפי להגעה ליעד חיסכון
void HouseholdFinance::calculateSavingsGoal() {
    double totalIncome = 0;
    double totalExpense = 0;

    for (const auto& t : transactions) {
        if (t.isExpense) totalExpense += t.amount;
        else totalIncome += t.amount;
    }

    double monthlySavings = totalIncome - totalExpense;

    std::cout << "\n--- מחשבון יעדי חיסכון חכם ---" << std::endl;
    if (monthlySavings <= 0) {
        std::cout << "[X] אזהרה: כרגע משק הבית בגרעון או מאוזן (לא נשאר כסף לחיסכון החודש)." << std::endl;
        std::cout << "כדי לחסוך, עליכם להגדיל הכנסות או לצמצם הוצאות!" << std::endl;
        return;
    }

    double goalAmount = readPositiveDouble("הכנס את סכום היעד שברצונכם לחסוך (למשל: 30000 לחופשה): ");

    // חישוב מספר החודשים (חלוקה פשוטה של סכום היעד בחיסכון החודשי הצפוי)
    double monthsNeeded = goalAmount / monthlySavings;

    std::cout << "\n[V] ניתוח כלכלי למשק הבית:" << std::endl;
    std::cout << "* החיסכון הפוטנציאלי שלכם בכל חודש הוא: " << monthlySavings << " ש\"ח." << std::endl;
    std::cout << "* כדי להגיע ליעד של " << goalAmount << " ש\"ח, ייקח לכם כ-" << std::fixed << std::setprecision(1) << monthsNeeded << " חודשים של חיסכון רציף!" << std::endl;
    std::cout << "----------------------------------------\n" << std::endl;
}

void HouseholdFinance::runMenu() {
    try {
        int choice = 0;
        while (choice != 4) {
            std::cout << "=== תפריט ניהול תקציב מאובטח ===" << std::endl;
            std::cout << "1. הוספת תנועה חדשה (הוצאה או הכנסה)" << std::endl;
            std::cout << "2. הצגת הדו\"ח הכספי" << std::endl;
            std::cout << "3. חישוב צפי הגעה ליעד חיסכון" << std::endl;
            std::cout << "4. יציאה ושמירה" << std::endl;
            choice = readInt("הקש את הבחירה שלך: ", 1, 4);

            if (choice == 1) {
                std::cout << std::endl;
                std::string desc = readLine("הכנס תיאור (למשל: סופרמרקט): ");
                double amt = readPositiveDouble("הכנס סכום: ");
                std::string cat = readLine("הכנס קטגוריה (אוכל, מגורים, בילויים, כללי): ");
                int type = readInt("האם זו הוצאה (1) או הכנסה (2)? ", 1, 2);
                bool isExp = (type == 1);
                std::string date = readDate("הכנס תאריך (שנה-חודש-יום): ");

                addTransaction(desc, amt, cat, isExp, date);
                std::cout << std::endl;
            }
            else if (choice == 2) {
                printReport();
            }
            else if (choice == 3) {
                calculateSavingsGoal();
            }
        }
    } catch (const InputClosed&) {
        // הקלט נסגר (למשל Ctrl+D) - יוצאים בצורה מסודרת במקום להיתקע בלולאה
        std::cout << std::endl;
    }
    std::cout << "\nלהתראות! כל הנתונים נשמרו בצורה מאובטחת." << std::endl;
}
