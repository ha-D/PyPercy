#include "zpercy.h"

stringstream errstream;
char error_message[1024];

char* get_err() {
    strcpy(error_message, errstream.str().c_str());
    errstream.str(std::string());
    errstream.clear();

    return error_message;
}
