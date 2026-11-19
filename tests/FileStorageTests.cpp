#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

#include "FileStorage.h"

namespace {

// Each test gets its own file names and deletes them afterwards.
class FileStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        const std::string name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        transactionsPath = "test_" + name + "_transactions.csv";
        budgetsPath = "test_" + name + "_budgets.csv";
        std::remove(transactionsPath.c_str());
        std::remove(budgetsPath.c_str());
    }

    void TearDown() override {
        std::remove(transactionsPath.c_str());
        std::remove(budgetsPath.c_str());
    }

    void writeFile(const std::string& path, const std::string& content) {
        std::ofstream(path) << content;
    }

    std::string transactionsPath;
    std::string budgetsPath;
};

} // namespace

TEST_F(FileStorageTest, ReportsMissingFile) {
    FileStorage storage(transactionsPath, budgetsPath);
    std::vector<Transaction> loaded;
    LoadResult result = storage.loadTransactions(loaded);
    EXPECT_FALSE(result.fileFound);
    EXPECT_TRUE(loaded.empty());
}

TEST_F(FileStorageTest, SavesAndLoadsTransactions) {
    FileStorage storage(transactionsPath, budgetsPath);
    std::vector<Transaction> original = {
        {1, "salary", 1000000, "salary", TransactionType::Income, "2026-11-01"},
        {2, "סופרמרקט", 23450, "אוכל", TransactionType::Expense, "2026-11-10"},
    };
    ASSERT_TRUE(storage.saveTransactions(original));

    std::vector<Transaction> loaded;
    LoadResult result = storage.loadTransactions(loaded);
    EXPECT_TRUE(result.fileFound);
    EXPECT_EQ(result.skippedLines, 0);
    ASSERT_EQ(loaded.size(), 2u);
    EXPECT_EQ(loaded[1].id, 2);
    EXPECT_EQ(loaded[1].description, "סופרמרקט");  // Hebrew text survives the round trip
    EXPECT_EQ(loaded[1].amount, 23450);
    EXPECT_EQ(loaded[1].category, "אוכל");
    EXPECT_TRUE(loaded[1].isExpense());
    EXPECT_EQ(loaded[1].date, "2026-11-10");
    EXPECT_FALSE(loaded[0].isExpense());
}

TEST_F(FileStorageTest, SkipsMalformedLinesAndKeepsTheRest) {
    writeFile(transactionsPath,
              "id,date,type,amount_agorot,category,description\n"
              "1,2026-11-01,income,1000000,salary,salary\n"
              "this line is garbage\n"
              "2,2026-13-01,expense,100,food,bad month\n"
              "3,2026-11-02,expense,-5,food,negative amount\n"
              "4,2026-11-02,gift,100,food,unknown type\n"
              "x,2026-11-02,expense,100,food,bad id\n"
              "5,2026-11-03,expense,12abc,food,bad amount\n"
              "\n"
              "6,2026-11-04,expense,2500,food,coffee\n");

    FileStorage storage(transactionsPath, budgetsPath);
    std::vector<Transaction> loaded;
    LoadResult result = storage.loadTransactions(loaded);
    EXPECT_EQ(result.skippedLines, 6);
    ASSERT_EQ(loaded.size(), 2u);
    EXPECT_EQ(loaded[0].id, 1);
    EXPECT_EQ(loaded[1].id, 6);
}

TEST_F(FileStorageTest, SavesAndLoadsBudgets) {
    FileStorage storage(transactionsPath, budgetsPath);
    std::map<std::string, Cents> original = {{"food", 200000}, {"rent", 600000}};
    ASSERT_TRUE(storage.saveBudgets(original));

    std::map<std::string, Cents> loaded;
    LoadResult result = storage.loadBudgets(loaded);
    EXPECT_TRUE(result.fileFound);
    EXPECT_EQ(loaded, original);
}

TEST_F(FileStorageTest, SavingReplacesThePreviousContent) {
    FileStorage storage(transactionsPath, budgetsPath);
    std::vector<Transaction> first = {
        {1, "a", 100, "x", TransactionType::Expense, "2026-11-01"},
        {2, "b", 100, "x", TransactionType::Expense, "2026-11-01"},
    };
    std::vector<Transaction> second = {{1, "a", 100, "x", TransactionType::Expense, "2026-11-01"}};
    ASSERT_TRUE(storage.saveTransactions(first));
    ASSERT_TRUE(storage.saveTransactions(second));

    std::vector<Transaction> loaded;
    storage.loadTransactions(loaded);
    EXPECT_EQ(loaded.size(), 1u);
}
