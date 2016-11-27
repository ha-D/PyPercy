#include "ag.h"

ZPercyServer* server_ag_new(char* dbfile, int db_offset, dbsize_t num_blocks, dbsize_t block_size,
                      dbsize_t N, dbsize_t word_size) {

    bool is_null = false;

    PercyParams * params =  new AGParams(num_blocks, block_size, N, word_size);
    PercyServerParams * serverparams =
            new PercyServerParams(params, 1, 0, DIST_SPLIT_RECORDS, 0, DIST_SPLIT_RECORDS,
                                  std::vector<nservers_t>(), 1);

    DataStore* datastore = new FileDataStore(dbfile, serverparams, db_offset);

    PercyServer* percy_server = new PercyAGServer(datastore, serverparams);
    ZPercyServer* server = new ZPercyServer(percy_server);

    return server;
}

ZPercyClient* client_ag_new(dbsize_t num_blocks, dbsize_t block_size, dbsize_t N,
                           dbsize_t word_size, int why) {
    bool is_null = false;

    PercyParams * params =  new AGParams(num_blocks, block_size, N, word_size);
    PercyClientParams * clientparams =  new PercyClientParams(params, 1, is_null);

    PercyClient* percy_client =  new PercyAGClient(clientparams);
    ZPercyClient* client =  new ZPercyClient(percy_client);
    // client->x = 10;
    return client;
}


// ZZ zzp_server_modulus(dbsize_t w) {
//    ZZ modulus = to_ZZ("256");
//    switch (w) {
//        case 2048:
//            modulus = to_ZZ("51162405833378812589599605953260132300166393994651819099454781579567509212081792013783783759303440508155949594262147212874957344953142209597742684263402581129339826752613431877280173074502314648334418584122460414512816448592261381117519846844295394134225624418756277265452922709245846828145574822031541004633366879073894273715489429502290966133193310966178373909137394353164436844312924586836474134940807305776164928781025210917912257206480517698118422827367766257579221703667784216949825206167241852365543481875593117676222875888924950402025039269210778276794873837063438751454865130720887819939394489366347567251243");
//            break;
//        case 1536:
//            modulus = to_ZZ("4065256781338999183533854850423382625119065920051798531476300569026463202897155088318466013703570859212040475097762405522038651420119366364979939687154236065682459920101982590074846996306687236388206057475890613264408059472973401701686869808348910896596468985609043697525749128687318350246421674945679872669881805678484464202726328189280359385791023305618545788872763420795247846720674554774196715770302797683129209164871258189464484019233379849839076263862630987");
//            break;
//        case 1024:
//            modulus = to_ZZ("343308946066366926839932845260501528909643718159825813630709694160026342456154871924497152436552679706642965502704642456637620829912957820221098686748075257358288200837461739492534713539606088624083011849535450485951774635526473457667739540374042376629835941950802202870595346459371144019363420985729553740241");
//            break;
//        case 256:
//            modulus = to_ZZ("115792089237316195423570985008687907853269984665640564039457584007913129640233");
//            break;
//        case 192:
//            modulus = to_ZZ("6277101735386680763835789423207666416102355444464034513029");
//            break;
//        case 160:
//            // NOTE: p2s is the prime from the PolyCommit params; spir
//            // will break if this value gets changed!
//            //
//            // TODO: read the prime from the PolyCommit params and check
//            //          that it is consistent with w.
//            modulus = to_ZZ("2425980306017163398341728799446792216592523285797");
//            break;
//        case 128:
//            modulus = to_ZZ("340282366920938463463374607431768211507");
//            break;
//        case 96:
//            modulus = to_ZZ("79228162514264337593543950397");
//            break;
//        case 32:
//            modulus = to_ZZ("4294967311");
//            break;
//        case 16:
//            modulus = to_ZZ("65537");
//            break;
//        case 8:
//            modulus = to_ZZ("257");
//            break;
//        default:
//            std::cerr << "Error: No modulus available for w = " << w << "." << std::endl;
//            return to_ZZ("256");
//     }
//     return modulus;
// }
//
// ZZ* zzp_client_modulus(dbsize_t w) {
//     ZZ p1, p2;
//     if (w == 2048) {
//     	p1 = to_ZZ("208647130951457402363969335056365957472826150618980217460328400485971950387185944410889077723063406198415802830757517777351462262669194793047360775411639408116452523756687066355086195124187048682420529316060567502352699557841412039275095485224490337148164650010000499984813523719988826268799665657866626493329");
//     	p2 = to_ZZ("245210205383950153265232956846271987008710436579074459102383753214859717124121302267932009072054546430711727811323033561244148876933172687995163379778095734152594201215411509169035373484564340604271927100344464582888777887746436564737355045100633587336239754449508771770564607896955672999950235015535154415867");
//     } else if (w == 1536) {
//     	p1 = to_ZZ("1762848592595080314705600925431624874456855439794595868418019480189213868063348394981842423875338178991362893712297567682392276281463789141688306484765105096429658863055172316227409205756175078509101834587188923103831602929062176351");
//     	p2 = to_ZZ("2306072568237159640249655953989533876736033293267891813492402870069702343561490811306173449455816459207943593196801405361355605814646339972518285709494570145269396000374210678514250118174550977925517522811232946347459478425104006037");
//     } else if (w == 1024) {
//     	p1 = to_ZZ("14710132128541592475387440366744304824352604767753216777226640368050037133836174845369895150342922969891066267019166301546403100960464521216972792406229873");
//     	p2 = to_ZZ("23338263930359653850870152235447790059566299230626918909126959284529524161146399225204807633841208114717867386116272471253667601589249734409576687328158817");
//     } else if (w == 8) {
//     	p1 = to_ZZ("1");
//     	p2 = to_ZZ("257");
//     } else if (w == 16) {
//     	p1 = to_ZZ("1");
//     	p2 = to_ZZ("65537");
//     } else if (w == 32) {
//     	p1 = to_ZZ("1");
//     	p2 = to_ZZ("4294967311");
//     } else if (w == 96) {
//     	p1 = to_ZZ("1");
//     	p2 = to_ZZ("79228162514264337593543950397");
//     } else if (w == 128) {
//     	p1 = to_ZZ("1");
//     	p2 = to_ZZ("340282366920938463463374607431768211507");
//     } else if (w == 160) {
//     	p1 = to_ZZ("1");
//     	p2 = to_ZZ("2425980306017163398341728799446792216592523285797");
//     } else if (w == 192) {
//     	p1 = to_ZZ("1");
//     	p2 = to_ZZ("6277101735386680763835789423207666416102355444464034513029");
//     } else if (w == 256) {
//     	p1 = to_ZZ("1");
//     	p2 = to_ZZ("115792089237316195423570985008687907853269984665640564039457584007913129640233");
//     } else {
//         std::cerr << "Error: No modulus available for w = " << w << "." << std::endl;
// 	    exit(1);
//     }
//     ZZ* p =  new ZZ[2];
//     p[0] = p1;
//     p[1] = p2;
//     return p;
// }
//
// ZPercyServer* server_zzp_new(char* dbfile, int db_offset, dbsize_t num_blocks, dbsize_t block_size,
//                            dbsize_t word_size, nservers_t tau, bool spir) {
//     bool is_null = false;
//     sid_t sid = 1;
//     int num_threads = 1;
//     int num_workers = 0;
//     std::vector<nservers_t> worker_sids = std::vector<nservers_t>(num_workers, sid);
//     bool is_forked = true;
//     bool be_byzantine = false;
//     ZZ modulus = zzp_server_modulus(word_size);
//
//     PercyParams * params =  new ZZ_pParams(num_blocks, block_size, word_size, modulus, tau, NULL, spir);
//     PercyServerParams * serverparams =
//             new PercyServerParams(params, sid,
//                                     num_threads, DIST_SPLIT_RECORDS, num_workers, DIST_SPLIT_RECORDS,
//                                     worker_sids, is_forked,
//                                     be_byzantine);
//
//     DataStore* datastore = new FileDataStore(dbfile, serverparams, db_offset);
//
//     PercyServer* percy_server = new PercyServer_ZZ_p(datastore, serverparams);
//     ZPercyServer* server = new ZPercyServer(percy_server);
//
//     return server;
// }
//
// ZPercyClient* client_zzp_new(nservers_t num_servers, nservers_t t, dbsize_t num_blocks, dbsize_t block_size,
//                            dbsize_t word_size, nservers_t tau, bool spir) {
//     ZZ* p = zzp_client_modulus(word_size);
//     ZZ p1 = p[0];
//     ZZ p2 = p[1];
//
//     // Make a list of fake sids
//     sid_t* sids = new sid_t[num_servers];
//     for (int i = 0; i < num_servers; i++) {
//         sids[i] = i + 1;
//     }
//
//     std::cout << p1 << " " << p2 << "       aaaaaaaaaaaaaaa \n";
//     PercyParams * params =  new ZZ_pParams(num_blocks, block_size, word_size, p1* p2, tau, NULL, 1);
//     PercyClientParams * clientparams =  new PercyClientParams(params, num_servers, 0); // CHECK is_null
//
//     PercyClient* percy_client =  new PercyClient_ZZ_p(clientparams, num_servers, t, sids, NULL);
//     ZPercyClient* client =  new ZPercyClient(percy_client);
//
//     return client;
// }
