#ifndef _Z_ZZP_
#define _Z_ZZP_

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
    ZPercyServer* server_zzp_new(char* dbfile, dbsize_t num_blocks, dbsize_t block_size,
                               dbsize_t w);
    ZPercyClient* client_zzp_new(dbsize_t num_blocks, dbsize_t block_size,
                              dbsize_t w, nservers_t num_servers, nservers_t t);
}
#endif
