#include "itzzp.h"

#include <sys/wait.h>
#include <sys/stat.h>

#include <sstream>

// stringstream errstream;

// Get the database size.  Returns 0 if the database does not exist.
dbsize_t database_bytes (const char * database) {
    struct stat filestatus;
    int not_exists = stat(database, &filestatus);
    if (not_exists)
    {
	errstream << "Error: cannot find database file " << database << std::endl;
	return 0;
    }
    return filestatus.st_size;
}

int _main (char* database, nservers_t sid, dbsize_t num_blocks, dbsize_t block_size, dbsize_t w, PercyServer* &percy_server)
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

    // Allow hybrid queries?
    bool do_hybrid = false;

    // Mode of operation selected (ZZ_p, GF28, GF216 or Chor)
    PercyMode mode = MODE_ZZ_P;

    // Strassen maximum depth
    bool strassen_set = false;
    int strassen_max_depth = PercyServer::STRASSEN_OPTIMAL;

    // Do symmetric PIR?
    bool do_spir = false;

    // PolyCommit Params file to read for SPIR
    char *pcparams_file = NULL;

    // Is the database tau independent?
    bool is_tau = false;

    // Should we be byzantine?
    bool be_byzantine = false;

    uint16_t port = 0;

    // Word size
    dboffset_t offset = 0;

    // Distributive computation parameters
    bool is_master = false;
    nservers_t num_threads = 0;
    DistSplit tsplit = DIST_SPLIT_RECORDS;
    nservers_t num_workers = 0;
    DistSplit wsplit = DIST_SPLIT_RECORDS;
    bool is_forked = false;
    bool is_worker = false;
    nservers_t worker_index = 0;
    bool is_partial_database = false;

    // Logging
    bool do_logging = false;
    char * logfilename = NULL;
    char * logappend = NULL;
    bool print_header = false;

    // Run once or as a daemon
    bool daemon_mode = true;

    // Parse arguments
#ifdef SPIR_SUPPORT
    const char * shortopts = "Mk:w:O:tm:hz1p:l:L:HZ:T:FS:D:Ps:";
#else
    const char * shortopts = "Mk:w:O:tm:hz1p:l:L:HT:FS:D:Ps:";
#endif

    // !! HADI: arguments
    // -m ZZ_P
    mode = MODE_ZZ_P;
    // -s optimal
    strassen_set = true;
    strassen_max_depth = PercyServer::STRASSEN_OPTIMAL;

    // HADI: CAN REMOVE
    // Cannot be a master and a worker
    // if (is_master && is_worker) {
	// fprintf(stderr, "Cannot be a master and a worker");
	// print_usage(argv[0], -1);
    // }

    // HADI: CAN REMOVE
    // Get required arguments
    // if (is_worker) {
	// num_workers = strtoul(argv[optind++], NULL, 10);
	// worker_index = strtoul(argv[optind++], NULL, 10);
	// // Check if these make sense
	// if (worker_index >= num_workers) {
	//     fprintf(stderr, "The worker index must be less than the number of workers\n");
	//     return -1;
	// }
    // }

    char * workerinfo = NULL;
    // HADI: CAN REMOVE Replaced with args
    // char * database = NULL;
    // if (is_master) {
	// workerinfo = argv[optind++];
    // } else {
	// database = argv[optind++];
    // }

    // HADI: CAN REMOVE Replaced with args
    // nservers_t sid = strtoul(argv[optind++], NULL, 10);
    // dbsize_t num_blocks = strtoull(argv[optind++], NULL, 10);
    // dbsize_t block_size = strtoull(argv[optind++], NULL, 10);

    dbbits_t n_bytes = num_blocks * block_size;
    dbsize_t b = block_size * 8;

    // Change threading method from pthread to fork if in ZZ_P
    if (num_threads > 0 and mode == MODE_ZZ_P and !is_forked) {
	fprintf(stderr, "The pthread library is not compatible with ZZ_p.  Forking separate processes instead.\n");
	is_forked = true;
    }

    // Sanity checks
    if (do_hybrid && (mode != MODE_ZZ_P)) {
        errstream <<  "Error: hybrid security can only be used with the integers mod p mode of operation.\n";
	return -1;
    }
#ifdef SPIR_SUPPORT
    if (do_hybrid && do_spir) {
        errstream <<  "Error: cannot use hybrid security with symmetric PIR.\n";
	return -1;
    }
    if (do_spir && mode != MODE_ZZ_P) {
        errstream << "Error: symmetric PIR can only be used with the integers mod p mode of operation.\n";
	return -1;
    }
#endif
    if (is_tau && mode == MODE_CHOR) {
        errstream << "Error: Chor et al.'s PIR scheme does not support tau independence.\n";
	return -1;
    }
    if (strassen_set && mode != MODE_ZZ_P && mode != MODE_GF28 &&
	    mode != MODE_GF216) {
        errstream <<  "Strassen is only implemented for GF28, GF216 and ZZ_p.\n";
	return -1;
    }

    bool badw = false;
    switch (mode) {
    case MODE_ZZ_P:
	badw = (w % 8 != 0);
	break;
    case MODE_CHOR:
	badw = (w != 1);
	break;
    case MODE_GF28:
	badw = (w != 8);
	break;
    case MODE_GF216:
	badw = (w != 16);
	break;
    default:
	break;
    }
    if (badw) {
	errstream << "Invalid word size for mode " << mode << ": " << w << "\n";
	return 1;
    }

    // Choose an appropriate modulus.
    ZZ modulus = to_ZZ("256");
    if (mode == MODE_ZZ_P) {
	switch (w) {
	case 2048:
	    modulus = to_ZZ("51162405833378812589599605953260132300166393994651819099454781579567509212081792013783783759303440508155949594262147212874957344953142209597742684263402581129339826752613431877280173074502314648334418584122460414512816448592261381117519846844295394134225624418756277265452922709245846828145574822031541004633366879073894273715489429502290966133193310966178373909137394353164436844312924586836474134940807305776164928781025210917912257206480517698118422827367766257579221703667784216949825206167241852365543481875593117676222875888924950402025039269210778276794873837063438751454865130720887819939394489366347567251243");
	    break;
	case 1536:
	    modulus = to_ZZ("4065256781338999183533854850423382625119065920051798531476300569026463202897155088318466013703570859212040475097762405522038651420119366364979939687154236065682459920101982590074846996306687236388206057475890613264408059472973401701686869808348910896596468985609043697525749128687318350246421674945679872669881805678484464202726328189280359385791023305618545788872763420795247846720674554774196715770302797683129209164871258189464484019233379849839076263862630987");
	    break;
	case 1024:
	    modulus = to_ZZ("343308946066366926839932845260501528909643718159825813630709694160026342456154871924497152436552679706642965502704642456637620829912957820221098686748075257358288200837461739492534713539606088624083011849535450485951774635526473457667739540374042376629835941950802202870595346459371144019363420985729553740241");
	    break;
	case 256:
	    modulus = to_ZZ("115792089237316195423570985008687907853269984665640564039457584007913129640233");
	    badw = do_hybrid;
	    break;
	case 192:
	    modulus = to_ZZ("6277101735386680763835789423207666416102355444464034513029");
	    badw = do_hybrid;
	    break;
	case 160:
	    // NOTE: p2s is the prime from the PolyCommit params; spir
	    // will break if this value gets changed!
	    //
	    // TODO: read the prime from the PolyCommit params and check
	    //          that it is consistent with w.
	    modulus = to_ZZ("2425980306017163398341728799446792216592523285797");
	    badw = do_hybrid;
	    break;
	case 128:
	    modulus = to_ZZ("340282366920938463463374607431768211507");
	    badw = do_hybrid;
	    break;
	case 96:
	    modulus = to_ZZ("79228162514264337593543950397");
	    badw = do_hybrid;
	    break;
	case 32:
	    modulus = to_ZZ("4294967311");
	    badw = do_hybrid;
	    break;
	case 16:
	    modulus = to_ZZ("65537");
	    badw = do_hybrid;
	    break;
	case 8:
	    modulus = to_ZZ("257");
	    badw = do_hybrid;
	    break;
	default:
	    errstream<< "Error: No modulus available for w = " << w << "." << std::endl;
	    return -1;
	}

	if (badw) {
	    errstream << "Error: No hybrid-compatible modulus available for w = " << w << "." << std::endl;
	    return -1;
	}

#ifdef SPIR_SUPPORT
	if (do_spir && w!=160)
	{
	    errstream <<  "Error: symmetric PIR currently supports only w=160.\n";
	    return -1;
	}
#endif
    }

    std::vector<std::iostream*> workers;
    std::vector<nservers_t> worker_sids;
    dbsize_t dbsize = 0;
    if (is_master) {
	// Get worker addresses and ports
	std::vector<char*> workerstr;
	char * token = strtok(workerinfo, " ");
	while (token != NULL) {
	    workerstr.push_back(token);
	    token = strtok(NULL, " ");
	}
	num_workers = workerstr.size();
	if (num_workers < 1) {
	    errstream << "The number of workers must be at least 1\n";
	    for (unsigned i = 0; i < workers.size(); ++i)
		if (workers[i]) delete workers[i];
	    return -1;
	}

    // HADI: CAN REMOVE as long as we don't want workers
	// Create worker iostreams
	// for (dbsize_t i = 0; i < num_workers; ++i) {
	//     char * sidchar = strtok(workerstr[i], ":");
	//     char * addrchar = strtok(NULL, ":");
	//     char * portchar = strtok(NULL, ":");
	//     if (addrchar == NULL || portchar == NULL || portchar == NULL) {
	// 	fprintf(stderr, "Error: worker information was incorrectly formatted.\n");
	// 	for (unsigned i = 0; i < workers.size(); ++i)
	// 	    if (workers[i]) delete workers[i];
	// 	return -1;
	//     }
	//     nservers_t sidnum = strtoul(sidchar, NULL, 10);
	//     if (!sidnum || sidnum > modulus) {
	// 	std::cerr << "Error: SID must be an integer greater than 0 and less than " << modulus << ".\n";
	// 	for (unsigned i = 0; i < workers.size(); ++i)
	// 	    if (workers[i]) delete workers[i];
	// 	return -1;
	//     }
	//     worker_sids.push_back(sidnum);
	//     unsigned long portnum = strtoul(portchar, NULL, 10);
	//     if (portnum < 1024 || portnum > 65535) {
	// 	fprintf(stderr, "Error: port number must be an integer greater than 1024 and less than 65535.\n");
	// 	for (unsigned i = 0; i < workers.size(); ++i)
	// 	    if (workers[i]) delete workers[i];
	// 	return -1;
	//     }
	//     // connect_to_server function is located in distserver.{h,cc}
	//     iosockinet * socket = connect_to_server(addrchar, portnum);
	//     if (socket == NULL) {
	// 	std::cerr << "Error: cannot connect to worker " << i << ": " << workerstr[i] << ".\n";
	// 	for (unsigned i = 0; i < workers.size(); ++i)
	// 	    if (workers[i]) delete workers[i];
	// 	return -1;
	//     }
	//     std::iostream * stream = socket;
	//     workers.push_back(stream);
	// }

	// database size must be specified
	if (!n_bytes) {
	    errstream << "Error: Database size (n) must be specified.\n";
	    for (unsigned i = 0; i < workers.size(); ++i)
		if (workers[i]) delete workers[i];
	    return -1;
	}

    } else {
	// Make sure the specified database file exists.
	dbsize = database_bytes(database);
	if (dbsize == 0) {
	    errstream << "Error: the database must exist and be non-empty.\n";
	    return -1;
	}

	// If no value for "n" is specified, then use a default database
	// size of dbsize. Otherwise, just check that 0<n<=dbsize.
	if (!n_bytes) {
	    n_bytes = dbsize;
	} else if (!(is_worker && is_partial_database) && n_bytes > dbsize) {
	    errstream << "Error: n cannot be larger than database file.\n";
	    return -1;
	}

	// Set all worker sids to my sid, so that we're using the right sid if
	// I'm a worker.
	worker_sids = std::vector<nservers_t>(num_workers, sid);
    }

    dbbits_t n = n_bytes * 8;
    if (n_bytes > n) {
        errstream <<  "Error: database file is too large for the current architecture!\n";
	for (unsigned i = 0; i < workers.size(); ++i)
	    if (workers[i]) delete workers[i];
	return -1;
    }

    // If no value for "b" is specified, then use a default block size
    // of \sqrt(n * w) bits.
    if (!b) {
        b = sqrt(n * w);
        if (n != b*b/w)
        {
            errstream << "Error: optimal parameter choice is invalid for this database. Please specify a value for both of b and w.\n";
	    for (unsigned i = 0; i < workers.size(); ++i)
		if (workers[i]) delete workers[i];
	    return -1;
        }
    }

    // Sanity checks for (n,b,w).
    if (n % b != 0 || b % w != 0)
    {
        errstream  << "Error: b must divide n and w must divide b. " << "n: " << n << " b: " << b << " w: " << w << std::endl;
	for (unsigned i = 0; i < workers.size(); ++i)
	    if (workers[i]) delete workers[i];
	return -1;
    }

    // Compute the number of blocks, and number of words per block.
    dbsize_t words_per_block = b / w;
    //std::cerr << "Number of blocks: " << num_blocks << std::endl;
    //std::cerr << "Words per block:  " << words_per_block << std::endl;
    //std::cerr << "Bits per block:  " << b << std::endl;
    //std::cerr << "Bits per word:  " << w << std::endl;
    if (num_blocks != words_per_block)
    {
        std::cerr << "Warning: non-optimal choice of blocksize detected. " << num_blocks << std::endl;
    }

    // Create the PercyServerParams object.
    PercyParams * params = NULL;
    switch (mode) {
    case MODE_ZZ_P:
	params = new ZZ_pParams(num_blocks, block_size, w, modulus,
		is_tau, pcparams_file, do_spir);
	break;
    case MODE_CHOR:
	params = new ChorParams(num_blocks, block_size);
	break;
    case MODE_GF28:
    case MODE_GF216:
	params = new GF2EParams(num_blocks, block_size, w, is_tau);
	break;
    default:
	errstream<< "Invalid mode: " << mode << "\n";
	for (unsigned i = 0; i < workers.size(); ++i)
	    if (workers[i]) delete workers[i];
	return -1;
    }
    if (!params) {
	errstream << "Error creating parameters object\n";
	for (unsigned i = 0; i < workers.size(); ++i)
	    if (workers[i]) delete workers[i];
	return -1;
    }
    const PercyServerParams * serverparams = new PercyServerParams(params, sid,
	    num_threads, tsplit, num_workers, wsplit, worker_sids, is_forked,
	    be_byzantine);
    if (!serverparams) {
	delete params;
	for (unsigned i = 0; i < workers.size(); ++i)
	    if (workers[i]) delete workers[i];
	return -1;
    }

    // Send params to workers if necessary
    if (is_master) {
	std::vector<const PercyServerParams*> wparams =
		serverparams->get_all_worker_serverparams();
	for (nservers_t i = 0; i < num_workers; ++i) {
	    wparams[i]->send(*workers[i], true);
	}
	unsigned char failure;
	for (nservers_t i = 0; i < num_workers; ++i) {
	    workers[i]->read((char*)&failure, 1);
	    if (failure) {
		errstream << "Worker " << i << " did not accept parameters\n";
		delete serverparams;
		delete params;
		for (unsigned i = 0; i < workers.size(); ++i)
		    if (workers[i]) delete workers[i];
		return -1;
	    }
	}
    }

    // Create datastore
    DataStore * datastore = NULL;
    if (!is_master) {
	if (is_worker && is_partial_database) {
	    const PercyParams * wparams = serverparams->get_worker_params(worker_index);
	    const PercyServerParams * wsparams = serverparams->get_worker_serverparams(worker_index);
	    if (wparams->num_blocks() * wparams->block_size() > dbsize) {
		errstream << "Error: The partial database is not large enough!\n";
		delete serverparams;
		delete params;
		for (unsigned i = 0; i < workers.size(); ++i)
		    if (workers[i]) delete workers[i];
		return -1;
	    }
	    datastore = new FileDataStore(database, wsparams, offset);
	} else {
	    datastore = new FileDataStore(database, serverparams, offset);
	}
	if (!datastore) {
	    errstream << "Error creating datastore\n";
	    delete serverparams;
	    delete params;
	    for (unsigned i = 0; i < workers.size(); ++i)
		if (workers[i]) delete workers[i];
	    return -1;
	}
    }

    const PercyServerParams * parentparams = NULL;
    const DataStore * parentdatastore = NULL;
    if (is_worker) {
	parentparams = serverparams;
	serverparams = parentparams->get_worker_serverparams(worker_index);
	if (!is_partial_database) {
	    parentdatastore = datastore;
	    datastore = parentdatastore->get_worker_datastore(worker_index);
	}
    }

#ifdef VERBOSE
    serverparams->print(std::cerr);
    std::cerr << "\n";
#endif

    // HADI: CAN REMOVE
    // Create a socket for clients to connect to.
    // sockinetbuf sin(sockbuf::sock_stream);
    // if (!bind_to_port(sin, port)) {
	// if (datastore) delete (parentdatastore ? parentdatastore : datastore);
	// delete (parentparams ? parentparams : serverparams);
	// delete params;
	// for (unsigned i = 0; i < workers.size(); ++i)
	//     if (workers[i]) delete workers[i];
	// fprintf(stderr, "Did not successfully bind to a port\n");
	// return -1;
    // }

    // Logging
    PercyStats * stats = NULL;
    ofstream logstream;
    if (do_logging) {
	if (logfilename != NULL) {
	    struct stat buffer;
	    print_header &= (stat (logfilename, &buffer) != 0);
	    logstream.open(logfilename, std::ios_base::app | std::ios_base::out);
	    stats = new PercyServerStats(serverparams, logstream, logappend);
	    if (print_header) {
		stats->print_header();
	    }
	} else {
	    stats = new PercyServerStats(serverparams, logappend);
	}
	if (!stats) {
	    if (datastore) delete (parentdatastore ? parentdatastore : datastore);
	    delete (parentparams ? parentparams : serverparams);
	    delete params;
	    for (unsigned i = 0; i < workers.size(); ++i)
		if (workers[i]) delete workers[i];
	    errstream <<  "Error creating stats object\n";
	    return -1;
	}
    }

    // Create the server
    PercyServer * server = PercyServer::make_server(datastore, serverparams, stats);
    if (server == NULL) {
	if (stats) delete stats;
	if (datastore) delete (parentdatastore ? parentdatastore : datastore);
	delete (parentparams ? parentparams : serverparams);
	delete params;
	for (unsigned i = 0; i < workers.size(); ++i)
	    if (workers[i]) delete workers[i];
	errstream <<  "Server not created successfully\n";
	return -1;
    }

    server->set_strassen_max_depth(strassen_max_depth);

    // HADI
    percy_server = server;

    // Clean up
    // delete server;
    // if (stats) delete stats;
    // if (datastore) delete (parentdatastore ? parentdatastore : datastore);
    // delete (parentparams ? parentparams : serverparams);
    // delete params;
    // for (unsigned i = 0; i < workers.size(); ++i)
	// if (workers[i]) delete workers[i];

    return 0;
}



ZPercyServer* server_zzp_new(char* dbfile, dbsize_t num_blocks, dbsize_t block_size,
                           dbsize_t w ) {
    PercyServer* percy_server = NULL;

    // Giving all sid 1 for now
    int result = _main(dbfile, 1, num_blocks, block_size, w, percy_server);
    if (!result) {
        ZPercyServer* server = new ZPercyServer(percy_server);
        return server;
    } else {
        // strcpy(error_message, errstream.str().c_str());
        // errstream.str(std::string());
        // errstream.clear();
        return NULL;
    }
}
