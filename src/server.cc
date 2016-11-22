#include "zpercy.h"

#include <vector>
#include <streambuf>

#include <iostream>

ZAGServer::ZAGServer(DataStore * datastore,
                     const PercyServerParams * params) : PercyAGServer(datastore, params, NULL), ZPercyServer() {
}

void ZAGServer::process_request(istream& req_steam) {
    // Clear buffer before new request
    this->response_buffer.str( std::string() );
    this->response_buffer.clear();

    std::vector<std::iostream*> workers;
    this->handle_request(req_steam, this->response_buffer, workers);
}

int ZAGServer::read_response(char *buf, int chunk_size) {
    this->response_buffer.read(buf, chunk_size);
    if (this->response_buffer) {
        return chunk_size;
    } else {
        return this->response_buffer.gcount();
    }
}


ZAGServer* server_new(char* dbfile, int db_offset, dbsize_t num_blocks, dbsize_t block_size,
                      dbsize_t N, dbsize_t word_size) {

    bool is_null = false;

    PercyParams * params =  new AGParams(num_blocks, block_size, N, word_size);
    PercyServerParams * serverparams =
            new PercyServerParams(params, 1, 0, DIST_SPLIT_RECORDS, 0, DIST_SPLIT_RECORDS,
                                  std::vector<nservers_t>(), 1);

    DataStore* datastore = new FileDataStore(dbfile, serverparams, db_offset);

    ZAGServer* server = new ZAGServer(datastore, serverparams);
    
    return server;
}



void server_process_request(ZAGServer* self, char* request, int request_length) {
    membuf buff(request, request + request_length);
    istream request_stream(&buff);
    self->process_request(request_stream);
}

int server_read_response(ZAGServer* self, char* buf, int chunk_size) {
    if (chunk_size > MAX_CHUNK_SiZE) {
        chunk_size = MAX_CHUNK_SiZE;
    }

    return self->read_response(buf, chunk_size);
}
