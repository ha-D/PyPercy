#ifndef _Z_AG_
#define _Z_AG_

#include "agclient.h"
#include "agserver.h"
#include "percytypes.h"
#include "percyparams.h"
#include "agparams.h"

#include "itparams.h"
#include "itserver.h"
#include "itclient.h"
#include "ZZ.h"

#include "zpercy.h"
#include <sstream>

extern "C" {
    ZPercyServer* server_ag_new(char* dbfile, dbsize_t num_blocks, dbsize_t block_size, dbsize_t N,
                          dbsize_t w);
    ZPercyClient* client_ag_new(dbsize_t num_blocks, dbsize_t block_size, dbsize_t N,
                         dbsize_t w);

}
#endif
