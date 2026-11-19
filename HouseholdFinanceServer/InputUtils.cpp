#include "InputUtils.h"

#include <iostream>

#include "Validation.h"

namespace {

// Reads one raw line. Throws InputClosed if there is no more input.
std::string getRawLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw InputClosed{};
    }
    return trim(line);
}

} // namespace

std::string readLine(const std::string& prompt) {
    while (true) {
        std::string line = sanitizeText(getRawLine(prompt));
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
            // pos != line.size() means there was extra text after the number, e.g. "3abc"
            if (pos == line.size() && value >= min && value <= max) {
                return value;
            }
        } catch (const std::exception&) {
            // std::stoi throws if the text is not a number or is too large
        }
        std::cout << "[X] יש להקליד מספר שלם בין " << min << " ל-" << max << "." << std::endl;
    }
}

Cents readMoney(const std::string& prompt) {
    while (true) {
        std::string line = getRawLine(prompt);
        Cents amount = 0;
        if (parseMoney(line, amount)) return amount;
        std::cout << "[X] יש להקליד סכום חיובי עם עד 2 ספרות אחרי הנקודה (למשל 250 או 99.90)."
                  << std::endl;
    }
}

std::string readDate(const std::string& prompt) {
    while (true) {
        std::string line = getRawLine(prompt);
        if (isValidDate(line)) return line;
        std::cout << "[X] תאריך לא תקין. הפורמט הוא שנה-חודש-יום, למשל 2026-11-19." << std::endl;
    }
}

bool readYesNo(const std::string& prompt) {
    return readInt(prompt + " (1 = כן, 2 = לא): ", 1, 2) == 1;
}
