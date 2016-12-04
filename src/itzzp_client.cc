#include "itzzp.h"

#include <sys/wait.h>
#include <sys/stat.h>

#include <sstream>

// stringstream errstream;

int _main(dbsize_t num_blocks, dbsize_t block_size, dbsize_t w, nservers_t num_servers,
    nservers_t t, sid_t * server_indices, PercyClient* &percy_client)
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


    // !! HADI: arguments
    // -m ZZ_P
    mode = MODE_ZZ_P;


    // Check for conflicting optional arguments.
    if (do_hybrid && (mode != MODE_ZZ_P)) {
	std::cerr << "Error: hybrid security can only be used with the integers mod p mode "
		"of operation." << std::endl;
	exit(1);
    }
#ifdef SPIR_SUPPORT
    if (do_hybrid && do_spir) {
	std::cerr << "Error: cannot use hybrid security with symmetric PIR." << std::endl;
	exit(1);
    }
    if (do_spir && mode != MODE_ZZ_P) {
	std::cerr << "Error: symmetric PIR can only be used with the integers mod p mode "
		"of operation." << std::endl;
	exit(1);
    }
#endif
    if (tau && mode == MODE_CHOR) {
	std::cerr << "Error: Chor et al.'s PIR scheme does not support tau independence." << std::endl;
	exit(1);
    }

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
    if (w == 2048)
    {
	p1 = to_ZZ("208647130951457402363969335056365957472826150618980217460328400485971950387185944410889077723063406198415802830757517777351462262669194793047360775411639408116452523756687066355086195124187048682420529316060567502352699557841412039275095485224490337148164650010000499984813523719988826268799665657866626493329");
	p2 = to_ZZ("245210205383950153265232956846271987008710436579074459102383753214859717124121302267932009072054546430711727811323033561244148876933172687995163379778095734152594201215411509169035373484564340604271927100344464582888777887746436564737355045100633587336239754449508771770564607896955672999950235015535154415867");
    }
    else if (w == 1536)
    {
	p1 = to_ZZ("1762848592595080314705600925431624874456855439794595868418019480189213868063348394981842423875338178991362893712297567682392276281463789141688306484765105096429658863055172316227409205756175078509101834587188923103831602929062176351");
	p2 = to_ZZ("2306072568237159640249655953989533876736033293267891813492402870069702343561490811306173449455816459207943593196801405361355605814646339972518285709494570145269396000374210678514250118174550977925517522811232946347459478425104006037");
    }
    else if (w == 1024)
    {
	p1 = to_ZZ("14710132128541592475387440366744304824352604767753216777226640368050037133836174845369895150342922969891066267019166301546403100960464521216972792406229873");
	p2 = to_ZZ("23338263930359653850870152235447790059566299230626918909126959284529524161146399225204807633841208114717867386116272471253667601589249734409576687328158817");
    }
    else if (mode == MODE_GF28)
    {
	p1 = to_ZZ("1");
	p2 = to_ZZ("256");
    }
    else if (mode == MODE_GF216)
    {
	p1 = to_ZZ("1");
	p2 = to_ZZ("65536");
    }
    else if (mode == MODE_CHOR)
    {
	//Important: for Chor we pretend as though a word is 1 byte. This is because many
	//parts of the code rely on a word being a byte multiple (for example, where bytes_per_word
	//is used). We set this here since the calculations for the optimal database shape need a
	//word size of 1 bit.
	//words_per_block /= 8;
	p1 = to_ZZ("1");
	p2 = to_ZZ("256");
    }
    else if (w == 8 && !do_hybrid)
    {
	p1 = to_ZZ("1");
	p2 = to_ZZ("257");
    }
    else if (w == 16 && !do_hybrid)
    {
	p1 = to_ZZ("1");
	p2 = to_ZZ("65537");
    }
    else if (w == 32 && !do_hybrid)
    {
	p1 = to_ZZ("1");
	p2 = to_ZZ("4294967311");
    }
    else if (w == 96 && !do_hybrid)
    {
	p1 = to_ZZ("1");
	p2 = to_ZZ("79228162514264337593543950397");
    }
    else if (w == 128 && !do_hybrid)
    {
	p1 = to_ZZ("1");
	p2 = to_ZZ("340282366920938463463374607431768211507");
    }
    else if (w == 160 && !do_hybrid)
    {
	// NOTE: p2s is the prime from the PolyCommit params; spir
	// will break if this value gets changed!
	//
	// TODO: read the prime from the PolyCommit params and check
	// 		 that it is consistent with w.
	p1 = to_ZZ("1");
	p2 = to_ZZ("2425980306017163398341728799446792216592523285797");
    }
    else if (w == 192 && !do_hybrid)
    {
	p1 = to_ZZ("1");
	p2 = to_ZZ("6277101735386680763835789423207666416102355444464034513029");
    }
    else if (w == 256 && !do_hybrid)
    {
	p1 = to_ZZ("1");
	p2 = to_ZZ("115792089237316195423570985008687907853269984665640564039457584007913129640233");
    }
    else if (do_hybrid)
    {
	errstream << "Error: No hybrid-compatible modulus available for w = " << w << "." << std::endl;
	return -1;
    }
    else
    {
	errstream << "Error: No modulus available for w = " << w << "." << std::endl;
	return -1;
    }
#ifdef SPIR_SUPPORT
    if (do_spir && w!=160)
    {
	errstream << "Error: symmetric PIR currently supports only w=160." << std::endl;
	return -1;
    }
#endif
    ZZ modulus = p1 * p2;

    unsigned long modulusbytes = NumBytes(modulus);
    if (modulusbytes <= w/8)
    {
	errstream << "Error: w must be at most " << (modulusbytes-1)*8 << " (was " << w << ")." << std::endl;
	return -1;
    }

    // Parse the server information
    // HADI: NO SERVER INFO NEEDED
    // vector<char*> servers;
    // char *token = strtok(argv[optind++], " ");
    // while (token != NULL) {
	// servers.push_back(token);
	// token = strtok(NULL, " ");
    // }
    // nservers_t ell = servers.size();
    // sid_t * server_indices = new sid_t[ell];
    // vector<serverinfo> sinfos;
    // for (nservers_t j = 0; j < ell; j++) {
	// char *sids = strtok(servers[j], ":");
	// char *addrs = strtok(NULL, ":");
	// char *ports = strtok(NULL, ":");
	// if (ports == NULL) {
	//     std::cerr << "Error: something is wrong with the server information in position " << (j+1) << "." << std::endl;
	//     delete[] server_indices;
	//     exit(1);
	// }
	// unsigned long sid = strtoul(sids, NULL, 10);
	// server_indices[j] = (sid_t)sid;
	// if (!sid || sid > modulus) {
	//     std::cerr << "Error: SID must be an integer greater than 0 and less than " << modulus << "." << std::endl;
	//     delete[] server_indices;
	//     exit(1);
	// }
	// unsigned long port = strtoul(ports, NULL, 10);
	// if (port < 1024 || port > 65535) {
	//     std::cerr << "Error: port number must be an integer greater than 1024 and less than 65536." << std::endl;
	//     delete[] server_indices;
	//     exit(1);
	// }
	// serverinfo sinfo;
	// sinfo.sid = sid;
	// sinfo.addr = addrs;
	// sinfo.port = port;
	// sinfos.push_back(sinfo);
    // }
    // Set up an iostream to each of the servers.
    // vector<serverinfo> onlinesinfos;
    // vector<iosockinet*> serverstreams;
    // vector<istream*> istreams;
    // vector<ostream*> ostreams;
    // for (nservers_t j = 0; j < ell; j++)
    // {
	// iosockinet *socket = connect_to_server(sinfos[j]);
	// std::iostream * stream = socket;
    //     if ( socket != NULL ) {
	//     onlinesinfos.push_back(sinfos[j]);
	//     serverstreams.push_back(socket);
	//     istreams.push_back(stream);
	//     ostreams.push_back(stream);
	// }
    // }

    // HADI from arguments
    nservers_t ell = num_servers;
    if (k == 0) {
	k = ell;
    }
    // HADI in arguments
    // nservers_t t = strtoul(argv[optind++], NULL, 10);

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

    // Get (and perform sanity check on) the query indices.
    // vector<dbsize_t> indices;
    // istringstream indexss(argv[optind++]);
    // while(!indexss.eof())
    // {
	// dbsize_t index;
	// indexss >> index;
	// indices.push_back(index);
    // }
    // for (nqueries_t q = 0; q < indices.size(); q++)
    // {
	// if (indices[q] >= num_blocks) {
	//     std::cerr << "Error: all query indices must be less than number of blocks." << std::endl;
	//     // for (nservers_t i = 0; i < serverstreams.size(); ++i) {
	// 	// if (serverstreams[i]) delete serverstreams[i];
	//     // }
	//     // delete[] server_indices;
	//     // exit(1);
    //     return -1;
	// }
    // }
    // std::cerr << "Fetching " << indices.size() << " blocks." << std::endl;

    // Create the PercyClientParams object.
    PercyParams * params = NULL;
    switch (mode) {
    case MODE_ZZ_P:
	if (do_hybrid) {
	    params = new ZZ_pParams(num_blocks, words_per_block * w / 8,
		    w, p1, p2, tau);
	} else {
	    params = new ZZ_pParams(num_blocks, words_per_block * w / 8,
		    w, modulus, tau, pcparams_file, do_spir);
	}
	break;
    case MODE_CHOR:
	    params = new ChorParams(num_blocks, words_per_block * w / 8);
	break;
    case MODE_GF28:
    case MODE_GF216:
	    params = new GF2EParams(num_blocks, words_per_block * w / 8, w, tau);
	break;
    // default:
	// std::cerr << "Invalid mode: " << mode << "\n";
	// for (nservers_t i = 0; i < serverstreams.size(); ++i) {
	//     if (serverstreams[i]) delete serverstreams[i];
	// }
	// delete[] server_indices;
	// exit(1);
	// break;
    }
    if (!params) {
	errstream << "Error creating parameters object\n";
	// for (nservers_t i = 0; i < serverstreams.size(); ++i) {
	//     if (serverstreams[i]) delete serverstreams[i];
	// }
	// delete[] server_indices;
	   return -1;
    }
    // !!!!!!!! IMPORTNANT: HADI: MUST MAKE IT A POINTER
    PercyClientParams* clientparams = new PercyClientParams (params, num_servers);

    // Send each server the parameters (and its SID, if necessary).
//     for (nservers_t j = 0; j < serverstreams.size(); j++) {
// 	std::ostream &os = *ostreams[j];
//
// 	// Send the PercyClientParams.
// //		std::cerr << "Sending query parameters to server " << onlinesinfos[j].sid << "...";
// 	clientparams.send(os, onlinesinfos[j].sid);
// //		std::cerr << "done" << std::endl;
//
// 	std::istream &is = *istreams[j];
// 	unsigned char failure;
// //		std::cerr << "Receiving response from server " << onlinesinfos[j].sid << "...";
// 	is.read((char*)&failure, 1);
// //		std::cerr << "done" << std::endl;
// 	if (failure) {
// 	    std::cerr << "Error: " << onlinesinfos[j].addr << ":" << onlinesinfos[j].port << " did not accept parameters." <<  std::endl;
// 	    delete params;
// 	    for (nservers_t i = 0; i < serverstreams.size(); ++i) {
// 		if (serverstreams[i]) delete serverstreams[i];
// 	    }
// 	    delete[] server_indices;
// 	    exit(1);
// 	}
//     }

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


ZPercyClient* client_zzp_new(dbsize_t num_blocks, dbsize_t block_size,
                           dbsize_t w, nservers_t num_servers, nservers_t t) {
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

    int result = _main(num_blocks, block_size, w, num_servers, t, sids, percy_client);
    if (!result) {
        ZPercyClient* client = new ZPercyClient(percy_client, num_servers);
        return client;
    } else {
        return NULL;
    }
}
