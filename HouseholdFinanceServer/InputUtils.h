#ifndef INPUT_UTILS_H
#define INPUT_UTILS_H

#include <string>

// פונקציות עזר לקריאת קלט בטוחה מהמשתמש.
// כל פונקציה ממשיכה לבקש קלט עד שמתקבל ערך תקין.
// אם הקלט נגמר (EOF, למשל Ctrl+D), נזרקת חריגה מסוג InputClosed.

struct InputClosed {};

// קריאת שורה שלמה (לא ריקה) מהמשתמש
std::string readLine(const std::string& prompt);

// קריאת מספר שלם בטווח [min, max]
int readInt(const std::string& prompt, int min, int max);

// קריאת מספר חיובי (גדול מ-0)
double readPositiveDouble(const std::string& prompt);

// קריאת תאריך בפורמט YYYY-MM-DD
std::string readDate(const std::string& prompt);

// בדיקה האם מחרוזת היא תאריך תקין בפורמט YYYY-MM-DD
bool isValidDate(const std::string& date);

#endif // INPUT_UTILS_H
