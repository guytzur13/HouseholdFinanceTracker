# HouseholdFinanceTracker

![CI](https://github.com/guytzur13/HouseholdFinanceTracker/actions/workflows/ci.yml/badge.svg)

A C++17 command-line application for managing a household's monthly budget: recording income and expenses, tracking spending against per-category budgets, and forecasting how long it will take to reach a savings goal.

I built it for my brother and his partner to manage their household finances, and used it to practise writing C++ the way it is written in real projects: separated layers, input validation, unit tests and continuous integration.

## Features

- **Income and expenses**: add, edit and delete transactions (description, amount, category, date)
- **Monthly report**: income, expenses and net balance for any month, with spending per category
- **Category budgets**: set a monthly budget per category; get a warning as soon as an expense goes over it
- **Savings forecast**: estimates how many months it will take to reach a goal, based on the average monthly savings
- **Persistent storage**: data is saved to CSV files after every change, with atomic writes so a crash cannot corrupt the data
- **Robust input handling**: invalid numbers, menu choices and dates are rejected with a clear message; malformed lines in the data files are skipped with a warning instead of crashing the program

## Example

```
=========================================
דו"ח חודשי: 2026-11
=========================================
הכנסות:  19,000.00 ש"ח
הוצאות:  6,840.50 ש"ח
נטו:     12,159.50 ש"ח
-----------------------------------------
מצב תקציבים:
* אוכל: 2,340.50 / 2,000.00 ש"ח  <-- חריגה של 340.50 ש"ח
* בילויים: 0.00 / 1,500.00 ש"ח
* כללי: 0.00 / 1,000.00 ש"ח
* מגורים: 4,500.00 / 6,000.00 ש"ח
=========================================
```

(The user interface is in Hebrew, since the app was built for a Hebrew-speaking household.)

## Architecture

The code is split into layers, each with one responsibility:

```
main.cpp ──> ConsoleApp ──> FinanceManager ──> Transaction, Money, Validation
                  │
                  └──────> FileStorage
```

| File | Responsibility |
|---|---|
| `Transaction.h` | The `Transaction` struct and the `TransactionType` enum |
| `Money.h/.cpp` | Parsing and formatting amounts; money is stored as integer agorot |
| `Validation.h/.cpp` | Pure validation helpers: dates (including leap years), text sanitizing |
| `FinanceManager.h/.cpp` | Business logic: transactions, monthly summaries, budgets, savings forecast. **No input/output at all** |
| `FileStorage.h/.cpp` | Loading and saving CSV files |
| `InputUtils.h/.cpp` | Reading validated input from the console |
| `ConsoleApp.h/.cpp` | The menu and all printing |
| `main.cpp` | Wires everything together |

### Design decisions

- **Money as integers, not `double`.** Amounts are stored in agorot as `long long`. Floating-point numbers cannot represent most decimal fractions exactly (`0.1 + 0.2 != 0.3`), which leads to rounding errors in financial sums. Integers are exact.
- **Business logic separated from I/O.** `FinanceManager` never touches `cin`, `cout` or files. That is what makes it unit-testable, and it means the same logic could later be used behind a GUI or a web API without changes.
- **Validation in one place.** `FinanceManager` validates every transaction and throws `std::invalid_argument` on bad data, so invalid data cannot get in regardless of where it comes from.
- **Atomic saves.** Data is written to a temporary file which then replaces the real file with `std::rename`, so an interrupted save never leaves a half-written file.
- **Forgiving loading.** A corrupted line in a data file is skipped and reported, rather than crashing the program or discarding all the data.

## Build and run

Requirements: a C++17 compiler and CMake 3.16+.

```bash
cmake -S . -B build
cmake --build build
./build/finance_tracker
```

On the first run, sample data for two months is created. Data is stored in `transactions.csv` and `budgets.csv` in the working directory (both are git-ignored).

An Xcode project is also included: open `HouseholdFinanceServer.xcodeproj` and press Run.

## Tests

The logic is covered by unit tests written with [GoogleTest](https://github.com/google/googletest) (downloaded automatically by CMake):

```bash
ctest --test-dir build --output-on-failure
```

The tests cover money parsing and formatting, date validation (including leap years), monthly summaries, budget overruns, the savings forecast, editing and deleting, and saving and loading files, including corrupted input. GitHub Actions runs the build and all tests on Linux and macOS on every push.

## Roadmap

- [x] Split the code into separate files and layers
- [x] Robust input handling
- [x] Monthly reports and per-month budgets
- [x] Edit and delete transactions
- [x] Configurable budgets saved to a file
- [x] Unit tests and CI
- [ ] Recurring transactions (e.g. rent every month)
- [ ] Export a monthly report to a file
- [ ] REST API (e.g. with the Drogon framework) and a web interface

## Author

Guy Tzur, Computer Science student at Colman.
