#include <iostream>
#include <thread>
#include "SessionLayer.h"
#include "SequencerAlgoLayer.h"
#include "SequencerAlgoLayerMsg.h"
#include "msgTemplates.h"

using namespace std;
using namespace fbae_SequencerAlgoLayer;

void SequencerAlgoLayer::callbackHandleMessageAsHost(std::unique_ptr<CommPeer> peer, const std::string &msgString)
{
    static atomic_int32_t seqNum{0};
    static atomic_int32_t nbConnectedBroadcasters{0};
    switch (BroadcasterMsgId broadcasterMsgTyp{ static_cast<BroadcasterMsgId>(msgString[0]) }; broadcasterMsgTyp)
    {
        case BroadcasterMsgId::DisconnectIntent :
        {
            auto bdi{deserializeStruct<BroadcasterDisconnectIntent>(msgString)};
            auto s{serializeStruct<SequencerAckDisconnectIntent>(SequencerAckDisconnectIntent{SequencerMsgId::AckDisconnectIntent})};
            peer->sendMsg(s);
            if (getSession()->getParam().getVerbose())
                cout << "\tSequencerAlgoLayer / Sequencer : Broadcaster #" << static_cast<unsigned int>(bdi.senderRank) << " announces it will disconnect.\n";
            break;
        }
        case BroadcasterMsgId::RankInfo :
        {
            auto bri{deserializeStruct<BroadcasterRankInfo>(msgString)};
            if (getSession()->getParam().getVerbose())
                cout << "\tSequencerAlgoLayer / Sequencer : Broadcaster #" << static_cast<unsigned int>(bri.senderRank) << " is connected to sequencer.\n";
            if (++nbConnectedBroadcasters == getBroadcasters().size())
            {
                if (getSession()->getParam().getVerbose())
                    cout << "\tSequencerAlgoLayer / Sequencer : All broadcasters are connected: Broadcast AllBroadcastersConnected\n";
                auto s{serializeStruct<SequencerAllBroadcastersConnected >(SequencerAllBroadcastersConnected{SequencerMsgId::AllBroadcastersConnected})};
                getSession()->getCommLayer()->broadcastMsg(s);
            }
            break;
        }
        case BroadcasterMsgId::MessageToBroadcast :
        {
            auto bmtb{deserializeStruct<BroadcasterMessageToBroadcast>(msgString)};
            auto s {serializeStruct<SequencerBroadcastMessage>(SequencerBroadcastMessage{SequencerMsgId::BroadcastMessage,
                                                                                         bmtb.senderRank,
                                                                                         ++seqNum,
                                                                                         bmtb.sessionMsg})};
            getSession()->getCommLayer()->broadcastMsg(s);
            break;
        }
        default:
        {
            cerr << "ERROR\tSequencerAlgoLayer / Sequencer : Unexpected broadcasterMsgTyp (" << static_cast<int>(broadcasterMsgTyp) << ")\n";
            exit(EXIT_FAILURE);
        }
    }
}

void SequencerAlgoLayer::callbackHandleMessageAsNonHostPeer(std::unique_ptr<CommPeer> peer, const std::string &msgString)
{
    thread_local int nextDeliver{1};
    switch (SequencerMsgId sequencerMsgId{ static_cast<SequencerMsgId>(msgString[0]) }; sequencerMsgId)
    {
        case SequencerMsgId::AckDisconnectIntent :
            peer->disconnect();
            break;
        case SequencerMsgId::AllBroadcastersConnected :
            getSession()->callbackInitDone();
            break;
        case SequencerMsgId::BroadcastMessage :
        {
            auto sbm {deserializeStruct<SequencerBroadcastMessage>(msgString)};
            if (nextDeliver != sbm.seqNum)
            {
                cerr << "ERROR\tSequencerAlgoLayer / Broadcaster #" << getSession()->getRank() << " : Received a totalOrderBroadcast message with seqNum=" << sbm.seqNum << " while nextDeliver=" << nextDeliver << "\n";
                exit(EXIT_FAILURE);
            }
            getSession()->callbackDeliver(sbm.senderRank, sbm.seqNum, sbm.sessionMsg);
            ++nextDeliver;
            break;
        }
        default:
        {
            cerr << "ERROR\tSequencerAlgoLayer / Broadcaster #" << getSession()->getRank() << " : Unexpected sequencerMsgId (" << static_cast<int>(sequencerMsgId) << ")\n";
            exit(EXIT_FAILURE);
        }
    }
}

bool SequencerAlgoLayer::executeAndProducedStatistics()
{
    // In Sequencer algorithm, the last site is not broadcasting. We build @broadcasters vector accordingly.
    std::vector<HostTuple> broadcasters = getSession()->getParam().getSites();
    broadcasters.pop_back();
    setBroadcasters(broadcasters);

    auto commLayer = getSession()->getCommLayer();
    if (getSession()->getRank() == getSession()->getParam().getSites().size() - 1)
    {
        // Process is sequencer
        auto sites = getSession()->getParam().getSites();
        if (getSession()->getParam().getVerbose())
            cout << "\tSequencerAlgoLayer / Sequencer : Wait for connections on port " << get<PORT>(sites.back()) << "\n";
        commLayer->initHost(get<PORT>(sites.back()), getBroadcasters().size(), this);
        if (getSession()->getParam().getVerbose())
            cout << "\tSequencerAlgoLayer / Sequencer : Wait for messages\n";
        commLayer->waitForMsg(false, getBroadcasters().size());
        if (getSession()->getParam().getVerbose())
            cout << "\tSequencerAlgoLayer / Sequencer : Finished waiting for messages ==> Giving back control to SessionLayer\n";
        return false;
    }
    else
    {
        // Process is a broadcaster
        if (getSession()->getParam().getVerbose())
            cout << "\tSequencerAlgoLayer / Broadcaster #" << getSession()->getRank() << " : Connect to sequencer at " << get<HOSTNAME>(getSession()->getParam().getSites().back()) << ":" << get<PORT>(getSession()->getParam().getSites().back()) << "\n";
        sequencerPeer = getSession()->getCommLayer()->connectToHost(getSession()->getParam().getSites().back(), this);

        // Send RankInfo to sequencer
        if (getSession()->getParam().getVerbose())
            cout << "\tSequencerAlgoLayer / Broadcaster #" << getSession()->getRank() << " : Send RankInfo to sequencer\n";
        auto s {serializeStruct<BroadcasterRankInfo>(BroadcasterRankInfo{BroadcasterMsgId::RankInfo, static_cast<unsigned char>(getSession()->getRank())})};
        sequencerPeer->sendMsg(s );

        if (getSession()->getParam().getVerbose())
            cout << "\tSequencerAlgoLayer / Broadcaster #" << getSession()->getRank() << " : Wait for messages\n";
        commLayer->waitForMsg(true, 1); // maxDisconnections is 1, because we have only a connection with sequencer.
        if (getSession()->getParam().getVerbose())
            cout << "\tSequencerAlgoLayer / Broadcaster #" << getSession()->getRank() << " : Finished waiting for messages ==> Giving back control to SessionLayer\n";
        return true;
    }
}

void SequencerAlgoLayer::totalOrderBroadcast(const std::string &msg) {
    // Send MessageToBroadcast to sequencer
    auto bmtb {serializeStruct<BroadcasterMessageToBroadcast>(BroadcasterMessageToBroadcast{BroadcasterMsgId::MessageToBroadcast,
                                                                                            static_cast<unsigned char>(getSession()->getRank()),
                                                                                            msg})};
    sequencerPeer->sendMsg(bmtb );
}

void SequencerAlgoLayer::terminate() {
    // Send DisconnectIntent to sequencer
    auto s {serializeStruct<BroadcasterDisconnectIntent>(BroadcasterDisconnectIntent{BroadcasterMsgId::DisconnectIntent, static_cast<unsigned char>(getSession()->getRank())})};
    sequencerPeer->sendMsg(s );

}

std::string SequencerAlgoLayer::toString() {
    return "Sequencer";
}
