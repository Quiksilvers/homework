#ifndef UNTITLED_TRANSACTION_H
#define UNTITLED_TRANSACTION_H

#include <string>

using namespace std;

class Transaction {

private:
    string accountId;
    double amount;
    string date;
    string details;

public:
    Transaction(const string &accId, double amnt, const string &dt, const string &det) : accountId(accId), amount(amnt), date(dt), details(det) {}

    string getAccountId() const {return accountId;}
    double getAmount() const {return amount;}
    string getDate() const {return date;}
    string getDetails() const {return details;}

};

#endif //UNTITLED_TRANSACTION_H
