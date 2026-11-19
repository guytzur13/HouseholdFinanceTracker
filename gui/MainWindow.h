#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QDateTime>
#include <QMainWindow>
#include <QString>
#include <memory>

#include "FileStorage.h"
#include "FinanceManager.h"

class QComboBox;
class QFileSystemWatcher;
class QLabel;
class QTableWidget;
class QTimer;
class QVBoxLayout;

// The main window of the desktop app.
//
// Like ConsoleApp, it contains no business logic: every calculation is done by
// FinanceManager and every file operation by FileStorage. This class only shows
// data and turns button clicks into calls on those two classes.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void addTransaction();
    void editSelectedTransaction();
    void deleteSelectedTransaction();
    void setBudget();
    void showSavingsForecast();
    void chooseDataFolder();
    void onDataFilesChanged();  // another computer (through cloud sync) changed the files

private:
    void buildUi();
    void loadData();                         // (re)reads the files from the data folder
    bool saveTransactions();
    bool saveBudgets();
    void refresh(const QString& monthToShow = QString());
    void refreshMonthList(const QString& monthToShow);
    void refreshTable(const std::string& month);
    void refreshSummary(const std::string& month);
    void refreshBudgets(const std::string& month);
    void watchDataFolder();
    void rememberFileState();                // what the files look like after our own load/save
    bool filesChangedOnDisk() const;         // changed since then, i.e. by someone else
    void syncFromDisk();                     // reload if another computer changed the files
    int selectedTransactionId() const;       // -1 if no row is selected
    QStringList knownCategories() const;
    std::string currentMonth() const;

    QString dataFolder_;
    std::unique_ptr<FileStorage> storage_;
    FinanceManager manager_;

    QComboBox* monthBox_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* incomeLabel_ = nullptr;
    QLabel* expensesLabel_ = nullptr;
    QLabel* netLabel_ = nullptr;
    QVBoxLayout* budgetsLayout_ = nullptr;
    QLabel* folderLabel_ = nullptr;

    QFileSystemWatcher* watcher_ = nullptr;
    QTimer* reloadTimer_ = nullptr;      // waits a moment so a sync that writes in steps is read once

    // Size and modification time of each data file after our last load or save.
    // Our own saves also trigger the file watcher; comparing against this tells them apart
    // from changes made by another computer.
    struct FileState {
        QDateTime modified;
        qint64 size = -1;
        bool operator==(const FileState& other) const {
            return modified == other.modified && size == other.size;
        }
    };
    FileState transactionsState_;
    FileState budgetsState_;
    FileState readFileState(const char* name) const;
};

#endif // MAIN_WINDOW_H
