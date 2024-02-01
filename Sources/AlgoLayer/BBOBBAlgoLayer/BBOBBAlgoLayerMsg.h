#ifndef FBAE_BBOBBALGOLAYERMSG_H
#define FBAE_BBOBBALGOLAYERMSG_H

#include "cereal/archives/binary.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "../../basicTypes.h"

namespace fbae_BBOBBAlgoLayer {

    enum class MsgId : MsgId_t {
    };


    struct BatchSessionMsg {
        rank_t senderRank{};
        std::vector<std::string> batchSessionMsg;

        // This method lets cereal know which data members to serialize
        template<class Archive>
        void serialize(Archive &archive) {
            archive(senderRank, batchSessionMsg); // serialize things by passing them to the archive
        }
    };

    struct StepMsg {
        MsgId msgId{};
        unsigned char senderRank{};
        int wave;
        int step;
        std::vector<BatchSessionMsg> batchesBroadcast;

        // This method lets cereal know which data members to serialize
        template<class Archive>
        void serialize(Archive &archive) {
            archive(msgId, senderRank, wave, step, batchesBroadcast); // serialize things by passing them to the archive
        }
    };

}

#endif //FBAE_BBOBBALGOLAYERMSG_H
