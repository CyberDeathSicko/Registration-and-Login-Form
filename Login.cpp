#include<iostream>
#include<fstream>
using namespace std;

class temp{
    string userName, Email, password;
    string searchName, searchPass, searchEmail;
    fstream file;
public:  
    void login();
    void signUP();
    void forgot();
};

int main(){
    char choice;
    temp obj;  
    cout << "\n1- Login";
    cout << "\n2- Sign-Up";
    cout << "\n3- Forgot Password";
    cout << "\n4- Exit";
    cout << "\nEnter Your Choice :: ";
    cin >> choice;

    switch(choice){
        case '1':
            cin.ignore();
            obj.login();
            break;
        case '2':
            cin.ignore();
            obj.signUP();
            break;
        case '3':
            cin.ignore();
            obj.forgot();
            break;
        case '4':
            return 0;
        default:  
            cout << "Invalid Selection...!";
    }
}

void temp :: signUP(){
    cout << "\nEnter Your User Name :: ";
    cin.ignore();  
    getline(cin, userName);
    cout << "Enter Your Email Address :: ";
    getline(cin, Email);
    cout << "Enter Your Password :: ";
    getline(cin, password);

    file.open("loginData.txt", ios::out | ios::app);
    file << userName << "*" << Email << "*" << password << endl;
    file.close();
}

void temp :: login(){
    cout << "----------LOGIN---------" << endl;
    cout << "Enter Your User Name :: " << endl;
    cin.ignore();  
    getline(cin, searchName);
    cout << "Enter Your Password :: " << endl;
    getline(cin, searchPass);

    file.open("loginData.txt", ios::in);
    while (getline(file, userName, '*') && getline(file, Email, '*') && getline(file, password, '\n')) {
        if (userName == searchName) {
            if (password == searchPass) {
                cout << "\nAccount Login Successful...!";
                cout << "\nUsername :: " << userName << endl;
                cout << "\nEmail :: " << Email << endl;
                return; 
            } else {
                cout << "Password is Incorrect...!";
                return;  
            }
        }
    }
    cout << "Account not found...!" << endl;
    file.close();
}

void temp :: forgot(){
    cout << "\nEnter Your UserName :: ";
    cin.ignore();  
    getline(cin, searchName);
    cout << "\nEnter Your Email Address :: ";
    getline(cin, searchEmail);

    file.open("loginData.txt", ios::in);
    while (getline(file, userName, '*') && getline(file, Email, '*') && getline(file, password, '\n')) {
        if (userName == searchName && Email == searchEmail) {
            cout << "\nAccount Found...!" << endl;
            cout << "Your Password :: " << password << endl;
            file.close();
            return;  
        }
    }
    cout << "\nAccount not found...!" << endl;
    file.close();
}