#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iterator>
#include <filesystem>
#include <sqlite3.h>
#include <boost/tokenizer.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "Account.h"
#include "Transaction.h"

using namespace std;
using namespace boost;

// Gets actual characters count instead of byte count encoded in UTF-8 format
int getUtf8StringLength(const string &str) {
    return count_if(str.begin(), str.end(), [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; } );
}

static int callback(void *passedVal, int argc, char **argv, char **azColName) {
    auto *accList = (vector<Account> *)passedVal;
    string accId;
    double balance = 0.0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(azColName[i], "account_id") == 0)
            accId = argv[i];
        else if (strcmp(azColName[i], "balance") == 0)
            balance = strtod(argv[i], nullptr);
    }
    accList->push_back(Account(accId, balance));
    return 0;
}

void writeToFile(const string &file, const string &line) {
    ofstream fileStream;
    fileStream.open (file, ios::out | ios::app);
    fileStream << line << '\n';
    fileStream.close();
}

void writeToLogFile(const string &line) {
    const string logFile = "log.txt";
    struct tm * timeInfo;
    char buffer [80];

    time_t currTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
    timeInfo = localtime (&currTime);
    strftime (buffer,80,"%Y-%m-%d %H:%M:%S",timeInfo);

    string dateTime = "[" + string(buffer) + "] ";
    writeToFile(logFile, dateTime + line);
}

void deleteFile(const string &dbFileName) {
    try {
        if (std::filesystem::remove(dbFileName))
            writeToLogFile("File " + dbFileName + " deleted");
        else
            writeToLogFile("File " + dbFileName + " not found");
    }
    catch (const std::filesystem::filesystem_error &err) {
        writeToLogFile("Filesystem error: " + string(err.what()));
        //return 1;
    }
}

sqlite3 * openDatabase(const string &dbFileName) {
    sqlite3 *db;
    int rc = sqlite3_open(dbFileName.c_str(), &db);

    if (rc) {
        writeToLogFile("Can't open database: " + string(sqlite3_errmsg(db)));
        //return 1;
    } else {
        writeToLogFile("Opened database successfully");
    }
    return db;
}

void createAccountsTable(sqlite3 *db) {
    const std::string sql =
            "create table accounts ( "
            "account_id varchar(2), "
            "balance float default 0.0 ); ";

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if ( rc != SQLITE_OK ) {
        writeToLogFile("SQL error: " + string(errMsg));
        sqlite3_free(errMsg);
        //return 1;
    } else {
        writeToLogFile("Table accounts created successfully");
    }
}

void createTransactionsTable(sqlite3 *db) {
    const std::string sql =
            "create table transactions ( "
            "account_id varchar(2), "
            "amount float, "
            "transaction_date text, "
            "transaction_details varchar(3) ); ";

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if ( rc != SQLITE_OK ) {
        writeToLogFile("SQL error: " + string(errMsg));
        sqlite3_free(errMsg);
        //return 1;
    } else {
        writeToLogFile("Table transactions created successfully");
    }
}

void addInitialAccounts(sqlite3 *db) {
    const std::string sql =
            "insert into accounts (account_id) values ('01'); "
            "insert into accounts (account_id) values ('02'); "
            "insert into accounts (account_id) values ('03'); ";

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if ( rc != SQLITE_OK ) {
        writeToLogFile("SQL error: " + string(errMsg));
        sqlite3_free(errMsg);
    } else {
        writeToLogFile("Records successfully inserted into accounts table");
    }
}

vector<Account> readAccounts(sqlite3 *db) {
    const std::string sql =
            "select * from accounts; ";

    vector<Account> accList;
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), callback, &accList, &errMsg);

    if ( rc != SQLITE_OK ) {
        writeToLogFile("SQL error: " + string(errMsg));
        sqlite3_free(errMsg);
    } else {
        writeToLogFile("Records successfully selected from accounts table");
    }

    return accList;
}

vector<Transaction> parseTransactions(const vector<string> &files, vector<Account> &accList) {
    const string importedTransFile = "transactions-imported.csv";
    const string skippedTransFile = "transactions-err.csv";

    deleteFile(importedTransFile);
    deleteFile(skippedTransFile);

    vector<Transaction> transList;
    for (const auto &file : files) {
        ifstream in(file.c_str());
        if (in.is_open()) {
            writeToLogFile("File " + file + " successfully opened");
        } else {
           //return 1;
        }

        int importedCnt = 0;
        int skippedCnt = 0;
        vector<string> vec;
        string line;
        int lineNum = 0;

        while (getline(in, line)) {
            lineNum++;
            tokenizer<escaped_list_separator<char> > tok(line);
            vec.assign(tok.begin(), tok.end()); // Vector now contains strings from one row

            if (vec.size() != 4) {
                writeToLogFile("Error while processing file " + file + " line " + to_string(lineNum) + " (" + line + "): incorrect value count - expected 4, got " + to_string(vec.size()));
                writeToFile(skippedTransFile, line);
                skippedCnt++;
                continue;
            }

            string accId = vec.at(0);
            if (getUtf8StringLength(accId) > 2) {
                writeToLogFile("Error while processing file " + file + " line " + to_string(lineNum) + " (" + line + "): account id length is more that 2 characters");
                writeToFile(skippedTransFile, line);
                skippedCnt++;
                continue;
            }

            double amt;
            try {
                amt = std::stod(vec.at(1));
            } catch (const std::invalid_argument&) {
                writeToLogFile("Error while processing file " + file + " line " + to_string(lineNum) + " (" + line + "): amount value is invalid");
                writeToFile(skippedTransFile, line);
                skippedCnt++;
                continue;
            } catch (const std::out_of_range&) {
                writeToLogFile("Error while processing file " + file + " line " + to_string(lineNum) + " (" + line + "): amount value is out of range for a double");
                writeToFile(skippedTransFile, line);
                skippedCnt++;
                continue;
            }

            string dt = vec.at(2);
            try {
                gregorian::date gregDt = gregorian::from_string(dt);
            } catch (...) {
                writeToLogFile("Error while processing file " + file + " line " + to_string(lineNum) + " (" + line + "): date value is invalid");
                writeToFile(skippedTransFile, line);
                skippedCnt++;
                continue;
            }

            string det = vec.at(3);
            if (getUtf8StringLength(det) > 3) {
                writeToLogFile("Error while processing file " + file + " line " + to_string(lineNum) + " (" + line + "): details length is more that 3 characters");
                writeToFile(skippedTransFile, line);
                skippedCnt++;
                continue;
            }

            auto it = find_if(accList.begin(), accList.end(), [&accId](const Account &a) { return a.getId() == accId; });
            if (it != accList.end()) {
                it->addBalance(amt);
                transList.push_back(Transaction(accId, amt, dt, det));
                writeToFile(importedTransFile, line);
                importedCnt++;
                //continue;
            } else {
                writeToLogFile("Error while processing file " + file + " line " + to_string(lineNum) + " (" + line + "): account with id = " + accId + " does not exist");
                writeToFile(skippedTransFile, line);
                skippedCnt++;
                //continue;
            }
        }

        writeToLogFile(file + " successfully imported row(s) " + to_string(importedCnt) + " skipped row(s) " + to_string(skippedCnt));
    }

    return transList;
}

void saveTransactions(sqlite3 *db, const vector<Transaction> &transList) {
    if (transList.empty()) {
        return;
    }

    string sql;
    for (const auto &trans : transList) {
        sql += "insert into transactions (account_id, amount, transaction_date, transaction_details) values "
               "('" + trans.getAccountId() + "', " + to_string(trans.getAmount()) + ", '" + trans.getDate() + "', '"+trans.getDetails() + "'); ";
    }

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if ( rc != SQLITE_OK ) {
        writeToLogFile("SQL error: " + string(errMsg));
        sqlite3_free(errMsg);
    } else {
        writeToLogFile("Records successfully inserted into transactions table");
    }
}

void updateAccountBalances(sqlite3 *db, const vector<Account> &accList) {
    if (accList.empty()) {
        return;
    }

    string sql;
    for (const auto &acc : accList) {
        sql += "update accounts set balance = " + to_string(acc.getBalance()) + " where account_id = '" + acc.getId() + "'; ";
    }

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if ( rc != SQLITE_OK ) {
        writeToLogFile("SQL error: " + string(errMsg));
        sqlite3_free(errMsg);
    } else {
        writeToLogFile("Records in accounts table successfully updated");
    }
}

int main() {
    writeToLogFile("Application started");

    const string dbFileName = "database.sqlite";
    const vector<string> transFiles {"transactions1.csv", "transactions2.csv"};

    deleteFile(dbFileName);
    sqlite3 *db = openDatabase(dbFileName);
    createAccountsTable(db);
    createTransactionsTable(db);
    addInitialAccounts(db);
    vector<Account> accounts = readAccounts(db);
    vector<Transaction> transactions = parseTransactions(transFiles, accounts);
    saveTransactions(db, transactions);
    updateAccountBalances(db, accounts);

    sqlite3_close(db);

    writeToLogFile("Application closing");

    return 0;
}
