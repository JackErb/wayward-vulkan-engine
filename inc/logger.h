#ifndef HerdLogger_h
#define HerdLogger_h

#include <string>
#include <iostream>
#include <stdexcept>

using std::cout;
using std::endl;
using std::cerr;

namespace logger {
    template <class T>
    void print(T msg) {
        cout << msg << endl;
    }

    template <class T>
    void error(T msg) {
        cerr << "[ERROR] " << msg << endl;
    }

    template <class T>
    void fatal_error(T msg) {
        error(msg);
        throw std::runtime_error(msg);
    }

    template <class T>
    void debug(T msg) {
        cout << "[DEBUG] " << msg << endl;
    }
};

#endif
