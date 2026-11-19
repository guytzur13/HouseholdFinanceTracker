#include <gtest/gtest.h>

#include "Validation.h"

TEST(IsValidDate, AcceptsRealDates) {
    EXPECT_TRUE(isValidDate("2026-11-19"));
    EXPECT_TRUE(isValidDate("2026-01-31"));
    EXPECT_TRUE(isValidDate("2024-02-29"));  // leap year
    EXPECT_TRUE(isValidDate("2000-02-29"));  // divisible by 400 -> leap year
}

TEST(IsValidDate, RejectsImpossibleOrMalformedDates) {
    EXPECT_FALSE(isValidDate("2026-02-29"));  // not a leap year
    EXPECT_FALSE(isValidDate("1900-02-29"));  // divisible by 100 but not 400
    EXPECT_FALSE(isValidDate("2026-04-31"));
    EXPECT_FALSE(isValidDate("2026-13-01"));
    EXPECT_FALSE(isValidDate("2026-00-10"));
    EXPECT_FALSE(isValidDate("2026-11-00"));
    EXPECT_FALSE(isValidDate("19-11-2026"));
    EXPECT_FALSE(isValidDate("2026/11/19"));
    EXPECT_FALSE(isValidDate("2026-1-19"));
    EXPECT_FALSE(isValidDate("abcd-ef-gh"));
    EXPECT_FALSE(isValidDate(""));
}

TEST(IsValidMonth, ChecksFormatAndRange) {
    EXPECT_TRUE(isValidMonth("2026-11"));
    EXPECT_FALSE(isValidMonth("2026-13"));
    EXPECT_FALSE(isValidMonth("2026-1"));
    EXPECT_FALSE(isValidMonth("202611"));
}

TEST(SanitizeText, RemovesCharactersThatWouldBreakTheCsvFile) {
    EXPECT_EQ(sanitizeText("  hello, world\n"), "hello world");
    EXPECT_EQ(sanitizeText("a,b,c"), "abc");
    EXPECT_EQ(sanitizeText("line1\r\nline2"), "line1line2");
    EXPECT_EQ(sanitizeText(" , "), "");
}
