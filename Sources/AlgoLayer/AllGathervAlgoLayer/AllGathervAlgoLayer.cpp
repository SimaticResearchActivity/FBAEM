//
// Created by lardeur on 7/2/24.
//

#include "../../SessionLayer/SessionLayer.h"
#include "AllGathervAlgoLayer.h"
#include "AllGathervAlgoLayerMsg.h"
#include "../../msgTemplates.h"

#include <algorithm>
#include <ctgmath>
#include <future>
#include <mpi.h>

using namespace std;
using namespace fbae_AllGathervAlgoLayer;


bool AllGathervAlgoLayer::executeAndProducedStatistics() {

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Barrier(MPI_COMM_WORLD);

    setBroadcasters(size);
    getSession()->callbackInitDone();

    if (getSession()->getParam().getVerbose())
        cout << "\tAllGathervAlgoLayer / Broadcaster#" << static_cast<uint32_t>(rank) << " : Wait for messages\n";

    auto task_to_receive_msg = std::async(std::launch::async, [this] {
        while (receive) {

            std::string s;
            {
                lock_guard lck(mtxBatchCtrl);
                s = {serializeStruct<Message>(Message{MsgId::Message,
                                                        static_cast<rank_t>(rank),
                                                        msgsWaitingToBeBroadcast})};
                msgsWaitingToBeBroadcast.clear();
            }

            int msgSize = s.size();

            // Gather the size of each message
            std::vector<int> message_sizes(size);
            MPI_Allgather(&msgSize, 1, MPI_INT, message_sizes.data(), 1, MPI_INT, MPI_COMM_WORLD);

            // Calculate total size of messages
            int total_message_size = 0;
            for (int i = 0; i < size; ++i)
                total_message_size += message_sizes[i];

            // Allocate buffer for receiving messages
            std::vector<char> buffer(total_message_size);

            // Calculate displacement array for allgatherv
            std::vector<int> displacements(size);
            displacements[0] = 0;
            for (int i = 1; i < size; ++i)
                displacements[i] = displacements[i - 1] + message_sizes[i - 1];

            // Gather the messages
            MPI_Allgatherv(s.data(), msgSize, MPI_BYTE,
                           buffer.data(), message_sizes.data(), displacements.data(),
                           MPI_BYTE, MPI_COMM_WORLD);

            int offset = 0;
            for (int i = 0; i < size; i++) {
                string msg(buffer.data() + offset, buffer.data() + offset + message_sizes[i]);
                auto deserializedMsg{deserializeStruct<Message>(std::move(msg))};
                cout << deserializedMsg.senderRank;
                for (string h: deserializedMsg.batchesBroadcast) {
                    // We surround the call to @callbackDeliver method with shortcutBatchCtrl = true; and
                    // shortcutBatchCtrl = false; This is because callbackDeliver() may lead to a call to
                    // @totalOrderBroadcast method which could get stuck in condVarBatchCtrl.wait() instruction
                    // because task @SessionLayer::sendPeriodicPerfMessage may have filled up @msgsWaitingToBeBroadcast
                    shortcutBatchCtrl = true;
                    getSession()->callbackDeliver(deserializedMsg.senderRank, std::move(h));
                    shortcutBatchCtrl = false;
                }
                offset += message_sizes[i];
            }
        }
    });
    task_to_receive_msg.get();

    if (getSession()->getParam().getVerbose())
        cout << "Broadcaster #" << static_cast <uint32_t>(rank)
             << " Finished waiting for messages ==> Giving back control to SessionLayer\n";

    return true;
}

void AllGathervAlgoLayer::terminate() {
    receive = false;
}

std::string AllGathervAlgoLayer::toString() {
    return "AllGatherv";
}

void AllGathervAlgoLayer::totalOrderBroadcast(std::string &&msg) {
    unique_lock lck(mtxBatchCtrl);
    condVarBatchCtrl.wait(lck, [this] {
        return (msgsWaitingToBeBroadcast.size() * getSession()->getParam().getSizeMsg() <
                getSession()->getParam().getMaxBatchSize())
               || shortcutBatchCtrl;
    });
    msgsWaitingToBeBroadcast.emplace_back(std::move(msg));
}
