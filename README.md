# HouseholdFinanceTracker

A C++ command-line application for tracking household income and expenses, with per-category budgets and a savings-goal calculator.

> **Status:** in progress. The core features work; see the Roadmap below for what is planned.

## Features

- Add income and expense transactions (description, amount, category, date)
- Persistent storage in a local text file (`finance_data.txt`)
- Per-category monthly budgets with an over-budget warning
- Financial report: total income, total expenses, net balance, and budget status per category
- Savings-goal calculator: estimates how many months it takes to reach a target amount
- Basic input validation and sanitizing of values written to the data file

## Tech

- C++ (STL: `vector`, `map`, `fstream`, `sstream`)
- Object-oriented design (`Transaction` struct, `HouseholdFinance` class), split into header and source files
- Xcode project included; also builds from the command line

## Project structure

    HouseholdFinanceServer/
    ├── Transaction.h          # Transaction struct
    ├── HouseholdFinance.h     # HouseholdFinance class declaration
    ├── HouseholdFinance.cpp   # HouseholdFinance implementation
    └── main.cpp               # entry point

## Build and run

**With Xcode:** open `HouseholdFinanceServer.xcodeproj` and press Run.

**From the terminal (macOS/Linux):**

    clang++ -std=c++17 HouseholdFinanceServer/*.cpp -o finance_tracker
    ./finance_tracker

On first run, a few sample transactions are created. Data is saved to `finance_data.txt` in the working directory (this file is git-ignored).

## Roadmap

- [x] Split the code into separate files (`Transaction.h`, `HouseholdFinance.h/.cpp`, `main.cpp`)
- [ ] Robust input handling (invalid numbers, malformed lines in the data file)
- [ ] Filter by month so the savings calculation uses monthly figures
- [ ] Unit tests
- [ ] Configurable budgets (load from a file instead of hardcoded values)
- [ ] Edit and delete transactions

## Author

Guy Tzur, Computer Science student at Colman.
