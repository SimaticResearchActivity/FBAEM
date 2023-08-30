#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

namespace fbae_SequencerAlgoLayer
{
    //---------------------------------------------------
    // Messages sent by the sequencer to broadcaster(s)
    //---------------------------------------------------
    enum class SequencerMsgId : unsigned char
    {
        AckDisconnectIntent = 65, // We start with a value which be displayed as a character in debugger
        AllBroadcastersConnected,
        BroadcastMessage
    };

    struct GenericSequencerMsgWithoutData
    {
        SequencerMsgId msgId{};

        // This method lets cereal know which data members to serialize
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(msgId); // serialize things by passing them to the archive
        }
    };

    using SequencerAckDisconnectIntent = GenericSequencerMsgWithoutData;
    using SequencerAllBroadcastersConnected = GenericSequencerMsgWithoutData;

    struct SequencerBroadcastMessage
    {
        SequencerMsgId msgId{};
        unsigned char senderRank{};
        int seqNum{};
        std::string sessionMsg;

        // This method lets cereal know which data members to serialize
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(msgId, senderRank, seqNum, sessionMsg); // serialize things by passing them to the archive
        }
    };

    //---------------------------------------------------
    // Messages sent by a broadcaster to the sequencer
    //---------------------------------------------------
    enum class BroadcasterMsgId : unsigned char
    {
        DisconnectIntent = 97, // We start with a value which be displayed as a character in debugger + the enum values are different from values in enum SequencerMsgId
        MessageToBroadcast,
        RankInfo,
    };

    struct GenericBroadcasterMsgWithRank
    {
        BroadcasterMsgId msgId{};
        unsigned char  senderRank{};

        // This method lets cereal know which data members to serialize
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(msgId, senderRank); // serialize things by passing them to the archive
        }
    };

    using BroadcasterDisconnectIntent = GenericBroadcasterMsgWithRank;
    using BroadcasterRankInfo = GenericBroadcasterMsgWithRank;

    struct BroadcasterMessageToBroadcast
    {
        BroadcasterMsgId msgId{};
        unsigned char senderRank{};
        std::string sessionMsg;

        // This method lets cereal know which data members to serialize
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(msgId, senderRank, sessionMsg); // serialize things by passing them to the archive
        }
    };


}