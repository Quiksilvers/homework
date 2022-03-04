#ifndef UNTITLED_ACCOUNT_H
#define UNTITLED_ACCOUNT_H

#include <string>

using namespace std;

class Account {

private:
    string id;
    double balance;

public:
    Account(const string &i, double bal) : id(i), balance(bal) {}

    string getId() const {return id;}
    double getBalance() const {return balance;}
    void addBalance(double bal) {balance+= bal;}

};

#endif //UNTITLED_ACCOUNT_H
