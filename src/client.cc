#include "zpercy.h"
#include <vector>

#include <iostream>
#include <sstream>

using namespace std;

char* zbuffer = new char[MAX_CHUNK_SiZE];

ZRequest::ZRequest(nqueries_t request_id, nservers_t num_servers, nqueries_t num_queries):
request_id(request_id), num_servers(num_servers), num_queries(num_queries) {
    for (int i = 0; i < num_servers; i++) {
        this->request_buffers.push_back(new std::stringstream());
        this->response_buffers.push_back(new std::stringstream());
    }

    for (int i = 0; i < num_queries; i++) {
        this->result_buffers.push_back(new std::stringstream());
    }
}
ZRequest::ZRequest() {
}
void ZRequest::destroy() {
    for (int i = 0; i < this->request_buffers.size(); i++) {
        delete this->request_buffers[i];
        delete this->response_buffers[i];
    }

    for (int i = 0; i < this->result_buffers.size(); i++) {
        delete this->result_buffers[i];
    }
}

// TODO not sure about these PercyClient constructor params, taken from AGClient
ZPercyClient::ZPercyClient(PercyClient* client, nservers_t num_servers):
client(client), num_servers(num_servers) {
}
ZPercyClient::~ZPercyClient() {
}

nqueries_t ZPercyClient::make_request(std::vector<dbsize_t> blocks) {
    nqueries_t querybsize = 1;

    //std::vector<dbsize_t> blocks (1, block);
    nqueries_t request_identifier = client->encode_request(blocks, querybsize);

    ZRequest request(request_identifier, this->num_servers, blocks.size());

    std::vector<std::ostream*> ostreams;// (1, &this->request_buffer);
    for (int i = 0; i < request.request_buffers.size(); i++) {
        ostreams.push_back(request.request_buffers[i]);
    }
    client->send_request(request_identifier, ostreams);

    this->requests[request_identifier] = request;

    return request_identifier;
}

int ZPercyClient::read_request(nqueries_t request_id, nservers_t sid, char* buf, int chunk_size) {
    ZRequest request = this->requests[request_id];
    request.request_buffers[sid]->read(buf, chunk_size);
    return request.request_buffers[sid]->gcount();
    // if (&this->request_buffers[sid]) {
    //     return chunk_size;
    // } else {
    //     return this->request_buffers[sid]->gcount();
    // }
}

void ZPercyClient::write_response(nqueries_t request_id, nservers_t sid, char* buf, int chunk_size) {
    ZRequest request = this->requests[request_id];
    request.response_buffers[sid]->write(buf, chunk_size);
}

void ZPercyClient::parse_response(nqueries_t request_id) {
    nqueries_t querybsize = 1;

    ZRequest request = this->requests[request_id];

    vector<istream*> isvec;// = std::vector<std::istream*>(1, &resp_stream);
    for (int i = 0; i < request.response_buffers.size(); i++) {
        // this->response_buffers[i]->clear();
        // this->response_buffers[i]->seekg(0, ios::beg);
        isvec.push_back(request.response_buffers[i]);
    }

    client->receive_replies(request_id, isvec);

    nservers_t h = client->calculate_h(querybsize);

    client->process_replies(h);

    vector<PercyBlockResults> results;
    bool res =  client->get_result(request_id, results);


    bool ret = true;
    nqueries_t num_res = results.size();

    for (nqueries_t r=0; ret && r < num_res; ++r) {
        if (results[r].results.empty()) {
            std::cerr << "PIR query failed.\n";
            ret = false;
            break;
        }
        else if (results[r].results.size() > 1) {
            std::cerr << results[r].results.size() << " possible blocks returned.\n";
        }
        // Output the retrieved block(s)
        vector<PercyResult>::const_iterator resiter;
        for (resiter = results[r].results.begin(); resiter != results[r].results.end(); ++resiter) {
            // HADI: What if multiple possible blocks?
            request.result_buffers[r]->str(resiter->sigma);
        }
    }
}

int ZPercyClient::read_result(nqueries_t request_id, nqueries_t query_number, char* buf, int chunk_size) {
    ZRequest request = this->requests[request_id];
    request.result_buffers[query_number]->read(buf, chunk_size);
    return request.result_buffers[query_number]->gcount();
    // if (this->result_buffer) {
    //     return chunk_size;
    // } else {
    //     return this->result_buffer.gcount();
    // }
}

void ZPercyClient::finish_request(nqueries_t request_id) {
    // Delete request from request map
    this->requests[request_id].destroy();
    this->requests.erase(request_id);
}

nqueries_t client_make_request(ZPercyClient* self, dbsize_t* blocks, nqueries_t query_count) {
    vector<dbsize_t> queries;

    for (int i = 0; i < query_count; i++) {
        queries.push_back(blocks[i]);
    }

    return self->make_request(queries);
}

int client_read_request(ZPercyClient* self, nqueries_t request_id, nservers_t sid, char* buf, int chunk_size) {
    if (chunk_size > MAX_CHUNK_SiZE) {
        chunk_size = MAX_CHUNK_SiZE;
    }

    return self->read_request(request_id, sid, buf, chunk_size);
}

void client_write_response(ZPercyClient* self, nqueries_t request_id, nservers_t sid, char* buf, int chunk_size) {
    return self->write_response(request_id, sid, buf, chunk_size);
}

int client_read_result(ZPercyClient* self, nqueries_t request_id, nqueries_t query_number, char* buf, int chunk_size) {
    if (chunk_size > MAX_CHUNK_SiZE) {
        chunk_size = MAX_CHUNK_SiZE;
    }

    return self->read_result(request_id, query_number, buf, chunk_size);
}

void client_parse_response(ZPercyClient* self, nqueries_t request_id) {
    // membuf buff(raw_response, raw_response + length);
    // istream response_stream(&buff);
    self->parse_response(request_id);
}

void client_finish_request(ZPercyClient* self, nqueries_t request_id) {
    self->finish_request(request_id);
}
