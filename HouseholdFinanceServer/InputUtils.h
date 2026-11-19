#ifndef INPUT_UTILS_H
#define INPUT_UTILS_H

#include <string>

#include "Money.h"

// Helpers for reading user input safely from the console.
// Each function keeps asking until it gets a valid value, so invalid input
// (letters instead of numbers, impossible dates...) never breaks the program.

// Thrown when input ends (EOF, e.g. Ctrl+D), so the app can exit cleanly
// instead of looping forever on a closed input stream.
struct InputClosed {};

// Reads a non-empty line of text.
std::string readLine(const std::string& prompt);

// Reads a whole number in the range [min, max].
int readInt(const std::string& prompt, int min, int max);

// Reads a positive money amount, e.g. "250" or "99.90", and returns it in agorot.
Cents readMoney(const std::string& prompt);

// Reads a valid date in YYYY-MM-DD format.
std::string readDate(const std::string& prompt);

// Asks a yes/no question. Accepts 1 for yes and 2 for no.
bool readYesNo(const std::string& prompt);

#endif // INPUT_UTILS_H
