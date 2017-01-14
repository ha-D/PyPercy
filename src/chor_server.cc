#include "chor.h"

#include <sys/wait.h>
#include <sys/stat.h>

#include <sstream>

// stringstream errstream;

// Get the database size.  Returns 0 if the database does not exist.
dbsize_t chor_database_bytes (const char * database) {
    struct stat filestatus;
    int not_exists = stat(database, &filestatus);
    if (not_exists)
    {
	errstream << "Error: cannot find database file " << database << std::endl;
	return 0;
    }
    return filestatus.st_size;
}

int _main (char* database, nservers_t sid, dbsize_t num_blocks, dbsize_t block_size, PercyServer* &percy_server)
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
    // HADI
    dbsize_t w = 1;
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
    // -m CHOR
    mode = MODE_CHOR;
    // -s optimal
    strassen_set = true;
    strassen_max_depth = PercyServer::STRASSEN_OPTIMAL;


    // }

    char * workerinfo = NULL;


    dbbits_t n_bytes = num_blocks * block_size;
    dbsize_t b = block_size * 8;



    if (is_tau && mode == MODE_CHOR) {
        errstream << "Error: Chor et al.'s PIR scheme does not support tau independence.\n";
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



	// database size must be specified
	if (!n_bytes) {
	    errstream << "Error: Database size (n) must be specified.\n";
	    for (unsigned i = 0; i < workers.size(); ++i)
		if (workers[i]) delete workers[i];
	    return -1;
	}

    } else {
	// Make sure the specified database file exists.
	dbsize = chor_database_bytes(database);
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

    if (num_blocks != words_per_block)
    {
        std::cerr << "Warning: non-optimal choice of blocksize detected. " << num_blocks << " " << b << " " << w << std::endl;
    }

    // Create the PercyServerParams object.
    PercyParams * params = NULL;
    switch (mode) {

    case MODE_CHOR:
	params = new ChorParams(num_blocks, block_size);
	break;

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


    return 0;
}



ZPercyServer* server_chor_new(char* dbfile, dbsize_t num_blocks, dbsize_t block_size)  {
    PercyServer* percy_server = NULL;

    // Giving all sid 1 for now
    int result = _main(dbfile, 1, num_blocks, block_size, percy_server);
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
