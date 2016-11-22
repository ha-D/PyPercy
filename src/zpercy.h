#ifndef _Z_PERCY_
#define _Z_PERCY_

#include "agclient.h"
#include "agserver.h"
#include "percytypes.h"
#include "percyparams.h"
#include "agparams.h"
#include <sstream>

#define MAX_CHUNK_SiZE 10000

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

class ZPercyClient {
protected:
    const PercyParams* zparams;
    const PercyClientParams* zclientparams;
public:
    virtual nqueries_t make_request(dbsize_t block) = 0;
    virtual void parse_response(nqueries_t req_id, istream& resp_stream) = 0;
};


class ZAGClient : public PercyAGClient, public ZPercyClient {
protected:
    stringstream request_buffer;
    stringstream response_buffer;
    nqueries_t request_id;
public:
    ZAGClient(const PercyClientParams * params);
    virtual nqueries_t make_request(dbsize_t block);
    int read_request(char* buf, int chunk_size);

    virtual void parse_response(nqueries_t req_id, istream& resp_stream);
    virtual void parse_response(istream& resp_stream);
    int read_response(char* buf, int chunk_size);

    int x;
};


class ZPercyServer {
public:
    virtual void process_request(istream& req_stream) = 0;
};


class ZAGServer : public PercyAGServer, public ZPercyServer {
protected:
    stringstream response_buffer;
public:
    ZAGServer(DataStore * datastore,
              const PercyServerParams * params);
    virtual void process_request(istream& req_stream);
    int read_response(char* buf, int chunk_size);
};

extern "C" {
    // TODO Consider virtual block size
    ZAGClient* client_new(dbsize_t num_blocks, dbsize_t block_size, dbsize_t N,
                               dbsize_t word_size, int x);

    nqueries_t client_make_request(ZAGClient* self, dbsize_t block);
    int client_read_request(ZAGClient* self, char* buf, int chunk_size);
    void client_parse_response(ZAGClient* self, int req_id, char* raw_response, int length);
    int client_read_response(ZAGClient* self, char* buf, int chunk_size);


    ZAGServer* server_new(char* dbfile, int db_offset, dbsize_t num_blocks, dbsize_t block_size, dbsize_t N,
                          dbsize_t word_size);
    void server_process_request(ZAGServer* self, char* request, int request_length);
    int server_read_response(ZAGServer* self, char* buf, int chunk_size);

}
#endif
