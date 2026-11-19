#ifndef MONEY_H
#define MONEY_H

#include <string>

// Money is stored as a whole number of agorot (1 shekel = 100 agorot).
// Using an integer instead of double avoids floating-point rounding errors:
// with double, 0.1 + 0.2 is 0.30000000000000004, which is not acceptable for money.
using Cents = long long;

// The largest single amount the app accepts: 1 billion shekels. Far more than any household
// needs, and small enough that sums of many amounts can never overflow a long long.
constexpr Cents kMaxAmount = 1'000'000'000LL * 100;

// Parses a positive amount such as "120", "120.5" or "120.50" into agorot.
// Returns false (and leaves `out` unchanged) if the text is not a valid positive
// amount with at most two digits after the decimal point.
bool parseMoney(const std::string& text, Cents& out);

// Formats agorot as shekels with a thousands separator, e.g. 123450 -> "1,234.50".
std::string formatMoney(Cents amount);

#endif // MONEY_H
