
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <iomanip> // ספרייה לעיצוב והגבלת ספרות אחרי הנקודה
 
// מבנה נתונים שמייצג הוצאה או הכנסה בודדת
struct Transaction {
    int id;
    std::string description;
    double amount;
    std::string category;
    bool isExpense;
    std::string date;
};
 
class HouseholdFinance {
private:
    std::vector<Transaction> transactions;
    std::map<std::string, double> categoryBudgets;
    int nextId = 1;
    const std::string filename = "finance_data.txt";
 
    // אבטחה: פונקציה שמנקה את הקלט מתווים מסוכנים (מניעת הזרקות קוד לקובץ)
    std::string sanitizeInput(std::string input) {
        input.erase(std::remove(input.begin(), input.end(), ','), input.end());
        input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
        input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
        return input;
    }
 
    double getCategoryTotalSpent(std::string cat) {
        double total = 0;
        for (const auto& t : transactions) {
            if (t.isExpense && t.category == cat) {
                total += t.amount;
            }
        }
        return total;
    }
 
public:
    HouseholdFinance() {
        // הגדרת תקציבים בעברית
        categoryBudgets["אוכל"] = 2000.0;
        categoryBudgets["מגורים"] = 6000.0;
        categoryBudgets["בילויים"] = 1500.0;
        categoryBudgets["כללי"] = 1000.0;
    }
 
    // הוספת תנועה חדשה עם מנגנון הגנה מובנה
    void addTransaction(std::string desc, double amt, std::string cat, bool isExp, std::string dt) {
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
 
    void saveToFile() {
        std::ofstream file(filename);
        if (!file.is_open()) return;
        for (const auto& t : transactions) {
            file << t.id << "," << t.description << "," << t.amount << ","
                 << t.category << "," << t.isExpense << "," << t.date << "\n";
        }
        file.close();
    }
 
    void loadFromFile() {
        std::ifstream file(filename);
        if (!file.is_open()) return;
        transactions.clear();
        std::string line;
        int maxId = 0;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string idStr, desc, amtStr, cat, isExpStr, date;
            std::getline(ss, idStr, ',');
            std::getline(ss, desc, ',');
            std::getline(ss, amtStr, ',');
            std::getline(ss, cat, ',');
            std::getline(ss, isExpStr, ',');
            std::getline(ss, date, ',');
            if(idStr.empty()) continue;
            int id = std::stoi(idStr);
            double amt = std::stod(amtStr);
            bool isExp = (isExpStr == "1");
            transactions.push_back({id, desc, amt, cat, isExp, date});
            if (id > maxId) maxId = id;
        }
        nextId = maxId + 1;
        file.close();
    }
 
    // הדפסת הדו"ח הפיננסי בעברית
    void printReport() {
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
    void calculateSavingsGoal() {
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
 
        double goalAmount;
        std::cout << "הכנס את סכום היעד שברצונכם לחסוך (למשל: 30000 לחופשה): ";
        std::cin >> goalAmount;
 
        if (goalAmount <= 0) {
            std::cout << "[X] שגיאה: סכום היעד חייב להיות גדול מ-0!" << std::endl;
            return;
        }
 
        // חישוב מספר החודשים (חלוקה פשוטה של סכום היעד בחיסכון החודשי הצפוי)
        double monthsNeeded = goalAmount / monthlySavings;
        
        std::cout << "\n[V] ניתוח כלכלי למשק הבית:" << std::endl;
        std::cout << "* החיסכון הפוטנציאלי שלכם בכל חודש הוא: " << monthlySavings << " ש\"ח." << std::endl;
        std::cout << "* כדי להגיע ליעד של " << goalAmount << " ש\"ח, ייקח לכם כ-" << std::fixed << std::setprecision(1) << monthsNeeded << " חודשים של חיסכון רציף!" << std::endl;
        std::cout << "----------------------------------------\n" << std::endl;
    }
 
    void runMenu() {
        int choice = 0;
        while (choice != 4) {
            std::cout << "=== תפריט ניהול תקציב מאובטח ===" << std::endl;
            std::cout << "1. הוספת תנועה חדשה (הוצאה או הכנסה)" << std::endl;
            std::cout << "2. הצגת הדו\"ח הכספי" << std::endl;
            std::cout << "3. חישוב צפי הגעה ליעד חיסכון" << std::endl;
            std::cout << "4. יציאה ושמירה" << std::endl;
            std::cout << "הקש את הבחירה שלך: ";
            std::cin >> choice;
 
            if (choice == 1) {
                std::string desc, cat, date;
                double amt;
                int type;
 
                std::cout << "\nהכנס תיאור (למשל: סופרמרקט): ";
                std::cin.ignore();
                std::getline(std::cin, desc);
                
                std::cout << "הכנס סכום: ";
                std::cin >> amt;
                
                std::cout << "הכנס קטגוריה (אוכל, מגורים, בילויים, כללי): ";
                std::cin >> cat;
                
                std::cout << "האם זו הוצאה (1) או הכנסה (2)? ";
                std::cin >> type;
                bool isExp = (type == 1);
                
                std::cout << "הכנס תאריך (שנה-חודש-יום): ";
                std::cin >> date;
 
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
        std::cout << "\nלהתראות! כל הנתונים נשמרו בצורה מאובטחת." << std::endl;
    }
};
 
int main() {
    HouseholdFinance ourHome;
    ourHome.loadFromFile();
 
    // נתוני דוגמה בלבד, נטענים רק אם קובץ הנתונים ריק
    std::ifstream file("finance_data.txt");
    if (file.peek() == std::ifstream::traits_type::eof()) {
        ourHome.addTransaction("משכורת א", 10000.0, "שכר", false, "2026-10-01");
        ourHome.addTransaction("משכורת ב", 9000.0, "שכר", false, "2026-10-01");
        ourHome.addTransaction("תשלום שכירות", 4500.0, "מגורים", true, "2026-10-02");
    }
    file.close();
 
    ourHome.runMenu();
    return 0;
}
