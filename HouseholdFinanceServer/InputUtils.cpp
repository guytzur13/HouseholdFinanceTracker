#include "InputUtils.h"

#include <cctype>
#include <iostream>

namespace {

// מחזירה את המחרוזת בלי רווחים בהתחלה ובסוף
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// קוראת שורה גולמית, וזורקת InputClosed אם הקלט נגמר
std::string getRawLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw InputClosed{};
    }
    return trim(line);
}

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

} // namespace

std::string readLine(const std::string& prompt) {
    while (true) {
        std::string line = getRawLine(prompt);
        if (!line.empty()) return line;
        std::cout << "[X] הקלט לא יכול להיות ריק, נסה שוב." << std::endl;
    }
}

int readInt(const std::string& prompt, int min, int max) {
    while (true) {
        std::string line = getRawLine(prompt);
        try {
            size_t pos = 0;
            int value = std::stoi(line, &pos);
            // pos != line.size() אומר שהיו תווים נוספים אחרי המספר, למשל "3abc"
            if (pos == line.size() && value >= min && value <= max) {
                return value;
            }
        } catch (const std::exception&) {
            // stoi זורקת חריגה אם זה בכלל לא מספר או שהוא גדול מדי
        }
        std::cout << "[X] יש להקליד מספר שלם בין " << min << " ל-" << max << "." << std::endl;
    }
}

double readPositiveDouble(const std::string& prompt) {
    while (true) {
        std::string line = getRawLine(prompt);
        try {
            size_t pos = 0;
            double value = std::stod(line, &pos);
            if (pos == line.size() && value > 0) {
                return value;
            }
        } catch (const std::exception&) {
        }
        std::cout << "[X] יש להקליד מספר גדול מ-0 (למשל 250 או 99.90)." << std::endl;
    }
}

bool isValidDate(const std::string& date) {
    // פורמט: YYYY-MM-DD, בדיוק 10 תווים
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    }

    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));

    if (year < 1900 || year > 2100) return false;
    if (month < 1 || month > 12) return false;

    const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[month - 1];
    if (month == 2 && isLeapYear(year)) maxDay = 29;

    return day >= 1 && day <= maxDay;
}

std::string readDate(const std::string& prompt) {
    while (true) {
        std::string line = getRawLine(prompt);
        if (isValidDate(line)) return line;
        std::cout << "[X] תאריך לא תקין. הפורמט הוא שנה-חודש-יום, למשל 2026-11-19." << std::endl;
    }
}
