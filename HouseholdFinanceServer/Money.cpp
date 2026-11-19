#include "Money.h"

#include <cctype>

bool parseMoney(const std::string& text, Cents& out) {
    if (text.empty()) return false;

    Cents shekels = 0;
    Cents agorot = 0;
    int fractionDigits = 0;
    bool seenDot = false;
    bool seenDigit = false;

    for (char c : text) {
        if (c == '.') {
            if (seenDot) return false;  // "1.2.3"
            seenDot = true;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            seenDigit = true;
            int digit = c - '0';
            if (seenDot) {
                if (fractionDigits == 2) return false;  // "1.234" - more than 2 decimals
                agorot = agorot * 10 + digit;
                fractionDigits++;
            } else {
                shekels = shekels * 10 + digit;
                if (shekels * 100 > kMaxAmount) return false;
            }
        } else {
            return false;  // letters, minus sign, spaces, commas...
        }
    }

    if (!seenDigit) return false;    // "." alone
    if (fractionDigits == 1) agorot *= 10;  // "12.5" means 12.50

    Cents total = shekels * 100 + agorot;
    if (total <= 0 || total > kMaxAmount) return false;

    out = total;
    return true;
}

std::string formatMoney(Cents amount) {
    bool negative = amount < 0;
    if (negative) amount = -amount;

    std::string shekels = std::to_string(amount / 100);
    // Insert a comma every three digits from the right: 1234567 -> 1,234,567
    for (int i = static_cast<int>(shekels.size()) - 3; i > 0; i -= 3) {
        shekels.insert(static_cast<size_t>(i), ",");
    }

    Cents agorot = amount % 100;
    std::string result = shekels + "." + (agorot < 10 ? "0" : "") + std::to_string(agorot);
    return negative ? "-" + result : result;
}
