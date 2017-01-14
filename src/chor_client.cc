#include "chor.h"

#include <sys/wait.h>
#include <sys/stat.h>

#include <sstream>


int _main(dbsize_t num_blocks, dbsize_t block_size,  nservers_t num_servers,
          sid_t * server_indices, PercyClient* &percy_client)
{
    // HADI: CAN REMOVE
    // // Ignore SIGPIPE
    // signal(SIGPIPE, SIG_IGN);

    // Initialize NTL and the random number stream
    ZZ modinit;
    modinit = to_ZZ(257);
    ZZ_p::init(modinit);
    unsigned char randbuf[128];
    ifstream urand("/dev/urandom");
    urand.read((char *)randbuf, sizeof(randbuf));
    urand.close();
    ZZ randzz = ZZFromBytes(randbuf, sizeof(randbuf));
    SetSeed(randzz);


    // Servers needed
    nservers_t k = 0;

    // Do symmetric PIR?
    bool do_spir = false;

    // PolyCommit Params file to read for SPIR
    char *pcparams_file = NULL;

    // Enable hybrid security?
    bool do_hybrid = false;

    // Mode of operation selected (ZZ_p, GF28, GF216 or Chor)
    PercyMode mode = MODE_ZZ_P;

    // qbs indicates query block size
    nqueries_t qbs = 1;

    nservers_t tau = 0;

    // Logging
    bool do_logging = false;
    char * logfilename = NULL;
    char * logappend = NULL;
    bool print_header = false;

    // HADI
    dbsize_t w = 1;

    // !! HADI: arguments
    // -m ZZ_P
    mode = MODE_CHOR;


    // HADI
    // dbsize_t num_blocks = strtoull(argv[optind++], NULL, 10);
    // dbsize_t block_size = strtoull(argv[optind++], NULL, 10);
    dbsize_t words_per_block = block_size * 8 / w;

    // Sanity checks for (n,b,w).
    bool badw = false;
    switch (mode) {
    case MODE_CHOR:
	badw = ( w != 1 );
	break;
    case MODE_GF28:
	badw = ( w != 8 );
	break;
    case MODE_GF216:
	badw = ( w != 16 );
	break;
    default:
	badw = ( w % 8 != 0 );
	break;
    }
    if (badw) {
	errstream << "Error: Invalid word size for mode " << mode << ": " << w << "\n";
	return -1;
    }

    // Compute the number of blocks, and number of words per block.
    //std::cerr << "Number of blocks: " << num_blocks << std::endl;
    //std::cerr << "Words per block:  " << words_per_block << std::endl;
    if (num_blocks != words_per_block)
    {
	std::cerr << "Warning: non-optimal choice of blocksize detected." << std::endl;
    }

    // Choose an appropriate modulus.
    ZZ p1, p2;
    if (mode == MODE_CHOR)
    {
	//Important: for Chor we pretend as though a word is 1 byte. This is because many
	//parts of the code rely on a word being a byte multiple (for example, where bytes_per_word
	//is used). We set this here since the calculations for the optimal database shape need a
	//word size of 1 bit.
	//words_per_block /= 8;
	p1 = to_ZZ("1");
	p2 = to_ZZ("256");
    }
    ZZ modulus = p1 * p2;

    unsigned long modulusbytes = NumBytes(modulus);
    if (modulusbytes <= w/8)
    {
	errstream << "Error: w must be at most " << (modulusbytes-1)*8 << " (was " << w << ")." << std::endl;
	return -1;
    }


    // HADI from arguments
    nservers_t ell = num_servers;
    if (k == 0) {
	k = ell;
    }
    // HADI in arguments
    // nservers_t t = strtoul(argv[optind++], NULL, 10);
    // HADI MANUALLY SET T
    nservers_t t = k - 1;

    // HADI: changing here
    if (num_servers < k)
    {
    	errstream << "Error: fewer than k=" << k << " out of ell=" << ell << " servers are online." << std::endl;
    	return -1;
    }

    // Sanity checks for (ell,t,k,tau).
    if (qbs-1+t+tau >= k)
    {
    	errstream << "Error: qbs-1+t+tau must be less than k." << std::endl;
    	return -1;
    }
    if (mode == MODE_CHOR && t != k-1) {
        errstream << "Error: Chor requires that t=k-1." << std::endl;
        return -1;
    }
    if (mode == MODE_CHOR && qbs != 1) {
        errstream << "Error: Chor requires that qbs=1." << std::endl;
        return -1;
    }



    // Create the PercyClientParams object.
    PercyParams * params = NULL;
    switch (mode) {
    case MODE_CHOR:
	    params = new ChorParams(num_blocks, words_per_block * w / 8);
	break;
    }
    if (!params) {
	       errstream << "Error creating parameters object\n";
	   return -1;
    }
    // !!!!!!!! IMPORTNANT: HADI: MUST MAKE IT A POINTER
    PercyClientParams* clientparams = new PercyClientParams (params, num_servers);


    // Logging
    PercyStats * stats = NULL;
    ofstream logstream;
    if (do_logging) {
	if (logfilename != NULL) {
	    struct stat buffer;
	    print_header &= (stat (logfilename, &buffer) != 0);
	    logstream.open(logfilename, std::ios_base::app | std::ios_base::out);
	    stats = new PercyClientStats(clientparams, logstream, logappend);
	    if (print_header) {
		stats->print_header();
	    }
	} else {
	    stats = new PercyClientStats(clientparams, logappend);
	}
    }

    // Finally, do the PIR query!
    int ret = 0;
    PercyClient * client = PercyClient::make_client(clientparams, ell, t, server_indices, stats);
    percy_client = client;

    return 0;
}


ZPercyClient* client_chor_new(dbsize_t num_blocks, dbsize_t block_size, nservers_t num_servers) {
    // Make a list of fake sids
    sid_t* sids = new sid_t[num_servers];
    for (int i = 0; i < num_servers; i++) {
        sids[i] = i + 1;
    }

    // vector<dbsize_t> indices;
    // for (int i = 0; i < num_queries; i++) {
    //     indices.push_back(queries[i]);
    // }
    //  vector<dbsize_t> indices,
// nqueries_t num_queries, dbsize_t* queries

    PercyClient* percy_client = NULL;

    int result = _main(num_blocks, block_size, num_servers, sids, percy_client);
    if (!result) {
        ZPercyClient* client = new ZPercyClient(percy_client, num_servers);
        return client;
    } else {
        return NULL;
    }
}
