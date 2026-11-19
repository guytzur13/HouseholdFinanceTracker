#include <gtest/gtest.h>

#include "Money.h"

TEST(ParseMoney, AcceptsWholeAndDecimalAmounts) {
    Cents value = 0;
    EXPECT_TRUE(parseMoney("120", value));
    EXPECT_EQ(value, 12000);
    EXPECT_TRUE(parseMoney("99.90", value));
    EXPECT_EQ(value, 9990);
    EXPECT_TRUE(parseMoney("12.5", value));   // one decimal digit means tens of agorot
    EXPECT_EQ(value, 1250);
    EXPECT_TRUE(parseMoney("0.01", value));
    EXPECT_EQ(value, 1);
}

TEST(ParseMoney, RejectsInvalidInput) {
    Cents value = 777;
    for (const char* bad : {"", "abc", "-5", "0", "0.00", "1.234", "1.2.3", "12a", ".", "1,000", " 5"}) {
        EXPECT_FALSE(parseMoney(bad, value)) << "input: '" << bad << "'";
    }
    EXPECT_EQ(value, 777) << "a failed parse must not change the output";
}

TEST(ParseMoney, RejectsHugeAmountsInsteadOfOverflowing) {
    Cents value = 0;
    EXPECT_FALSE(parseMoney("99999999999999999999", value));
}

TEST(ParseMoney, AvoidsFloatingPointErrors) {
    // With double, 0.1 + 0.2 != 0.3. With agorot it is exact.
    Cents a = 0, b = 0, c = 0;
    ASSERT_TRUE(parseMoney("0.10", a));
    ASSERT_TRUE(parseMoney("0.20", b));
    ASSERT_TRUE(parseMoney("0.30", c));
    EXPECT_EQ(a + b, c);
}

TEST(FormatMoney, FormatsWithThousandsSeparatorAndTwoDecimals) {
    EXPECT_EQ(formatMoney(0), "0.00");
    EXPECT_EQ(formatMoney(5), "0.05");
    EXPECT_EQ(formatMoney(12050), "120.50");
    EXPECT_EQ(formatMoney(123456789), "1,234,567.89");
    EXPECT_EQ(formatMoney(-34050), "-340.50");
}
