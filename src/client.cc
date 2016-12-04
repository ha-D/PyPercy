#include "zpercy.h"
#include <vector>

#include <iostream>
#include <sstream>

using namespace std;

char* zbuffer = new char[MAX_CHUNK_SiZE];

// TODO not sure about these PercyClient constructor params, taken from AGClient
ZPercyClient::ZPercyClient(PercyClient* client, nservers_t num_servers):
 num_servers(num_servers)  {
    for (int i = 0; i < num_servers; i++) {
        this->request_buffers.push_back(new stringstream());
        this->response_buffers.push_back(new stringstream());
    }
    this->client = client;
}
ZPercyClient::~ZPercyClient() {
    for (int i = 0; i < num_servers; i++) {
        delete this->request_buffers[i];
        delete this->response_buffers[i];
    }
}

nqueries_t ZPercyClient::make_request(dbsize_t block) {

    nqueries_t querybsize = 1;

    std::vector<dbsize_t> blocks (1, block);
    nqueries_t request_identifier = client->encode_request(blocks, querybsize);

    // Clear buffer before new request
    for (int i = 0; i < num_servers; i++) {
        this->request_buffers[i]->str( std::string() );
        this->request_buffers[i]->clear();
    }

    std::vector<std::ostream*> ostreams;// (1, &this->request_buffer);
    for (int i = 0; i < this->request_buffers.size(); i++) {
        ostreams.push_back(this->request_buffers[i]);
    }
    client->send_request(request_identifier, ostreams);

    this->request_id = request_identifier;

    return request_identifier;
}

int ZPercyClient::read_request(nservers_t sid, char* buf, int chunk_size) {
    this->request_buffers[sid]->read(buf, chunk_size);
    return this->request_buffers[sid]->gcount();
    // if (&this->request_buffers[sid]) {
    //     return chunk_size;
    // } else {
    //     return this->request_buffers[sid]->gcount();
    // }
}

void ZPercyClient::write_response(nservers_t sid, char* buf, int chunk_size) {
    this->response_buffers[sid]->write(buf, chunk_size);
}

void ZPercyClient::parse_response() {
    parse_response(this->request_id);
}

void ZPercyClient::parse_response(nqueries_t req_id) {
    nqueries_t querybsize = 1;

    vector<istream*> isvec;// = std::vector<std::istream*>(1, &resp_stream);
    for (int i = 0; i < this->response_buffers.size(); i++) {
        // this->response_buffers[i]->clear();
        // this->response_buffers[i]->seekg(0, ios::beg);
        isvec.push_back(this->response_buffers[i]);
    }

    client->receive_replies(req_id, isvec);

    nservers_t h = client->calculate_h(querybsize);

    client->process_replies(h);

    vector<PercyBlockResults> results;
    bool res =  client->get_result(req_id, results);
//    return res;

    this->result_buffer.str( std::string() );
    this->result_buffer.clear();

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
//            std::cout << resiter->sigma;
//            std::cout << "DATA_LENGTH " << resiter->sigma.size() << std::endl;
            this->result_buffer.str(resiter->sigma);
        }
    }

    // Clear response buffers for next use
    for (int i = 0; i < num_servers; i++) {
        this->response_buffers[i]->str( std::string() );
        this->response_buffers[i]->clear();
    }

}

int ZPercyClient::read_result(char* buf, int chunk_size) {
    this->result_buffer.read(buf, chunk_size);
    return this->result_buffer.gcount();
    // if (this->result_buffer) {
    //     return chunk_size;
    // } else {
    //     return this->result_buffer.gcount();
    // }
}


nqueries_t client_make_request(ZPercyClient* self, dbsize_t block) {
    return self->make_request(block);
//    self->x = 10;
    return 0;
}

int client_read_request(ZPercyClient* self, nservers_t sid, char* buf, int chunk_size) {
    if (chunk_size > MAX_CHUNK_SiZE) {
        chunk_size = MAX_CHUNK_SiZE;
    }

    return self->read_request(sid, buf, chunk_size);
}

void client_write_response(ZPercyClient* self, nservers_t sid, char* buf, int chunk_size) {
    return self->write_response(sid, buf, chunk_size);
}

int client_read_result(ZPercyClient* self, char* buf, int chunk_size) {
    if (chunk_size > MAX_CHUNK_SiZE) {
        chunk_size = MAX_CHUNK_SiZE;
    }

    return self->read_result(buf, chunk_size);
}

void client_parse_response(ZPercyClient* self, int req_id) {
    // membuf buff(raw_response, raw_response + length);
    // istream response_stream(&buff);

    if (req_id == 0) {
        self->parse_response();
    } else {
        self->parse_response(req_id);
    }
}
