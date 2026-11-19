#include <fstream>

#include "HouseholdFinance.h"

int main() {
    HouseholdFinance ourHome;
    ourHome.loadFromFile();

    // נתוני דוגמה בלבד, נטענים רק אם קובץ הנתונים ריק
    std::ifstream file("finance_data.txt");
    if (file.peek() == std::ifstream::traits_type::eof()) {
        ourHome.addTransaction("משכורת א", 10000.0, "שכר", false, "2026-10-01");
        ourHome.addTransaction("משכורת ב", 9000.0, "שכר", false, "2026-10-01");
        ourHome.addTransaction("תשלום שכירות", 4500.0, "מגורים", true, "2026-10-02");
    }
    file.close();

    ourHome.runMenu();
    return 0;
}
