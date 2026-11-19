#include "ConsoleApp.h"

#include <iostream>
#include <stdexcept>

#include "InputUtils.h"

namespace {

const std::string kLine = "=========================================";
const std::string kThinLine = "-----------------------------------------";

std::string typeLabel(const Transaction& t) {
    return t.isExpense() ? "הוצאה" : "הכנסה";
}

} // namespace

ConsoleApp::ConsoleApp(FinanceManager& manager, const FileStorage& storage)
    : manager_(manager), storage_(storage) {}

void ConsoleApp::run() {
    try {
        while (true) {
            std::cout << "\n" << kLine << "\n"
                      << "   ניהול תקציב משק הבית\n"
                      << kLine << "\n"
                      << "1. הוספת הכנסה או הוצאה\n"
                      << "2. הצגת תנועות לפי חודש\n"
                      << "3. דו\"ח חודשי ומצב תקציבים\n"
                      << "4. עריכת תנועה\n"
                      << "5. מחיקת תנועה\n"
                      << "6. הגדרת תקציב לקטגוריה\n"
                      << "7. מחשבון יעד חיסכון\n"
                      << "0. יציאה\n";
            int choice = readInt("בחירה: ", 0, 7);
            std::cout << std::endl;

            switch (choice) {
                case 1: addTransaction(); break;
                case 2: listTransactions(); break;
                case 3: showMonthlyReport(); break;
                case 4: editTransaction(); break;
                case 5: deleteTransaction(); break;
                case 6: setBudget(); break;
                case 7: showSavingsForecast(); break;
                case 0:
                    std::cout << "להתראות! כל הנתונים נשמרו." << std::endl;
                    return;
            }
        }
    } catch (const InputClosed&) {
        // Input ended (e.g. Ctrl+D). Everything is saved after each change, so just exit.
        std::cout << "\nהקלט נסגר. כל הנתונים נשמרו." << std::endl;
    }
}

void ConsoleApp::addTransaction() {
    bool isExpense = readInt("1. הוצאה  2. הכנסה\nבחירה: ", 1, 2) == 1;
    std::string description = readLine("תיאור (למשל: סופרמרקט): ");
    Cents amount = readMoney("סכום בש\"ח: ");
    std::string category = isExpense
        ? readLine("קטגוריה (למשל: אוכל, מגורים, בילויים, כללי): ")
        : readLine("קטגוריה (למשל: שכר): ");
    std::string date = readDate("תאריך (YYYY-MM-DD): ");

    TransactionType type = isExpense ? TransactionType::Expense : TransactionType::Income;
    int id = manager_.addTransaction(description, amount, category, type, date);
    saveTransactions();
    std::cout << "[V] נוסף בהצלחה (מספר " << id << ")." << std::endl;

    // Warn right away if this expense pushed the category over its monthly budget.
    std::string month = date.substr(0, 7);
    if (isExpense && manager_.isOverBudget(category, month)) {
        for (const BudgetStatus& status : manager_.budgetStatus(month)) {
            if (status.category == category) {
                std::cout << "[!] אזהרה: חריגה מהתקציב בקטגוריית '" << category << "' בחודש "
                          << month << ": " << formatMoney(status.spent) << " מתוך "
                          << formatMoney(status.budget) << " ש\"ח." << std::endl;
            }
        }
    }
}

void ConsoleApp::listTransactions() {
    std::string month = chooseMonth();
    if (month.empty()) return;

    std::cout << "\nתנועות בחודש " << month << ":\n" << kThinLine << std::endl;
    for (const Transaction& t : manager_.transactionsForMonth(month)) {
        printTransaction(t);
    }
}

void ConsoleApp::showMonthlyReport() {
    std::string month = chooseMonth();
    if (month.empty()) return;

    MonthlySummary summary = manager_.summarizeMonth(month);
    std::cout << "\n" << kLine << "\n"
              << "דו\"ח חודשי: " << month << "\n"
              << kLine << "\n"
              << "הכנסות:  " << formatMoney(summary.income) << " ש\"ח\n"
              << "הוצאות:  " << formatMoney(summary.expenses) << " ש\"ח\n"
              << "נטו:     " << formatMoney(summary.net()) << " ש\"ח\n"
              << kThinLine << "\n"
              << "מצב תקציבים:" << std::endl;

    for (const BudgetStatus& status : manager_.budgetStatus(month)) {
        std::cout << "* " << status.category << ": " << formatMoney(status.spent) << " / "
                  << formatMoney(status.budget) << " ש\"ח";
        if (status.isOverBudget()) {
            std::cout << "  <-- חריגה של " << formatMoney(status.spent - status.budget) << " ש\"ח";
        }
        std::cout << std::endl;
    }

    // Expense categories that have no budget, so nothing is hidden from the report.
    for (const auto& [category, spent] : summary.spentByCategory) {
        if (manager_.budgets().count(category) == 0) {
            std::cout << "* " << category << ": " << formatMoney(spent) << " ש\"ח (ללא תקציב)"
                      << std::endl;
        }
    }
    std::cout << kLine << std::endl;
}

void ConsoleApp::editTransaction() {
    const Transaction* found = chooseTransaction();
    if (found == nullptr) return;

    Transaction updated = *found;  // edit a copy; the manager validates it before saving
    std::cout << "מה לערוך?\n"
              << "1. תיאור\n2. סכום\n3. קטגוריה\n4. סוג (הכנסה/הוצאה)\n5. תאריך\n";
    switch (readInt("בחירה: ", 1, 5)) {
        case 1: updated.description = readLine("תיאור חדש: "); break;
        case 2: updated.amount = readMoney("סכום חדש: "); break;
        case 3: updated.category = readLine("קטגוריה חדשה: "); break;
        case 4:
            updated.type = readInt("1. הוצאה  2. הכנסה\nבחירה: ", 1, 2) == 1
                ? TransactionType::Expense : TransactionType::Income;
            break;
        case 5: updated.date = readDate("תאריך חדש (YYYY-MM-DD): "); break;
    }

    manager_.updateTransaction(updated);
    saveTransactions();
    std::cout << "[V] התנועה עודכנה:" << std::endl;
    printTransaction(updated);
}

void ConsoleApp::deleteTransaction() {
    const Transaction* found = chooseTransaction();
    if (found == nullptr) return;

    int id = found->id;  // copy the id: the pointer is invalid after the transaction is removed
    if (!readYesNo("למחוק את התנועה?")) {
        std::cout << "המחיקה בוטלה." << std::endl;
        return;
    }
    manager_.removeTransaction(id);
    saveTransactions();
    std::cout << "[V] התנועה נמחקה." << std::endl;
}

void ConsoleApp::setBudget() {
    std::cout << "תקציבים נוכחיים:" << std::endl;
    for (const auto& [category, amount] : manager_.budgets()) {
        std::cout << "* " << category << ": " << formatMoney(amount) << " ש\"ח" << std::endl;
    }
    std::string category = readLine("שם הקטגוריה (קיימת או חדשה): ");
    Cents amount = readMoney("תקציב חודשי בש\"ח: ");

    manager_.setBudget(category, amount);
    saveBudgets();
    std::cout << "[V] התקציב נשמר." << std::endl;
}

void ConsoleApp::showSavingsForecast() {
    Cents goal = readMoney("סכום היעד לחיסכון בש\"ח (למשל 30000): ");
    SavingsForecast forecast = manager_.forecastSavings(goal);

    std::cout << "\n--- מחשבון יעד חיסכון ---" << std::endl;
    if (forecast.monthsOfData == 0) {
        std::cout << "אין עדיין נתונים לחישוב." << std::endl;
        return;
    }
    std::cout << "חיסכון ממוצע לחודש (לפי " << forecast.monthsOfData << " חודשים): "
              << formatMoney(forecast.averageMonthlySavings) << " ש\"ח" << std::endl;

    if (!forecast.reachable) {
        std::cout << "[X] בממוצע ההוצאות גבוהות מההכנסות או שוות להן, ולכן לא ניתן להגיע ליעד."
                  << "\nכדי לחסוך צריך להגדיל הכנסות או לצמצם הוצאות." << std::endl;
        return;
    }
    std::cout << "[V] בקצב הנוכחי תגיעו ליעד של " << formatMoney(goal) << " ש\"ח בעוד כ-"
              << forecast.monthsNeeded << " חודשים." << std::endl;
}

std::string ConsoleApp::chooseMonth() {
    std::vector<std::string> months = manager_.months();
    if (months.empty()) {
        std::cout << "אין עדיין תנועות. הוסף תנועה דרך אפשרות 1." << std::endl;
        return "";
    }
    std::cout << "חודשים עם נתונים:" << std::endl;
    for (size_t i = 0; i < months.size(); ++i) {
        std::cout << (i + 1) << ". " << months[i] << std::endl;
    }
    int index = readInt("בחר חודש: ", 1, static_cast<int>(months.size()));
    return months[static_cast<size_t>(index - 1)];
}

const Transaction* ConsoleApp::chooseTransaction() {
    if (manager_.transactions().empty()) {
        std::cout << "אין עדיין תנועות." << std::endl;
        return nullptr;
    }
    std::cout << "טיפ: את מספר התנועה אפשר לראות באפשרות 2 בתפריט." << std::endl;
    int id = readInt("מספר תנועה: ", 1, 1'000'000'000);
    const Transaction* found = manager_.findTransaction(id);
    if (found == nullptr) {
        std::cout << "[X] לא נמצאה תנועה מספר " << id << "." << std::endl;
        return nullptr;
    }
    printTransaction(*found);
    return found;
}

void ConsoleApp::printTransaction(const Transaction& t) const {
    std::cout << "#" << t.id << "  " << t.date << "  " << typeLabel(t) << "  "
              << formatMoney(t.amount) << " ש\"ח  [" << t.category << "]  " << t.description
              << std::endl;
}

void ConsoleApp::saveTransactions() {
    if (!storage_.saveTransactions(manager_.transactions())) {
        std::cout << "[!] שגיאה: לא ניתן לשמור את הנתונים לקובץ." << std::endl;
    }
}

void ConsoleApp::saveBudgets() {
    if (!storage_.saveBudgets(manager_.budgets())) {
        std::cout << "[!] שגיאה: לא ניתן לשמור את התקציבים לקובץ." << std::endl;
    }
}
