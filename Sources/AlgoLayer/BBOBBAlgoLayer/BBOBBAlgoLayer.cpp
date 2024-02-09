//
// Created by lardeur on 09/10/23.
//

#include "../../SessionLayer/SessionLayer.h"
#include "BBOBBAlgoLayer.h"
#include "BBOBBAlgoLayerMsg.h"
#include "../../msgTemplates.h"

#include <algorithm>
#include <ctgmath>
#include <future>
#include <mpi.h>

using namespace std;
using namespace fbae_BBOBBAlgoLayer;


void BBOBBAlgoLayer::beginWave() {
    //Build first Step Message of a new Wave
    lastSentStepMsg.wave += 1;
    lastSentStepMsg.step = 0;
    const auto senderRank = rank;
    lastSentStepMsg.senderRank = senderRank;
    lastSentStepMsg.batchesBroadcast.clear();
    {
        lock_guard lck(mtxBatchCtrl);
        BatchSessionMsg newMessage{static_cast<rank_t>(senderRank), std::move(msgsWaitingToBeBroadcast)};
        msgsWaitingToBeBroadcast.clear();
        lastSentStepMsg.batchesBroadcast.emplace_back(std::move(newMessage));
    }
    condVarBatchCtrl.notify_one();

    // Send it
    if (getSession()->getParam().getVerbose())
        cout << "\tBBOOBBAlgoLayer / Broadcaster #" << static_cast<uint32_t>(rank)
             << " : Send Step Message (step : 0 / wave : " << lastSentStepMsg.wave << ") to Broadcaster #"
             << static_cast<uint32_t>(peersRank[lastSentStepMsg.step])
             << "\n";
    auto s{serializeStruct(lastSentStepMsg)};

    MPI_Send(s.data(), s.size(), MPI_BYTE, peersRank[lastSentStepMsg.step], 0, MPI_COMM_WORLD);

}

void BBOBBAlgoLayer::CatchUpIfLateInMessageSending() {
    // Are we able to send new stepMsg knowing the messages received in currentWaveReceivedStepMsg?
    auto step = lastSentStepMsg.step;
    while (lastSentStepMsg.step < nbStepsInWave - 1 && currentWaveReceivedStepMsg.contains(step)) {
        // Build the new version of lastSentStepMsg
        lastSentStepMsg.step += 1;
        lastSentStepMsg.batchesBroadcast.insert(lastSentStepMsg.batchesBroadcast.end(),
                                                currentWaveReceivedStepMsg[step].batchesBroadcast.begin(),
                                                currentWaveReceivedStepMsg[step].batchesBroadcast.end());
        // Send it
        if (getSession()->getParam().getVerbose())
            cout << "\tBBOOBBAlgoLayer / Broadcaster #" << static_cast<uint32_t>(rank)
                 << " : Send Step Message (step : " << lastSentStepMsg.step << " / wave : " << lastSentStepMsg.wave
                 << ") to Broadcaster#" << static_cast<uint32_t>(peersRank[lastSentStepMsg.step]) << "\n";
        auto s{serializeStruct(lastSentStepMsg)};
        MPI_Send(s.data(), s.size(), MPI_BYTE, peersRank[lastSentStepMsg.step], 0, MPI_COMM_WORLD);

        step = lastSentStepMsg.step;
    }
    //After all the messages of one wave have been received (and thus sent), deliver the messages of this wave.
    if (currentWaveReceivedStepMsg.size() == nbStepsInWave) {
        // Build vector of BatchSessionMsg to deliver
        // Note that to append currentWaveReceivedStepMsg[nbStepsInWave - 1].batchesBroadcast to batches, we
        // do not use batches.insert() but a for loop in order to be able to use std::move()
        vector<BatchSessionMsg> batches{std::move(lastSentStepMsg.batchesBroadcast)};
        for (auto &batch: currentWaveReceivedStepMsg[nbStepsInWave - 1].batchesBroadcast)
            batches.push_back(std::move(batch));

        // Compute the position in batches vector of participant rank 0, then participant rank 1, etc.
        // For example, positions[0] represents position of BatchSessionMsg of participant 0 in batches vector.
        // Note: If BatchSessionMsg of a participant appears twice, we memorize only one position
        //       (thus, afterward, we will not deliver twice this BatchSessionMsg).
        constexpr int not_found = -1;
        vector<int> positions(size, not_found);
        for (int pos = 0; pos < batches.size(); ++pos) {
            positions[batches[pos].senderRank] = pos;
        }

        // Deliver the different BatchSessionMsg
        for (auto const &pos: positions) {
            if (pos != not_found) {
                auto senderRank = batches[pos].senderRank;
                for (auto &msg: batches[pos].batchSessionMsg) {
                    // We surround the call to @callbackDeliver method with shortcutBatchCtrl = true; and
                    // shortcutBatchCtrl = false; This is because callbackDeliver() may lead to a call to
                    // @totalOrderBroadcast method which could get stuck in condVarBatchCtrl.wait() instruction
                    // because task @SessionLayer::sendPeriodicPerfMessage may have filled up @msgsWaitingToBeBroadcast
                    shortcutBatchCtrl = true;
                    getSession()->callbackDeliver(rank, std::move(msg));
                }
            }
        }

        // Prepare new wave
        currentWaveReceivedStepMsg = nextWaveReceivedStepMsg;
        nextWaveReceivedStepMsg.clear();
        beginWave();
        CatchUpIfLateInMessageSending();
    }
}

bool BBOBBAlgoLayer::executeAndProducedStatistics() {

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Barrier(MPI_COMM_WORLD);

    setBroadcasters(size);

    const auto n = static_cast<int>(size);

    nbStepsInWave = static_cast<int>(static_cast<int>(std::floor(log2(n))) == log2(n) ? log2(n) : ceil(log2(n)));



    for (int power_of_2 = 1; power_of_2 < size; power_of_2 *= 2) {
        peersRank.push_back((power_of_2 + rank) % size);
    }

    getSession()->callbackInitDone();
    beginWave();

    if (getSession()->getParam().getVerbose())
        cout << "\tBBOOBBAlgoLayer / Broadcaster#" << static_cast<uint32_t>(rank) << " : Wait for messages\n";

    auto task_to_receive_msg = std::async(std::launch::async, [this] {

        while (receive) {

            if (sendWave) {

                MPI_Status status;
                MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

                int message_size;

                MPI_Get_count(&status, MPI_BYTE, &message_size);

                std::vector<char> buffer(message_size);

                MPI_Recv(buffer.data(), message_size, MPI_BYTE, status.MPI_SOURCE, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);

                auto stepMsg{deserializeStruct<StepMsg>(std::string(buffer.begin(), buffer.end()))};


                if (getSession()->getParam().getVerbose())
                    cout << "\tBBOOBBAlgoLayer / Broadcaster #" << static_cast<uint32_t>(rank)
                         << " : Receive a Step Message (step : " << stepMsg.step << " / wave : "
                         << stepMsg.wave << ") from Broadcaster#" << static_cast<uint32_t>(stepMsg.senderRank) << "\n";

                if (stepMsg.wave == lastSentStepMsg.wave) {
                    currentWaveReceivedStepMsg[stepMsg.step] = stepMsg;
                    CatchUpIfLateInMessageSending();
                } else if (stepMsg.wave == lastSentStepMsg.wave + 1) {
                    // Message is early and needs to be treated in the next wave
                    nextWaveReceivedStepMsg[stepMsg.step] = stepMsg;
                } else {
                    cerr << "ERROR\tBBOBBAlgoLayer/ Broadcaster #" << rank << " (currentWave = "
                         << lastSentStepMsg.wave << ") : Unexpected wave = " << stepMsg.wave
                         << " from sender #" << static_cast<int>(stepMsg.senderRank) << " (with step = " << stepMsg.step
                         << ")\n";
                    exit(EXIT_FAILURE);
                }
            }
        }
    });
    task_to_receive_msg.get();


    if (getSession()->getParam().getVerbose())
        cout << "Broadcaster #" << static_cast<uint32_t>(rank)
             << " Finished waiting for messages ==> Giving back control to SessionLayer\n";

    return true;
}

void BBOBBAlgoLayer::terminate() {
    sendWave = false;
    receive = false;
}

std::string BBOBBAlgoLayer::toString() {
    return "BBOBB";
}

void BBOBBAlgoLayer::totalOrderBroadcast(std::string &&msg) {
    unique_lock lck(mtxBatchCtrl);
    condVarBatchCtrl.wait(lck, [this] {
        return (msgsWaitingToBeBroadcast.size() * getSession()->getParam().getSizeMsg() <
                getSession()->getParam().getMaxBatchSize())
               || shortcutBatchCtrl;
    });
    msgsWaitingToBeBroadcast.emplace_back(std::move(msg));
}
