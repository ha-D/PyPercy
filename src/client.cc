#include "zpercy.h"
#include <vector>

#include <iostream>
#include <sstream>

char* zbuffer = new char[MAX_CHUNK_SiZE];

ZAGClient::ZAGClient(const PercyClientParams * params) : PercyAGClient(params, NULL),ZPercyClient()  {
    this->zparams = params->percy_params();
    this->zclientparams = params;
//    this->request_buffer = new stringstream();
    std::cout << "I'm here in c1 " << this <<  std::endl;
}


nqueries_t ZAGClient::make_request(dbsize_t block) {
    nqueries_t querybsize = 1;

    std::vector<dbsize_t> blocks (1, block);
    nqueries_t request_identifier = encode_request(blocks, querybsize);

    // Clear buffer before new request
    this->request_buffer.str( std::string() );
    this->request_buffer.clear();

    std::vector<std::ostream*> ostreams (1, &this->request_buffer);
    send_request(request_identifier, ostreams);

    this->request_id = request_identifier;

    return request_identifier;
}

int ZAGClient::read_request(char* buf, int chunk_size) {
    this->request_buffer.read(buf, chunk_size);
    if (this->request_buffer) {
        return chunk_size;
    } else {
        return this->request_buffer.gcount();
    }
}

void ZAGClient::parse_response(istream &resp_stream) {
    parse_response(this->request_id, resp_stream);
}

void ZAGClient::parse_response(nqueries_t req_id, istream &resp_stream) {
    nqueries_t querybsize = 1;

    vector<istream*> isvec = std::vector<std::istream*>(1, &resp_stream);
    receive_replies(req_id, isvec);

    nservers_t k = goodservers.size();
    if(k == 0) {
        std::cerr << "ERROR: No good replies received, returning" << endl;
        return;
    }
    // Calculate what h to use
    nservers_t h = (nservers_t)(floor(sqrt((t+clientparams->percy_params()->tau()+querybsize-1)*k)))+1;
    const char *envh = getenv("PIRC_H");
    if (envh) {
        nservers_t override_h = atoi(envh);
        if (override_h > 0) {
            h = override_h;
        }
    }
    process_replies(h);

    vector<PercyBlockResults> results;
    bool res =  get_result(req_id, results);
//    return res;

    // Clear response buffer
    this->response_buffer.str( std::string() );
    this->response_buffer.clear();

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
            this->response_buffer.str(resiter->sigma);
        }
    }
}

int ZAGClient::read_response(char* buf, int chunk_size) {
    this->response_buffer.read(buf, chunk_size);
    if (this->response_buffer) {
        return chunk_size;
    } else {
        return this->response_buffer.gcount();
    }
}


ZAGClient* client_new(dbsize_t num_blocks, dbsize_t block_size, dbsize_t N,
                           dbsize_t word_size, int why) {
    bool is_null = false;
    // std::cout << "This shit " << num_blocks << " " << block_size << " " << N << " " << word_size << std::endl;

    PercyParams * params =  new AGParams(num_blocks, block_size, N, word_size);
    PercyClientParams * clientparams =  new PercyClientParams(params, 1, is_null);

    ZAGClient* client =  new ZAGClient(clientparams);
    client->x = 10;
    return client;
}


nqueries_t client_make_request(ZAGClient* self, dbsize_t block) {
    std::cout << "I'm here in c2 " << self <<  std::endl;
    return self->make_request(block);
//    self->x = 10;
    return 0;
}

int client_read_request(ZAGClient* self, char* buf, int chunk_size) {
    if (chunk_size > MAX_CHUNK_SiZE) {
        chunk_size = MAX_CHUNK_SiZE;
    }

    return self->read_request(buf, chunk_size);
}

int client_read_response(ZAGClient* self, char* buf, int chunk_size) {
    if (chunk_size > MAX_CHUNK_SiZE) {
        chunk_size = MAX_CHUNK_SiZE;
    }

    return self->read_response(buf, chunk_size);
}

void client_parse_response(ZAGClient* self, int req_id, char* raw_response, int length) {
    membuf buff(raw_response, raw_response + length);
    istream response_stream(&buff);

    if (req_id == 0) {
        self->parse_response(response_stream);
    } else {
        self->parse_response(req_id, response_stream);
    }
}
