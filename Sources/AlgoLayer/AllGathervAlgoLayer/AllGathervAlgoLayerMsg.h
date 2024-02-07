#ifndef FBAE_ALLGATHERVALGOLAYERMSG_H
#define FBAE_ALLGATHERVALGOLAYERMSG_H

#include "cereal/archives/binary.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "../../basicTypes.h"

namespace fbae_AllGathervAlgoLayer {

    enum class MsgId : MsgId_t {
        Message,
    };

    struct Message {
        MsgId msgId{};
        unsigned char senderRank{};
        std::vector<std::string> batchesBroadcast;

        // This method lets cereal know which data members to serialize
        template<class Archive>
        void serialize(Archive &archive) {
            archive(msgId, senderRank, batchesBroadcast); // serialize things by passing them to the archive
        }
    };

}

#endif //FBAE_ALLGATHERVALGOLAYERMSG_H
