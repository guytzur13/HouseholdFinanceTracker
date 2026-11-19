#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>

#include "FinanceManager.h"

namespace {

using T = TransactionType;

// A test fixture: every TEST_F below starts with a fresh manager that has
// two months of data, so each test is independent of the others.
class FinanceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager.setBudget("food", 2000 * 100);
        manager.setBudget("rent", 6000 * 100);

        manager.addTransaction("salary", 10000 * 100, "salary", T::Income, "2026-10-01");
        manager.addTransaction("rent", 4500 * 100, "rent", T::Expense, "2026-10-02");
        manager.addTransaction("groceries", 1500 * 100, "food", T::Expense, "2026-10-15");

        manager.addTransaction("salary", 10000 * 100, "salary", T::Income, "2026-11-01");
        manager.addTransaction("rent", 4500 * 100, "rent", T::Expense, "2026-11-02");
        manager.addTransaction("groceries", 2500 * 100, "food", T::Expense, "2026-11-20");
    }

    FinanceManager manager;
};

} // namespace

TEST_F(FinanceManagerTest, AssignsIncreasingIds) {
    int id = manager.addTransaction("coffee", 1500, "food", T::Expense, "2026-11-21");
    EXPECT_EQ(id, 7);
    EXPECT_EQ(manager.transactions().size(), 7u);
}

TEST_F(FinanceManagerTest, RejectsInvalidTransactionsWithoutChangingData) {
    EXPECT_THROW(manager.addTransaction("x", 0, "food", T::Expense, "2026-11-01"),
                 std::invalid_argument);
    EXPECT_THROW(manager.addTransaction("x", -100, "food", T::Expense, "2026-11-01"),
                 std::invalid_argument);
    EXPECT_THROW(manager.addTransaction("", 100, "food", T::Expense, "2026-11-01"),
                 std::invalid_argument);
    EXPECT_THROW(manager.addTransaction("x", 100, "", T::Expense, "2026-11-01"),
                 std::invalid_argument);
    EXPECT_THROW(manager.addTransaction("x", 100, "food", T::Expense, "2026-02-30"),
                 std::invalid_argument);
    EXPECT_EQ(manager.transactions().size(), 6u);
}

TEST_F(FinanceManagerTest, SanitizesTextBeforeStoring) {
    int id = manager.addTransaction("pizza, drinks", 100, " fun ", T::Expense, "2026-11-05");
    const Transaction* t = manager.findTransaction(id);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->description, "pizza drinks");
    EXPECT_EQ(t->category, "fun");
}

TEST_F(FinanceManagerTest, SummarizesOnlyTheRequestedMonth) {
    MonthlySummary november = manager.summarizeMonth("2026-11");
    EXPECT_EQ(november.income, 10000 * 100);
    EXPECT_EQ(november.expenses, 7000 * 100);
    EXPECT_EQ(november.net(), 3000 * 100);
    EXPECT_EQ(november.spentByCategory["food"], 2500 * 100);

    MonthlySummary empty = manager.summarizeMonth("2025-01");
    EXPECT_EQ(empty.income, 0);
    EXPECT_EQ(empty.expenses, 0);
}

TEST_F(FinanceManagerTest, ListsMonthsInOrderWithoutDuplicates) {
    manager.addTransaction("old", 100, "food", T::Expense, "2026-09-30");
    std::vector<std::string> expected = {"2026-09", "2026-10", "2026-11"};
    EXPECT_EQ(manager.months(), expected);
}

TEST_F(FinanceManagerTest, ReturnsMonthTransactionsSortedByDate) {
    manager.addTransaction("early", 100, "food", T::Expense, "2026-11-01");
    std::vector<Transaction> november = manager.transactionsForMonth("2026-11");
    ASSERT_EQ(november.size(), 4u);
    for (size_t i = 1; i < november.size(); ++i) {
        EXPECT_LE(november[i - 1].date, november[i].date);
    }
}

TEST_F(FinanceManagerTest, DetectsBudgetOverrunPerMonth) {
    EXPECT_FALSE(manager.isOverBudget("food", "2026-10"));  // 1,500 of 2,000
    EXPECT_TRUE(manager.isOverBudget("food", "2026-11"));   // 2,500 of 2,000
    EXPECT_FALSE(manager.isOverBudget("no-such-category", "2026-11"));
}

TEST_F(FinanceManagerTest, BudgetStatusIncludesCategoriesWithNoSpending) {
    manager.setBudget("fun", 500 * 100);
    std::vector<BudgetStatus> statuses = manager.budgetStatus("2026-11");
    ASSERT_EQ(statuses.size(), 3u);
    for (const BudgetStatus& s : statuses) {
        if (s.category == "fun") EXPECT_EQ(s.spent, 0);
        if (s.category == "food") EXPECT_TRUE(s.isOverBudget());
    }
}

TEST_F(FinanceManagerTest, RejectsInvalidBudgets) {
    EXPECT_THROW(manager.setBudget("food", 0), std::invalid_argument);
    EXPECT_THROW(manager.setBudget("", 100), std::invalid_argument);
}

TEST_F(FinanceManagerTest, UpdatesAndRemovesTransactions) {
    Transaction edited = *manager.findTransaction(6);
    edited.amount = 1000 * 100;
    EXPECT_TRUE(manager.updateTransaction(edited));
    EXPECT_FALSE(manager.isOverBudget("food", "2026-11"));

    EXPECT_TRUE(manager.removeTransaction(6));
    EXPECT_EQ(manager.findTransaction(6), nullptr);
    EXPECT_FALSE(manager.removeTransaction(6));  // already gone
}

TEST_F(FinanceManagerTest, UpdateValidatesAndRejectsUnknownIds) {
    Transaction bad = *manager.findTransaction(1);
    bad.date = "not-a-date";
    EXPECT_THROW(manager.updateTransaction(bad), std::invalid_argument);

    Transaction unknown = *manager.findTransaction(1);
    unknown.id = 999;
    EXPECT_FALSE(manager.updateTransaction(unknown));
}

TEST_F(FinanceManagerTest, ForecastsSavingsFromMonthlyAverage) {
    // October saves 4,000 and November saves 3,000 -> average 3,500 per month.
    SavingsForecast forecast = manager.forecastSavings(10000 * 100);
    EXPECT_TRUE(forecast.reachable);
    EXPECT_EQ(forecast.monthsOfData, 2);
    EXPECT_EQ(forecast.averageMonthlySavings, 3500 * 100);
    EXPECT_EQ(forecast.monthsNeeded, 3);  // 10,000 / 3,500 = 2.86 -> rounded up to 3
}

TEST_F(FinanceManagerTest, ForecastIsUnreachableWhenNotSaving) {
    manager.addTransaction("car", 20000 * 100, "car", T::Expense, "2026-11-25");
    SavingsForecast forecast = manager.forecastSavings(1000 * 100);
    EXPECT_FALSE(forecast.reachable);
}

TEST(FinanceManagerEmpty, ForecastWithNoData) {
    FinanceManager manager;
    SavingsForecast forecast = manager.forecastSavings(1000);
    EXPECT_FALSE(forecast.reachable);
    EXPECT_EQ(forecast.monthsOfData, 0);
}

TEST(FinanceManagerLoading, ContinuesIdsAfterTheHighestLoadedId) {
    FinanceManager manager;
    Transaction a{3, "a", 100, "food", T::Expense, "2026-11-01"};
    Transaction b{8, "b", 100, "food", T::Expense, "2026-11-02"};
    manager.loadTransactions({a, b});
    EXPECT_EQ(manager.addTransaction("c", 100, "food", T::Expense, "2026-11-03"), 9);
}

TEST_F(FinanceManagerTest, RejectsAmountsAboveTheMaximum) {
    EXPECT_THROW(manager.addTransaction("x", kMaxAmount + 1, "food", T::Expense, "2026-11-01"),
                 std::invalid_argument);
    EXPECT_NO_THROW(manager.addTransaction("x", kMaxAmount, "food", T::Expense, "2026-11-01"));
    EXPECT_THROW(manager.setBudget("food", kMaxAmount + 1), std::invalid_argument);
}

TEST_F(FinanceManagerTest, ForecastIgnoresTheOrderOfTransactions) {
    // Same data added in a different order must give the same forecast.
    FinanceManager reversed;
    std::vector<Transaction> all = manager.transactions();
    std::reverse(all.begin(), all.end());
    reversed.loadTransactions(all);
    EXPECT_EQ(reversed.forecastSavings(10000 * 100).averageMonthlySavings,
              manager.forecastSavings(10000 * 100).averageMonthlySavings);
}
