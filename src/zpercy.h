#ifndef _Z_PERCY_
#define _Z_PERCY_

#include "percytypes.h"
#include "percyparams.h"
#include "percyserver.h"
#include "percyclient.h"

#include <sstream>
#include <map>

#define MAX_CHUNK_SiZE 10000

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

class ZRequest {
public:
    ZRequest(nqueries_t request_id, nservers_t num_servers, nqueries_t num_queries);
    ZRequest();
    nqueries_t request_id;
    nservers_t num_servers;
    nqueries_t num_queries;

    std::vector<std::stringstream*> request_buffers;
    std::vector<std::stringstream*> response_buffers;
    std::vector<std::stringstream*> result_buffers;

    void destroy();
};

class ZPercyClient  {
public:
    PercyClient* client;

    const PercyParams* zparams;
    const PercyClientParams* zclientparams;

    std::map<nqueries_t, ZRequest> requests;
    // std::vector<std::stringstream*> request_buffers;
    // std::vector<std::stringstream*> response_buffers;
    // std::vector<std::stringstream*> result_buffers;
    // nqueries_t request_id;

    nservers_t num_servers;

public:
    ZPercyClient(PercyClient* client, nservers_t num_servers);
    ~ZPercyClient();
    virtual nqueries_t make_request(std::vector<dbsize_t> blocks);
    int read_request(nqueries_t request_id, nservers_t sid, char* buf, int chunk_size);
    void write_response(nqueries_t request_id, nservers_t sid, char* buf, int chunk_size);
    virtual void parse_response(nqueries_t request_id);
    int read_result(nqueries_t request_id, nqueries_t query_number, char* buf, int chunk_size);
    void finish_request(nqueries_t request_id);
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
    nqueries_t client_make_request(ZPercyClient* self, dbsize_t* blocks, nqueries_t query_count);
    int client_read_request(ZPercyClient* self, nqueries_t request_id, nservers_t sid, char* buf, int chunk_size);
    void client_write_response(ZPercyClient* self, nqueries_t request_id, nservers_t sid, char* buf, int chunk_size);
    void client_parse_response(ZPercyClient* self, nqueries_t request_id);
    int client_read_result(ZPercyClient* self, nqueries_t request_id, nqueries_t query_number, char* buf, int chunk_size);
    void client_finish_request(ZPercyClient* self, nqueries_t request_id);

    void server_process_request(ZPercyServer* self, char* request, int request_length);
    int server_read_response(ZPercyServer* self, char* buf, int chunk_size);

    char* get_err();
}

extern std::stringstream errstream;
#endif
