#include "TransactionDialog.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "Money.h"
#include "Validation.h"

TransactionDialog::TransactionDialog(const QStringList& categories, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("תנועה חדשה");
    setMinimumWidth(380);

    expenseButton_ = new QRadioButton("הוצאה");
    incomeButton_ = new QRadioButton("הכנסה");
    expenseButton_->setChecked(true);
    auto* typeGroup = new QButtonGroup(this);
    typeGroup->addButton(expenseButton_);
    typeGroup->addButton(incomeButton_);
    auto* typeRow = new QHBoxLayout;
    typeRow->addWidget(expenseButton_);
    typeRow->addWidget(incomeButton_);
    typeRow->addStretch();

    descriptionEdit_ = new QLineEdit;
    descriptionEdit_->setPlaceholderText("למשל: סופרמרקט");

    amountEdit_ = new QLineEdit;
    amountEdit_->setPlaceholderText("למשל: 250 או 99.90");

    categoryBox_ = new QComboBox;
    categoryBox_->setEditable(true);  // pick an existing category or type a new one
    categoryBox_->addItems(categories);
    categoryBox_->setCurrentText("");

    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("dd/MM/yyyy");

    errorLabel_ = new QLabel;
    errorLabel_->setStyleSheet("color: #d64545;");
    errorLabel_->setWordWrap(true);
    errorLabel_->hide();

    auto* form = new QFormLayout;
    form->addRow("סוג:", typeRow);
    form->addRow("תיאור:", descriptionEdit_);
    form->addRow("סכום (₪):", amountEdit_);
    form->addRow("קטגוריה:", categoryBox_);
    form->addRow("תאריך:", dateEdit_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText("שמירה");
    buttons->button(QDialogButtonBox::Cancel)->setText("ביטול");
    connect(buttons, &QDialogButtonBox::accepted, this, &TransactionDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &TransactionDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(errorLabel_);
    layout->addWidget(buttons);
}

void TransactionDialog::setTransaction(const Transaction& transaction) {
    setWindowTitle("עריכת תנועה");
    result_ = transaction;  // keeps the id
    (transaction.isExpense() ? expenseButton_ : incomeButton_)->setChecked(true);
    descriptionEdit_->setText(QString::fromStdString(transaction.description));
    // formatMoney adds thousands separators, which parseMoney rejects, so remove them
    amountEdit_->setText(QString::fromStdString(formatMoney(transaction.amount)).remove(','));
    categoryBox_->setCurrentText(QString::fromStdString(transaction.category));
    dateEdit_->setDate(QDate::fromString(QString::fromStdString(transaction.date), "yyyy-MM-dd"));
}

void TransactionDialog::accept() {
    std::string description = sanitizeText(descriptionEdit_->text().toStdString());
    std::string category = sanitizeText(categoryBox_->currentText().toStdString());
    Cents amount = 0;

    QString error;
    if (description.empty()) {
        error = "יש למלא תיאור.";
    } else if (!parseMoney(amountEdit_->text().trimmed().toStdString(), amount)) {
        error = "הסכום חייב להיות מספר חיובי עם עד 2 ספרות אחרי הנקודה.";
    } else if (category.empty()) {
        error = "יש לבחור או להקליד קטגוריה.";
    }

    if (!error.isEmpty()) {
        errorLabel_->setText(error);
        errorLabel_->show();
        return;  // keep the dialog open so the user can fix the input
    }

    result_.description = description;
    result_.amount = amount;
    result_.category = category;
    result_.type = expenseButton_->isChecked() ? TransactionType::Expense : TransactionType::Income;
    result_.date = dateEdit_->date().toString("yyyy-MM-dd").toStdString();
    QDialog::accept();
}
