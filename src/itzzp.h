#ifndef _Z_AG_
#define _Z_AG_

#include "percytypes.h"
#include "percyparams.h"

#include "itparams.h"
#include "itserver.h"
#include "itclient.h"
#include "ZZ.h"

#include "zpercy.h"
#include <sstream>

extern "C" {
    ZZ zzp_server_modulus(dbsize_t w);
    ZZ* zzp_client_modulus(dbsize_t w);
    ZPercyServer* server_zzp_new(char* dbfile, int db_offset, dbsize_t num_blocks, dbsize_t block_size,
                               dbsize_t word_size, nservers_t tau, bool spir);
    ZPercyClient* client_zzp_new(nservers_t num_servers, nservers_t t, dbsize_t num_blocks, dbsize_t block_size,
                              dbsize_t word_size, nservers_t tau, bool spir);
}
#endif
