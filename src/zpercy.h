#ifndef _Z_PERCY_
#define _Z_PERCY_

#include "percytypes.h"
#include "percyparams.h"
#include "percyserver.h"
#include "percyclient.h"

#include <sstream>

#define MAX_CHUNK_SiZE 10000

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

class ZPercyClient  {
protected:
    friend class PercyClient;
    PercyClient* client;

    const PercyParams* zparams;
    const PercyClientParams* zclientparams;

    std::stringstream request_buffer;
    std::stringstream response_buffer;
    nqueries_t request_id;

public:
    ZPercyClient(PercyClient* client);
    virtual nqueries_t make_request(dbsize_t block);
    int read_request(char* buf, int chunk_size);

    virtual void parse_response(nqueries_t req_id, std::istream& resp_stream);
    virtual void parse_response(std::istream& resp_stream);
    int read_response(char* buf, int chunk_size);
};

class ZPercyServer {
protected:
    PercyServer* server;
    std::stringstream response_buffer;
public:
    ZPercyServer(PercyServer* server);

    virtual void process_request(std::istream& req_stream);
    int read_response(char* buf, int chunk_size);

};

extern "C" {
    // TODO Consider virtual block size
    nqueries_t client_make_request(ZPercyClient* self, dbsize_t block);
    int client_read_request(ZPercyClient* self, char* buf, int chunk_size);
    void client_parse_response(ZPercyClient* self, int req_id, char* raw_response, int length);
    int client_read_response(ZPercyClient* self, char* buf, int chunk_size);


    void server_process_request(ZPercyServer* self, char* request, int request_length);
    int server_read_response(ZPercyServer* self, char* buf, int chunk_size);
}
#endif
