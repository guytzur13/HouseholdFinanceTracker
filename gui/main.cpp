#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Used by QSettings to remember the data folder, and for the app data location.
    QApplication::setOrganizationName("GuyTzur");
    QApplication::setApplicationName("HouseholdFinance");
    QApplication::setApplicationDisplayName("ניהול תקציב משק הבית");

    // The interface is in Hebrew, so lay everything out right to left.
    QApplication::setLayoutDirection(Qt::RightToLeft);

    MainWindow window;
    window.show();
    return app.exec();
}
