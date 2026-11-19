#ifndef CONSOLE_APP_H
#define CONSOLE_APP_H

#include <string>

#include "FileStorage.h"
#include "FinanceManager.h"

// The text-based user interface: shows the menu, reads input and prints results.
// All calculations are delegated to FinanceManager and all file access to FileStorage,
// so this class only deals with presentation.
class ConsoleApp {
public:
    ConsoleApp(FinanceManager& manager, const FileStorage& storage);

    // Runs the main menu loop until the user chooses to exit.
    void run();

private:
    void addTransaction();
    void listTransactions();
    void showMonthlyReport();
    void editTransaction();
    void deleteTransaction();
    void setBudget();
    void showSavingsForecast();

    // Lets the user pick one of the months that have data.
    // Returns an empty string if there is no data yet.
    std::string chooseMonth();

    // Asks for a transaction id and returns the transaction, or nullptr if not found.
    const Transaction* chooseTransaction();

    void printTransaction(const Transaction& transaction) const;
    void saveTransactions();
    void saveBudgets();

    FinanceManager& manager_;
    const FileStorage& storage_;
};

#endif // CONSOLE_APP_H
