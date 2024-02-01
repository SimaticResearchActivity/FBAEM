#include <iostream>
#include <thread>
#include "../../SessionLayer/SessionLayer.h"
#include "SequencerAlgoLayer.h"
#include "SequencerAlgoLayerMsg.h"
#include "../../msgTemplates.h"
#include <mpi.h>
#include <future>

using namespace std;
using namespace fbae_SequencerAlgoLayer;


bool SequencerAlgoLayer::executeAndProducedStatistics() {

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    cout << "MPI INFO : rank/size : " << rank << "/" << size << "\n";

    MPI_Barrier(MPI_COMM_WORLD);


    // In Sequencer algorithm, the last site is not broadcasting. We build @broadcasters vector accordingly.
    setBroadcasters(size - 1);

    // Process is sequencer
    if (rank == 0) {

        if (getSession()->getParam().getVerbose())
            cout << "\tSequencerAlgoLayer / Sequencer : Wait for messages\n";

        while (receive) {

            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

            int message_size;

            MPI_Get_count(&status, MPI_BYTE, &message_size);

            std::vector<char> buffer(message_size);

            MPI_Recv(buffer.data(), message_size, MPI_BYTE, status.MPI_SOURCE, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);


            auto bmtb{deserializeStruct<StructMessageToBroadcast>(std::string(buffer.begin(), buffer.end()))};
            auto s{serializeStruct<StructBroadcastMessage>(StructBroadcastMessage{MsgId::BroadcastMessage,
                                                                                  bmtb.senderRank,
                                                                                  bmtb.sessionMsg})};

            int msgSize = s.size();
            MPI_Bcast(&msgSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(s.data(), msgSize, MPI_BYTE, 0, MPI_COMM_WORLD);

        }
        if (getSession()->getParam().getVerbose())
            cout
                    << "\tSequencerAlgoLayer / Sequencer : Finished waiting for messages ==> Giving back control to SessionLayer\n";

        return false;

    } else {
        // Process is a broadcaster
        getSession()->callbackInitDone();

        if (getSession()->getParam().getVerbose())
            cout << "\tSequencerAlgoLayer / Broadcaster #" << rank << " : Wait for messages\n";
        while (receive) {
            std::vector<char> buffer;

            auto task_to_receive_msg = std::async(std::launch::async, [&buffer, this] {
                int msgSize;

                MPI_Bcast(&msgSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
                buffer.resize(msgSize);
                MPI_Bcast(buffer.data(), msgSize, MPI_BYTE, 0, MPI_COMM_WORLD);

            });
            task_to_receive_msg.get();

            auto sbm {deserializeStruct<StructBroadcastMessage>(std::move(std::string(buffer.begin(), buffer.end())))};
            getSession()->callbackDeliver(sbm.senderRank,std::move(sbm.sessionMsg));

        }
        cout << "\tSequencerAlgoLayer / Broadcaster #" << rank
             << " : Finished waiting for messages ==> Giving back control to SessionLayer\n";

        return true;
    }
}

void SequencerAlgoLayer::terminate() {
    receive = false;
}

std::string SequencerAlgoLayer::toString() {
    return "Sequencer";
}


void SequencerAlgoLayer::totalOrderBroadcast(string &&msg) {
    // Send MessageToBroadcast to sequencer
    auto bmtb{serializeStruct<StructMessageToBroadcast>(StructMessageToBroadcast{MsgId::MessageToBroadcast,
                                                                                 static_cast<rank_t>(rank),
                                                                                 msg})};

    MPI_Send(bmtb.data(), bmtb.size(), MPI_BYTE, 0, 0, MPI_COMM_WORLD);

}



