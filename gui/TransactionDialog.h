#ifndef TRANSACTION_DIALOG_H
#define TRANSACTION_DIALOG_H

#include <QDialog>
#include <QStringList>

#include "Transaction.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QRadioButton;

// A form for adding a new transaction or editing an existing one.
// It only collects and checks the input; saving is done by the caller.
class TransactionDialog : public QDialog {
    Q_OBJECT

public:
    // `categories` fills the category list; the user may also type a new one.
    TransactionDialog(const QStringList& categories, QWidget* parent = nullptr);

    // Fills the form with an existing transaction (edit mode).
    void setTransaction(const Transaction& transaction);

    // The values the user entered. Only valid after the dialog was accepted.
    Transaction transaction() const { return result_; }

protected:
    void accept() override;  // validates before closing

private:
    QRadioButton* expenseButton_;
    QRadioButton* incomeButton_;
    QLineEdit* descriptionEdit_;
    QLineEdit* amountEdit_;
    QComboBox* categoryBox_;
    QDateEdit* dateEdit_;
    QLabel* errorLabel_;
    Transaction result_;
};

#endif // TRANSACTION_DIALOG_H
