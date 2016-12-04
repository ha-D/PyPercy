#include "ag.h"

ZPercyServer* server_ag_new(char* dbfile, dbsize_t num_blocks, dbsize_t block_size,
                      dbsize_t N, dbsize_t w) {

    bool is_null = false;

    PercyParams * params =  new AGParams(num_blocks, block_size, N, w);
    PercyServerParams * serverparams =
            new PercyServerParams(params, 1, 0, DIST_SPLIT_RECORDS, 0, DIST_SPLIT_RECORDS,
                                  std::vector<nservers_t>(), 1);

    DataStore* datastore = new FileDataStore(dbfile, serverparams, 0);

    PercyServer* percy_server = new PercyAGServer(datastore, serverparams);
    ZPercyServer* server = new ZPercyServer(percy_server);

    return server;
}

ZPercyClient* client_ag_new(dbsize_t num_blocks, dbsize_t block_size, dbsize_t N,
                           dbsize_t w) {
    bool is_null = false;

    PercyParams * params =  new AGParams(num_blocks, block_size, N, w);
    PercyClientParams * clientparams =  new PercyClientParams(params, 1, is_null);

    PercyClient* percy_client =  new PercyAGClient(clientparams);
    ZPercyClient* client =  new ZPercyClient(percy_client, 1);
    // client->x = 10;
    return client;
}
