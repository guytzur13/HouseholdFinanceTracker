#include "Validation.h"

#include <cctype>

namespace {

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

bool allDigits(const std::string& text) {
    for (char c : text) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return !text.empty();
}

} // namespace

bool isValidMonth(const std::string& month) {
    if (month.size() != 7 || month[4] != '-') return false;
    std::string yearPart = month.substr(0, 4);
    std::string monthPart = month.substr(5, 2);
    if (!allDigits(yearPart) || !allDigits(monthPart)) return false;

    int year = std::stoi(yearPart);
    int m = std::stoi(monthPart);
    return year >= 1900 && year <= 2100 && m >= 1 && m <= 12;
}

bool isValidDate(const std::string& date) {
    if (date.size() != 10 || date[7] != '-') return false;
    if (!isValidMonth(date.substr(0, 7))) return false;
    std::string dayPart = date.substr(8, 2);
    if (!allDigits(dayPart)) return false;

    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(dayPart);

    const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[month - 1];
    if (month == 2 && isLeapYear(year)) maxDay = 29;

    return day >= 1 && day <= maxDay;
}

std::string trim(const std::string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

std::string sanitizeText(const std::string& text) {
    std::string result;
    for (char c : text) {
        if (c != ',' && c != '\n' && c != '\r') result += c;
    }
    return trim(result);
}
