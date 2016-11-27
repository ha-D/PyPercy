#include "zpercy.h"

#include <vector>
#include <streambuf>

#include <iostream>

ZPercyServer::ZPercyServer(PercyServer* server) {
    this->server = server;
}

void ZPercyServer::process_request(istream& req_steam) {
    // Clear buffer before new request
    this->response_buffer.str( std::string() );
    this->response_buffer.clear();

    std::vector<std::iostream*> workers;
    server->handle_request(req_steam, this->response_buffer, workers);
}

int ZPercyServer::read_response(char *buf, int chunk_size) {
    this->response_buffer.read(buf, chunk_size);
    if (this->response_buffer) {
        return chunk_size;
    } else {
        return this->response_buffer.gcount();
    }
}

void server_process_request(ZPercyServer* self, char* request, int request_length) {
    membuf buff(request, request + request_length);
    istream request_stream(&buff);
    self->process_request(request_stream);
}

int server_read_response(ZPercyServer* self, char* buf, int chunk_size) {
    if (chunk_size > MAX_CHUNK_SiZE) {
        chunk_size = MAX_CHUNK_SiZE;
    }

    return self->read_response(buf, chunk_size);
}
