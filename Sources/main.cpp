#include <iostream>
#include "OptParserExtended.h"
#include "Param.h"

using namespace std;
using namespace mlib;

int main(int argc, char* argv[])
{
    //
    // Take care of program arguments
    //
    OptParserExtended parser{
            "a:algo algo_number \t Broadcast Algorithm\n\t\t\t\t\t\t0 = Sequencer based",
            "c:comm communicationLayer_identifier \t Communication layer to be used\n\t\t\t\t\t\te = Enet (reliable)",
            "h|help \t Show help message",
            "n:nbMsg number \t Number of messages to be sent",
            "r:rank rank_number \t Rank of process in site file (if -1, all algorithm participants are executed within threads in current process)",
            "s:size size_in_bytes \t Size of messages sent by a client (must be in interval [22,65515])",
            "S:site siteFile_name \t Name (including path) of the sites file to be used",
            "v|verbose \t [optional] Verbose display required"
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

    cout << "Algo = " << parser.getoptIntRequired('a') << "\n";
    cout << "CommLayer = " << parser.getoptStringRequired('c') << "\n";

    //
    // Launch the application
    //
    return EXIT_SUCCESS;
}
