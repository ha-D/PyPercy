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
public:
    PercyClient* client;

    const PercyParams* zparams;
    const PercyClientParams* zclientparams;

    std::vector<std::stringstream*> request_buffers;
    std::vector<std::stringstream*> response_buffers;
    std::vector<std::stringstream*> result_buffers;
    nqueries_t request_id;
    nservers_t num_servers;

public:
    ZPercyClient(PercyClient* client, nservers_t num_servers);
    ~ZPercyClient();
    virtual nqueries_t make_request(std::vector<dbsize_t> blocks);
    int read_request(nservers_t sid, char* buf, int chunk_size);

    void write_response(nservers_t sid, char* buf, int chunk_size);

    virtual void parse_response(nqueries_t req_id);
    virtual void parse_response();
    int read_result(int query_number, char* buf, int chunk_size);
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
    nqueries_t client_make_request(ZPercyClient* self, dbsize_t* blocks, int query_count);
    int client_read_request(ZPercyClient* self, nservers_t sid, char* buf, int chunk_size);
    void client_write_response(ZPercyClient* self, nservers_t sid, char* buf, int chunk_size);
    void client_parse_response(ZPercyClient* self, int req_id);
    int client_read_result(ZPercyClient* self, int query_number, char* buf, int chunk_size);


    void server_process_request(ZPercyServer* self, char* request, int request_length);
    int server_read_response(ZPercyServer* self, char* buf, int chunk_size);

    char* get_err();
}

extern std::stringstream errstream;
#endif
