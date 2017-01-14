#ifndef _Z_CHOR_
#define _Z_CHOR_

#include "percytypes.h"
#include "percyparams.h"

#include "itparams.h"
#include "itserver.h"
#include "itclient.h"
#include "ZZ.h"

#include "zpercy.h"
#include <sstream>

extern "C" {
    ZPercyServer* server_chor_new(char* dbfile, dbsize_t num_blocks, dbsize_t block_size);
    ZPercyClient* client_chor_new(dbsize_t num_blocks, dbsize_t block_size, nservers_t num_servers);
}
#endif
