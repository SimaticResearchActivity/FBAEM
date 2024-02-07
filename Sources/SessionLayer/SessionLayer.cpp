#include <iostream>
#include <mutex>
#include <future>
#include "SessionLayer.h"
#include "SessionLayerMsg.h"
#include "../msgTemplates.h"
#include <mpi.h>

using namespace std;
using namespace fbae_SessionLayer;

SessionLayer::SessionLayer(const Param &param, rank_t rank, std::unique_ptr<AlgoLayer> algoLayer)
: param{param}
, rank{rank}
, algoLayer{std::move(algoLayer)}
, measures{static_cast<size_t>(param.getNbMsg()*(100-param.getWarmupCooldown())/100)+1}//We add +1 to avoid not allocating enough size because of rounding by default
{
    this->algoLayer->setSession(this);
}

void SessionLayer::broadcastPerfMeasure() {
    if (param.getVerbose())
        cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : Broadcast PerfMeasure (senderRank = " << static_cast<uint32_t>(rank) << " ; msgNum = " << numPerfMeasure << ")\n";
    if (numPerfMeasure == param.getNbMsg()*param.getWarmupCooldown()/100/2)
        measures.setStartTime();
    auto s {serializeStruct<SessionPerfMeasure>(SessionPerfMeasure{SessionMsgId::PerfMeasure,
                                                                   rank,
                                                                   numPerfMeasure,
                                                                   std::chrono::system_clock::now(),
                                                                   std::string(
                                                                           param.getSizeMsg() -
                                                                           minSizeClientMessageToBroadcast,
                                                                           0)})};
    algoLayer->totalOrderBroadcast(std::move(s));
    ++numPerfMeasure;
}

void SessionLayer::callbackDeliver(rank_t senderRank, std::string && msg) {
    switch (auto sessionMsgTyp{ static_cast<SessionMsgId>(msg[0]) }; sessionMsgTyp)
    {
        using enum SessionMsgId;
        case FinishedPerfMeasures :
            processFinishedPerfMeasuresMsg(senderRank);
            break;
        case FirstBroadcast :
            processFirstBroadcastMsg(senderRank);
            break;
        case PerfMeasure :
            processPerfMeasureMsg(senderRank, std::move(msg));
            break;
        case PerfResponse :
            processPerfResponseMsg(senderRank, std::move(msg));
            break;
        default:
        {
            cerr << "ERROR: Unexpected sessionMsgTyp (" << static_cast<int>(sessionMsgTyp) << ")\n";
            exit(EXIT_FAILURE);
        }
    }
}

void SessionLayer::callbackInitDone() const
{
    // Broadcast FirstBroadcast
    if (param.getVerbose())
        cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : Broadcast FirstBroadcast (sender = " << static_cast<uint32_t>(rank) << ")\n";
    auto s {serializeStruct<SessionFirstBroadcast>(SessionFirstBroadcast{SessionMsgId::FirstBroadcast})};
    algoLayer->totalOrderBroadcast(std::move(s));
}

void SessionLayer::execute()
{
    future<void> taskSendPeriodicPerfMessage;
    if (param.getVerbose())
        cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : Start execution\n";
    if (param.getFrequency())
        taskSendPeriodicPerfMessage = std::async(std::launch::async, &SessionLayer::sendPeriodicPerfMessage, this);
    if (algoLayer->executeAndProducedStatistics())
    {
        // Display statistics
        static std::mutex mtx;
        scoped_lock lock{mtx};
        cout << Param::csvHeadline() << "," << Measures::csvHeadline() << "\n";
        cout << param.asCsv(algoLayer->toString(), to_string(rank)) << "," << measures.asCsv()
                << "\n";
    }
    if (param.getFrequency())
        taskSendPeriodicPerfMessage.get();
    if (param.getVerbose())
        cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : End of execution\n";
}


const Param &SessionLayer::getParam() const
{
    return param;
}

rank_t SessionLayer::getRank() const {
    return rank;
}

void SessionLayer::processFinishedPerfMeasuresMsg(rank_t senderRank)
{
    ++nbReceivedFinishedPerfMeasures;
    if (param.getVerbose())
        cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : Deliver FinishedPerfMeasures from sender #" << static_cast<uint32_t>(senderRank) << " (nbReceivedFinishedPerfMeasures = " << nbReceivedFinishedPerfMeasures << ")\n";

    if (nbReceivedFinishedPerfMeasures > algoLayer->getBroadcasters())
    {
        cerr << "ERROR : Delivering a FinishedPerfMeasures message while we already have received all FinishedPerfMeasures messages we were waiting for.\n";
        exit(EXIT_FAILURE);
    }
    if (nbReceivedFinishedPerfMeasures == algoLayer->getBroadcasters())
    {
        // All broadcasters are done doing measures ==> We can ask the AlgoLayer to terminate.
        algoLayer -> terminate();
    }
}

void SessionLayer::processFirstBroadcastMsg(rank_t senderRank) {
    ++nbReceivedFirstBroadcast;
    if (param.getVerbose())
        cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : Deliver FirstBroadcast from sender #" << static_cast<uint32_t>(senderRank) << " (nbReceivedFirstBroadcast = " << nbReceivedFirstBroadcast << ")\n";
    if (nbReceivedFirstBroadcast > algoLayer->getBroadcasters())
    {
        cerr << "ERROR : Delivering a FirstBroadcast message while we already have received all FirstBroadcast messages we were waiting for.\n";
        exit(EXIT_FAILURE);
    }
    if (nbReceivedFirstBroadcast == algoLayer->getBroadcasters())
    {
        // As we have received all awaited FirstBroadcast messages, we know that @AlgoLayer is fully
        // operational ==> We can start our performance measures.
        if (param.getFrequency())
            // We start periodic sending of PerfMessage
            okToSendPeriodicPerfMessage.count_down();
        else
            // We send a single PerfMeasure
            broadcastPerfMeasure();
    }
}

void SessionLayer::processPerfMeasureMsg(rank_t senderRank, std::string && msg) {
    auto spm{deserializeStruct<SessionPerfMeasure>(std::move(msg))};
    if (param.getVerbose())
        cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : Deliver PerfMeasure from sender #" << static_cast<uint32_t>(senderRank) << " (senderRank = " << static_cast<uint32_t>(spm.senderRank) << " ; msgNum = " << spm.msgNum << ")\n";
    measures.addNbBytesDelivered(param.getSizeMsg());
    // We check which process must send the PerfResponse. The formula hereafter guarantees that first PerfMeasure is
    // answered by successor of sender process, second PerfMeasure message is answered by successor of the successor of
    // sender process, etc.
    if ((spm.senderRank + spm.msgNum) % algoLayer->getBroadcasters() == rank)
    {
        // Current process must broadcast PerfResponse message for this PerfMeasure message.
        if (param.getVerbose())
            cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : Broadcast PerfResponse by sender #" << static_cast<uint32_t>(rank) << " (perfMeasureSenderRank = " << static_cast<uint32_t>(spm.senderRank) << " ; perfMeasureMsgNum = " << spm.msgNum << ")\n";
        auto s {serializeStruct<SessionPerfResponse>(SessionPerfResponse{SessionMsgId::PerfResponse,
                                                                         spm.senderRank,
                                                                         spm.msgNum,
                                                                         spm.sendTime,
                                                                         spm.filler})};
        algoLayer->totalOrderBroadcast(std::move(s));
    }
}

void SessionLayer::processPerfResponseMsg(rank_t senderRank, std::string && msg) {
    auto spr{deserializeStruct<SessionPerfResponse>(std::move(msg))};
    if (param.getVerbose())
        cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : Deliver PerfResponse from sender #" << static_cast<uint32_t>(senderRank) << " (perfMeasureSenderRank = " << static_cast<uint32_t>(spr.perfMeasureSenderRank) << " ; perfMeasureMsgNum = " << spr.perfMeasureMsgNum << ")\n";
    measures.addNbBytesDelivered(param.getSizeMsg());
    chrono::duration<double, std::milli> elapsed = std::chrono::system_clock::now() - spr.perfMeasureSendTime;
    if (spr.perfMeasureSenderRank == rank)
    {
        if (nbReceivedPerfResponseForSelf >= param.getNbMsg()) {
            cerr << "WARNING : SessionLayer #" << static_cast<uint32_t>(rank) << " : Deliver too many PerfResponse for self\n";
            return;
        }
        ++nbReceivedPerfResponseForSelf;
        if (spr.perfMeasureMsgNum >= param.getNbMsg()*param.getWarmupCooldown()/100/2
            && spr.perfMeasureMsgNum < param.getNbMsg() - param.getNbMsg()*param.getWarmupCooldown()/100/2)
            measures.add(elapsed);
        if (spr.perfMeasureMsgNum == param.getNbMsg() - param.getNbMsg()*param.getWarmupCooldown()/100/2 - 1)
            measures.setStopTime();
        if (spr.perfMeasureMsgNum < param.getNbMsg() - 1)
        {
            if (!param.getFrequency())
                // As we do not send periodic PerfMessage, we send another PerfMessage
                broadcastPerfMeasure();
        }
        else
        {
            // Process is done with sending PerfMeasure messages. It tells it is done to all broadcasters
            if (param.getVerbose())
                cout << "SessionLayer #" << static_cast<uint32_t>(rank) << " : Broadcast FinishedPerfMeasures by sender #" << static_cast<uint32_t>(rank) << "\n";
            auto s {serializeStruct<SessionFinishedPerfMeasures>(SessionFinishedPerfMeasures{SessionMsgId::FinishedPerfMeasures})};
            algoLayer->totalOrderBroadcast(std::move(s));
        }
    }
}

void SessionLayer::sendPeriodicPerfMessage() {
    okToSendPeriodicPerfMessage.wait();
    constexpr std::chrono::duration<double, std::milli> sleepDuration{5ms};
    constexpr double nbMillisecondsPerSecond{ 1'000.0 };
    const auto freq{ param.getFrequency() };
    auto startSending{ std::chrono::system_clock::now() };
    while(true) {
        auto elapsedPeriod{ duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startSending) };
        // Broadcast PerfMeasure messages until we reach the desired frequency
        while (numPerfMeasure < freq * static_cast<double>(elapsedPeriod.count()) / nbMillisecondsPerSecond) {
            broadcastPerfMeasure();
            if (numPerfMeasure >= param.getNbMsg())
                return;
        }
        std::this_thread::sleep_for(sleepDuration);
    }
}
