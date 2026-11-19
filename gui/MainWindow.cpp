#include "MainWindow.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <filesystem>
#include <set>
#include <stdexcept>

#include "Money.h"
#include "TransactionDialog.h"

namespace {

const char* kTransactionsFile = "transactions.csv";
const char* kBudgetsFile = "budgets.csv";
const char* kFolderSetting = "dataFolder";

// QString -> filesystem path. Windows paths are wide strings; elsewhere UTF-8 is used.
std::filesystem::path toPath(const QString& text) {
#ifdef _WIN32
    return std::filesystem::path(text.toStdWString());
#else
    return std::filesystem::path(text.toStdString());
#endif
}

QString qs(const std::string& text) { return QString::fromStdString(text); }

QString money(Cents amount) { return qs(formatMoney(amount)) + " ₪"; }

std::map<std::string, Cents> defaultBudgets() {
    return {
        {"אוכל", 2000LL * 100},
        {"מגורים", 6000LL * 100},
        {"בילויים", 1500LL * 100},
        {"כללי", 1000LL * 100},
    };
}

// One of the three numbers at the top of the window (income, expenses, net).
QLabel* addSummaryBox(QHBoxLayout* row, const QString& title) {
    auto* box = new QFrame;
    box->setFrameShape(QFrame::StyledPanel);
    auto* layout = new QVBoxLayout(box);
    auto* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("color: gray;");
    auto* valueLabel = new QLabel("0.00 ₪");
    valueLabel->setStyleSheet("font-size: 20px; font-weight: 600;");
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    row->addWidget(box);
    return valueLabel;
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("ניהול תקציב משק הבית");
    resize(1000, 640);

    // The data folder is remembered between runs. Choosing a folder that a cloud service
    // syncs (iCloud Drive, Google Drive, Dropbox, OneDrive) shares the data between computers.
    QSettings settings;
    dataFolder_ = settings.value(kFolderSetting).toString();
    if (dataFolder_.isEmpty()) {
        dataFolder_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }

    watcher_ = new QFileSystemWatcher(this);
    reloadTimer_ = new QTimer(this);
    reloadTimer_->setSingleShot(true);
    reloadTimer_->setInterval(700);
    connect(watcher_, &QFileSystemWatcher::fileChanged, this, &MainWindow::onDataFilesChanged);
    connect(watcher_, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onDataFilesChanged);
    connect(reloadTimer_, &QTimer::timeout, this, [this] {
        QString month = qs(currentMonth());
        loadData();
        refresh(month);
        statusBar()->showMessage("הנתונים עודכנו ממחשב אחר", 5000);
    });

    buildUi();
    loadData();
    refresh(QDate::currentDate().toString("yyyy-MM"));
}

void MainWindow::buildUi() {
    // ---- Top row: month selector and actions ----
    monthBox_ = new QComboBox;
    monthBox_->setMinimumWidth(120);
    connect(monthBox_, &QComboBox::currentTextChanged, this, [this] { refresh(monthBox_->currentText()); });

    auto* addButton = new QPushButton("+ הוספת תנועה");
    addButton->setDefault(true);
    auto* editButton = new QPushButton("עריכה");
    auto* deleteButton = new QPushButton("מחיקה");
    auto* budgetButton = new QPushButton("הגדרת תקציב");
    auto* savingsButton = new QPushButton("מחשבון חיסכון");
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addTransaction);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::editSelectedTransaction);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedTransaction);
    connect(budgetButton, &QPushButton::clicked, this, &MainWindow::setBudget);
    connect(savingsButton, &QPushButton::clicked, this, &MainWindow::showSavingsForecast);

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel("חודש:"));
    topRow->addWidget(monthBox_);
    topRow->addStretch();
    topRow->addWidget(addButton);
    topRow->addWidget(editButton);
    topRow->addWidget(deleteButton);
    topRow->addWidget(budgetButton);
    topRow->addWidget(savingsButton);

    // ---- Summary: income, expenses, net ----
    auto* summaryRow = new QHBoxLayout;
    incomeLabel_ = addSummaryBox(summaryRow, "הכנסות");
    expensesLabel_ = addSummaryBox(summaryRow, "הוצאות");
    netLabel_ = addSummaryBox(summaryRow, "נטו");

    // ---- Transactions table ----
    table_ = new QTableWidget(0, 6);
    table_->setHorizontalHeaderLabels({"#", "תאריך", "סוג", "תיאור", "קטגוריה", "סכום"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->hide();
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    connect(table_, &QTableWidget::cellDoubleClicked, this, &MainWindow::editSelectedTransaction);

    // ---- Budgets panel ----
    auto* budgetsBox = new QGroupBox("תקציבים החודש");
    budgetsLayout_ = new QVBoxLayout(budgetsBox);
    budgetsBox->setMinimumWidth(280);

    auto* splitter = new QSplitter;
    splitter->addWidget(table_);
    splitter->addWidget(budgetsBox);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    // ---- Data folder line at the bottom ----
    folderLabel_ = new QLabel;
    folderLabel_->setStyleSheet("color: gray;");
    folderLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto* folderButton = new QPushButton("החלפת תיקיית נתונים...");
    connect(folderButton, &QPushButton::clicked, this, &MainWindow::chooseDataFolder);
    auto* folderRow = new QHBoxLayout;
    folderRow->addWidget(folderLabel_, 1);
    folderRow->addWidget(folderButton);

    auto* central = new QWidget;
    auto* layout = new QVBoxLayout(central);
    layout->addLayout(topRow);
    layout->addLayout(summaryRow);
    layout->addWidget(splitter, 1);
    layout->addLayout(folderRow);
    setCentralWidget(central);
}

void MainWindow::loadData() {
    QDir().mkpath(dataFolder_);
    QDir dir(dataFolder_);
    storage_ = std::make_unique<FileStorage>(toPath(dir.filePath(kTransactionsFile)),
                                             toPath(dir.filePath(kBudgetsFile)));

    std::vector<Transaction> transactions;
    LoadResult transactionsResult = storage_->loadTransactions(transactions);
    manager_.loadTransactions(std::move(transactions));

    std::map<std::string, Cents> budgets;
    LoadResult budgetsResult = storage_->loadBudgets(budgets);
    if (!budgetsResult.fileFound) {
        budgets = defaultBudgets();
        manager_.loadBudgets(budgets);
        saveBudgets();
    } else {
        manager_.loadBudgets(std::move(budgets));
    }

    int skipped = transactionsResult.skippedLines + budgetsResult.skippedLines;
    if (skipped > 0) {
        QMessageBox::warning(this, "נתונים פגומים",
                             QString("דולגו %1 שורות פגומות בקבצי הנתונים.").arg(skipped));
    }

    folderLabel_->setText("תיקיית נתונים: " + QDir::toNativeSeparators(dataFolder_));
    watchDataFolder();
    rememberFileState();
}

MainWindow::FileState MainWindow::readFileState(const char* name) const {
    QFileInfo info(QDir(dataFolder_).filePath(name));
    if (!info.exists()) return FileState{};
    return FileState{info.lastModified(), info.size()};
}

void MainWindow::rememberFileState() {
    transactionsState_ = readFileState(kTransactionsFile);
    budgetsState_ = readFileState(kBudgetsFile);
}

bool MainWindow::filesChangedOnDisk() const {
    return !(readFileState(kTransactionsFile) == transactionsState_) ||
           !(readFileState(kBudgetsFile) == budgetsState_);
}

void MainWindow::syncFromDisk() {
    // Called right before every change. If the other computer saved in the meantime,
    // load its version first, so our save adds to it instead of overwriting it.
    if (filesChangedOnDisk()) loadData();
}

void MainWindow::watchDataFolder() {
    if (!watcher_->files().isEmpty()) watcher_->removePaths(watcher_->files());
    if (!watcher_->directories().isEmpty()) watcher_->removePaths(watcher_->directories());

    // Watch the folder as well as the files: saving replaces a file with a new one
    // (atomic rename), and cloud sync often does the same, which a file watch alone misses.
    watcher_->addPath(dataFolder_);
    QDir dir(dataFolder_);
    for (const char* name : {kTransactionsFile, kBudgetsFile}) {
        QString path = dir.filePath(name);
        if (QFile::exists(path)) watcher_->addPath(path);
    }
}

void MainWindow::onDataFilesChanged() {
    // A notification for our own save, or for nothing that matters (another file in the folder)
    if (!filesChangedOnDisk()) return;
    reloadTimer_->start();  // restarts if already running, so several changes reload once
}

bool MainWindow::saveTransactions() {
    bool ok = storage_->saveTransactions(manager_.transactions());
    rememberFileState();
    watchDataFolder();
    if (!ok) {
        QMessageBox::critical(this, "שגיאה", "לא ניתן לשמור את הנתונים בתיקייה:\n" + dataFolder_);
    }
    return ok;
}

bool MainWindow::saveBudgets() {
    bool ok = storage_->saveBudgets(manager_.budgets());
    rememberFileState();
    watchDataFolder();
    if (!ok) {
        QMessageBox::critical(this, "שגיאה", "לא ניתן לשמור את התקציבים בתיקייה:\n" + dataFolder_);
    }
    return ok;
}

void MainWindow::refresh(const QString& monthToShow) {
    refreshMonthList(monthToShow);
    std::string month = currentMonth();
    refreshTable(month);
    refreshSummary(month);
    refreshBudgets(month);
}

void MainWindow::refreshMonthList(const QString& monthToShow) {
    std::vector<std::string> months = manager_.months();
    std::string wanted = monthToShow.toStdString();
    if (!wanted.empty() && std::find(months.begin(), months.end(), wanted) == months.end()) {
        months.push_back(wanted);  // e.g. the current month before anything was added to it
        std::sort(months.begin(), months.end());
    }

    QSignalBlocker block(monthBox_);  // don't trigger refresh() again while refilling the list
    monthBox_->clear();
    for (auto it = months.rbegin(); it != months.rend(); ++it) {  // newest month first
        monthBox_->addItem(qs(*it));
    }
    if (!wanted.empty()) monthBox_->setCurrentText(monthToShow);
}

void MainWindow::refreshTable(const std::string& month) {
    std::vector<Transaction> transactions = manager_.transactionsForMonth(month);
    table_->setRowCount(static_cast<int>(transactions.size()));

    for (int row = 0; row < static_cast<int>(transactions.size()); ++row) {
        const Transaction& t = transactions[static_cast<size_t>(row)];
        QString date = QDate::fromString(qs(t.date), "yyyy-MM-dd").toString("dd/MM/yyyy");
        // No +/- sign: in a right-to-left layout it ends up on the wrong side of the number.
        // The type column and the colour already say whether it is income or an expense.
        QString amount = money(t.amount);

        auto* idItem = new QTableWidgetItem(QString::number(t.id));
        idItem->setData(Qt::UserRole, t.id);
        auto* amountItem = new QTableWidgetItem(amount);
        amountItem->setForeground(t.isExpense() ? QColor("#d64545") : QColor("#2e9e5b"));

        table_->setItem(row, 0, idItem);
        table_->setItem(row, 1, new QTableWidgetItem(date));
        table_->setItem(row, 2, new QTableWidgetItem(t.isExpense() ? "הוצאה" : "הכנסה"));
        table_->setItem(row, 3, new QTableWidgetItem(qs(t.description)));
        table_->setItem(row, 4, new QTableWidgetItem(qs(t.category)));
        table_->setItem(row, 5, amountItem);
    }
}

void MainWindow::refreshSummary(const std::string& month) {
    MonthlySummary summary = manager_.summarizeMonth(month);
    incomeLabel_->setText(money(summary.income));
    expensesLabel_->setText(money(summary.expenses));
    netLabel_->setText(money(summary.net()));
    QString color = summary.net() < 0 ? "#d64545" : "#2e9e5b";
    netLabel_->setStyleSheet("font-size: 20px; font-weight: 600; color: " + color + ";");
}

void MainWindow::refreshBudgets(const std::string& month) {
    // Remove the rows from the previous month before building the new ones.
    while (QLayoutItem* item = budgetsLayout_->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (const BudgetStatus& status : manager_.budgetStatus(month)) {
        auto* label = new QLabel(qs(status.category) + ": " + money(status.spent) + " / " +
                                 money(status.budget));
        auto* bar = new QProgressBar;
        bar->setTextVisible(false);
        bar->setMaximumHeight(10);
        bar->setRange(0, 100);
        // Computed in double: spent * 100 in integers could overflow for very large sums.
        double percent = status.budget > 0
                             ? static_cast<double>(status.spent) * 100.0 / static_cast<double>(status.budget)
                             : 0.0;
        bar->setValue(static_cast<int>(std::min(percent, 100.0)));
        QString color = status.isOverBudget() ? "#d64545" : (percent >= 80 ? "#e0a030" : "#2e9e5b");
        bar->setStyleSheet("QProgressBar::chunk { background-color: " + color + "; }");

        budgetsLayout_->addWidget(label);
        budgetsLayout_->addWidget(bar);
        if (status.isOverBudget()) {
            auto* warning = new QLabel("חריגה של " + money(status.spent - status.budget));
            warning->setStyleSheet("color: #d64545;");
            budgetsLayout_->addWidget(warning);
        }
    }
    budgetsLayout_->addStretch();
}

std::string MainWindow::currentMonth() const {
    return monthBox_->currentText().toStdString();
}

int MainWindow::selectedTransactionId() const {
    int row = table_->currentRow();
    if (row < 0 || table_->item(row, 0) == nullptr) return -1;
    return table_->item(row, 0)->data(Qt::UserRole).toInt();
}

QStringList MainWindow::knownCategories() const {
    std::set<std::string> categories;  // sorted, no duplicates
    for (const auto& [category, budget] : manager_.budgets()) categories.insert(category);
    for (const Transaction& t : manager_.transactions()) categories.insert(t.category);

    QStringList result;
    for (const std::string& category : categories) result << qs(category);
    return result;
}

void MainWindow::addTransaction() {
    TransactionDialog dialog(knownCategories(), this);
    if (dialog.exec() != QDialog::Accepted) return;

    Transaction t = dialog.transaction();
    syncFromDisk();
    try {
        manager_.addTransaction(t.description, t.amount, t.category, t.type, t.date);
    } catch (const std::exception&) {
        QMessageBox::warning(this, "הוספת תנועה", "לא ניתן להוסיף את התנועה.");
        return;
    }
    if (!saveTransactions()) return;

    refresh(qs(t.month()));
    if (t.isExpense() && manager_.isOverBudget(t.category, t.month())) {
        QMessageBox::warning(this, "חריגה מהתקציב",
                             "ההוצאה הזאת עברה את התקציב של '" + qs(t.category) + "' החודש.");
    }
}

void MainWindow::editSelectedTransaction() {
    int id = selectedTransactionId();
    const Transaction* existing = manager_.findTransaction(id);
    if (existing == nullptr) {
        QMessageBox::information(this, "עריכה", "יש לבחור תנועה בטבלה.");
        return;
    }

    TransactionDialog dialog(knownCategories(), this);
    dialog.setTransaction(*existing);
    if (dialog.exec() != QDialog::Accepted) return;

    Transaction updated = dialog.transaction();
    syncFromDisk();
    bool found = false;
    try {
        found = manager_.updateTransaction(updated);
    } catch (const std::exception&) {
        QMessageBox::warning(this, "עריכה", "הנתונים שהוזנו לא תקינים.");
        return;
    }
    if (!found) {  // deleted on the other computer while the dialog was open
        QMessageBox::information(this, "עריכה", "התנועה נמחקה במחשב אחר, ולכן לא נשמרה.");
        refresh(qs(currentMonth()));
        return;
    }
    if (saveTransactions()) refresh(qs(updated.month()));
}

void MainWindow::deleteSelectedTransaction() {
    int id = selectedTransactionId();
    const Transaction* existing = manager_.findTransaction(id);
    if (existing == nullptr) {
        QMessageBox::information(this, "מחיקה", "יש לבחור תנועה בטבלה.");
        return;
    }

    QString question = "למחוק את התנועה '" + qs(existing->description) + "' (" +
                       money(existing->amount) + ")?";
    if (QMessageBox::question(this, "מחיקה", question) != QMessageBox::Yes) return;

    QString month = qs(currentMonth());
    syncFromDisk();
    // `existing` may point to freed memory now (syncFromDisk can reload the list), so only `id` is used.
    if (manager_.removeTransaction(id)) {
        if (saveTransactions()) refresh(month);
    } else {
        refresh(month);  // already deleted on the other computer
    }
}

void MainWindow::setBudget() {
    QDialog dialog(this);
    dialog.setWindowTitle("הגדרת תקציב חודשי");
    auto* categoryBox = new QComboBox;
    categoryBox->setEditable(true);
    categoryBox->addItems(knownCategories());
    auto* amountEdit = new QLineEdit;
    amountEdit->setPlaceholderText("למשל: 2000");
    auto* form = new QFormLayout;
    form->addRow("קטגוריה:", categoryBox);
    form->addRow("תקציב לחודש (₪):", amountEdit);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto* layout = new QVBoxLayout(&dialog);
    layout->addLayout(form);
    layout->addWidget(buttons);

    // Show the current budget when an existing category is picked.
    connect(categoryBox, &QComboBox::currentTextChanged, &dialog, [this, amountEdit](const QString& text) {
        auto it = manager_.budgets().find(text.toStdString());
        if (it != manager_.budgets().end()) {
            amountEdit->setText(qs(formatMoney(it->second)).remove(','));
        }
    });
    emit categoryBox->currentTextChanged(categoryBox->currentText());

    if (dialog.exec() != QDialog::Accepted) return;

    Cents amount = 0;
    if (!parseMoney(amountEdit->text().trimmed().toStdString(), amount)) {
        QMessageBox::warning(this, "תקציב", "הסכום חייב להיות מספר חיובי.");
        return;
    }
    syncFromDisk();
    try {
        manager_.setBudget(categoryBox->currentText().toStdString(), amount);
    } catch (const std::invalid_argument&) {
        QMessageBox::warning(this, "תקציב", "יש לבחור או להקליד קטגוריה.");
        return;
    }
    if (saveBudgets()) refresh(qs(currentMonth()));
}

void MainWindow::showSavingsForecast() {
    bool ok = false;
    QString text = QInputDialog::getText(this, "מחשבון יעד חיסכון", "כמה רוצים לחסוך? (₪)",
                                         QLineEdit::Normal, "", &ok);
    if (!ok) return;

    Cents goal = 0;
    if (!parseMoney(text.trimmed().remove(',').toStdString(), goal)) {
        QMessageBox::warning(this, "מחשבון יעד חיסכון", "יש להקליד סכום חיובי.");
        return;
    }

    SavingsForecast forecast = manager_.forecastSavings(goal);
    QString message;
    if (forecast.monthsOfData == 0) {
        message = "אין עדיין נתונים לחישוב.";
    } else if (!forecast.reachable) {
        message = QString("החיסכון הממוצע לחודש (לפי %1 חודשים) הוא %2.\n"
                          "בממוצע ההוצאות גבוהות מההכנסות או שוות להן, ולכן לא ניתן להגיע ליעד.")
                      .arg(forecast.monthsOfData)
                      .arg(money(forecast.averageMonthlySavings));
    } else {
        message = QString("החיסכון הממוצע לחודש (לפי %1 חודשים) הוא %2.\n"
                          "בקצב הזה תגיעו ליעד של %3 בעוד כ-%4 חודשים.")
                      .arg(forecast.monthsOfData)
                      .arg(money(forecast.averageMonthlySavings))
                      .arg(money(goal))
                      .arg(forecast.monthsNeeded);
    }
    QMessageBox::information(this, "מחשבון יעד חיסכון", message);
}

void MainWindow::chooseDataFolder() {
    QString folder = QFileDialog::getExistingDirectory(
        this, "בחירת תיקיית נתונים (למשל תיקייה ב-iCloud Drive או ב-Google Drive)", dataFolder_);
    if (folder.isEmpty() || folder == dataFolder_) return;

    QDir oldDir(dataFolder_);
    QDir newDir(folder);
    bool newHasData = QFile::exists(newDir.filePath(kTransactionsFile));
    bool oldHasData = QFile::exists(oldDir.filePath(kTransactionsFile));

    // Moving to an empty folder: offer to bring the current data along.
    // Moving to a folder that already has data (e.g. set up on the other computer): just use it.
    if (!newHasData && oldHasData) {
        auto answer = QMessageBox::question(this, "תיקיית נתונים",
                                            "להעתיק את הנתונים הקיימים לתיקייה החדשה?");
        if (answer == QMessageBox::Yes) {
            for (const char* name : {kTransactionsFile, kBudgetsFile}) {
                QFile::copy(oldDir.filePath(name), newDir.filePath(name));
            }
        }
    }

    dataFolder_ = folder;
    QSettings().setValue(kFolderSetting, dataFolder_);
    loadData();
    refresh(QDate::currentDate().toString("yyyy-MM"));
    statusBar()->showMessage("תיקיית הנתונים הוחלפה", 5000);
}
