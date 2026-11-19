#ifndef VALIDATION_H
#define VALIDATION_H

#include <string>

// Pure validation helpers, with no input/output. Kept separate from InputUtils
// so that the business logic and the file storage can use them too, and so they
// are easy to unit test.

// True if `date` is a real calendar date in YYYY-MM-DD format (handles leap years).
bool isValidDate(const std::string& date);

// True if `month` is in YYYY-MM format, e.g. "2026-11".
bool isValidMonth(const std::string& month);

// Removes leading and trailing whitespace.
std::string trim(const std::string& text);

// Prepares free text for storage: removes characters that would break the
// CSV file format (commas and line breaks) and trims whitespace.
std::string sanitizeText(const std::string& text);

#endif // VALIDATION_H
