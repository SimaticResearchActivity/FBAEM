#include <future>
#include <iostream>
#include "OptParserExtended.h"
#include "SessionLayer/SessionLayer.h"
#include "AlgoLayer/BBOBBAlgoLayer/BBOBBAlgoLayer.h"
#include "AlgoLayer/AllGathervAlgoLayer/AllGathervAlgoLayer.h"
#include "AlgoLayer/SequencerAlgoLayer/SequencerAlgoLayer.h"
#include <mpi.h>

using namespace std;
using namespace mlib;

unique_ptr<AlgoLayer> concreteAlgoLayer(OptParserExtended const &parser)
{
    char algoId = parser.getoptStringRequired('a')[0];
    switch(algoId)
    {
        case 'S': return make_unique<SequencerAlgoLayer>();
        case 'B' : return make_unique<BBOBBAlgoLayer>();
        case 'V': return make_unique<AllGathervAlgoLayer>();
        default:
            std::cerr << "ERROR: Argument for Broadcast Algorithm is " << algoId
                      << " which is not the identifier of a defined algorithm"
                      << std::endl
                      << parser.synopsis () << std::endl;
            exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[])
{
    //
    // Take care of program arguments
    //
    OptParserExtended parser{
            "a:algo algo_identifier \t Broadcast Algorithm\n\t\t\t\t\t\tB = BBOBB\n\t\t\t\t\t\tS = Sequencer based",
            "c:comm communicationLayer_identifier \t Communication layer to be used\n\t\t\t\t\t\te = Enet (reliable)\n\t\t\t\t\t\tt = TCP",
            "f:frequency number \t [optional] Number of PerfMessage session messages which must be sent each second (By default, a PerfMessage is sent when receiving a PerfResponse)",
            "h|help \t Show help message",
            "m:maxBatchSize size_in_bytes \t [optional] Maximum size of batch of messages (if specified algorithm allows batch of messages; By default, maxBatchSize is unlimited)",
            "n:nbMsg number \t Number of messages to be sent",
            "s:size size_in_bytes \t Size of messages sent by a client (must be in interval [22,65515])",
            "v|verbose \t [optional] Verbose display required",
            "w:warmupCooldown number \t [optional] Number in [0,99] representing percentage of PerfMessage session messages which will be considered as part of warmup phase or cool down phase and thus will not be measured for ping (By default, percentage is 0%)"
    };

    int nonopt;
    if (int ret ; (ret = parser.parse (argc, argv, &nonopt)) != 0)
    {
        if (ret == 1)
            cerr << "Unknown option: " << argv[nonopt] << " Valid options are : " << endl
                 << parser.synopsis () << endl;
        else if (ret == 2)
            cerr << "Option " << argv[nonopt] << " requires an argument." << endl;
        else if (ret == 3)
            cerr << "Invalid options combination: " << argv[nonopt] << endl;
        exit (1);
    }
    if (nonopt < argc)
    {
        cerr << "ERROR: There is a non-option argument '" << argv[nonopt]
             << "' which cannot be understood. Please run again program but without this argument" << endl
             << parser.synopsis () << endl;
        exit(1);
    }

    if ((argc == 1) || parser.hasopt ('h'))
    {
        //No arguments on command line or help required. Show help and exit.
        cerr << "Usage:" << endl;
        cerr << parser.synopsis () << endl;
        cerr << "Where:" << endl
             << parser.description () << endl;
        exit (0);
    }

    Param param{parser};

    //
    // Launch the application
    //


    int provided;
    MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    SessionLayer session{param, static_cast<rank_t>(rank), concreteAlgoLayer(parser)};
    session.execute();

    MPI_Finalize();

    return EXIT_SUCCESS;
}
