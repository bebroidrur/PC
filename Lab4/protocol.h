#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>

using namespace std;

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

    const string IN_PROGRESS = "IN_PROGRESS";
    const string DONE = "DONE";
    const string NOT_STARTED = "NOT_STARTED";
    const string ALREADY_RUNNING = "ALREADY_RUNNING";

    const string ERROR_INVALID_COMMAND = "ERROR_INVALID_COMMAND";
    const string ERROR_INVALID_CONFIG = "ERROR_INVALID_CONFIG";
    const string ERROR_INVALID_DATA = "ERROR_INVALID_DATA";
    const string ERROR_NOT_READY = "ERROR_NOT_READY";
    const string ERROR_RESULT_NOT_READY = "ERROR_RESULT_NOT_READY";
}

#endif