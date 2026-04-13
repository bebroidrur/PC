#ifndef PROTOCOL_H
#define PROTOCOL_H
using namespace std;
#include <string>

namespace Protocol {
    const string PING = "PING";
    const string PONG = "PONG";
    const string EXIT = "EXIT";
    const string OK = "OK";
    const string ERROR = "ERROR";

    const string CONFIG = "CONFIG";
    const string DATA = "DATA";
    const string START = "START";
    const string STATUS = "STATUS";
    const string RESULT = "RESULT";
}


#endif