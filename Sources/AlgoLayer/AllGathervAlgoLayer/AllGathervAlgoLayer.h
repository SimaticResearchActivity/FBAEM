#ifndef FBAE_AllGathervALGOLAYER_H
#define FBAE_AllGathervALGOLAYER_H

#include <condition_variable>
#include <latch>
#include <map>

#include "cereal/archives/binary.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

#include "../AlgoLayer.h"
#include "AllGathervAlgoLayerMsg.h"

class AllGathervAlgoLayer : public AlgoLayer {
private :

    /**
     * @brief Mutex coupled with @condVarBatchCtrl to control that batch of messages in msgsWaitingToBeBroadcast is not
     * too big
     */
    std::mutex mtxBatchCtrl;

    /**
     * @brief Condition variable coupled with @mtxBatchCtrl to control that batch of messages in msgsWaitingToBeBroadcast is not
     * too big
     */
    std::condition_variable condVarBatchCtrl;

    /**
     * @brief Variable used to shortcut BatchCtrl mechanism (with @mtxBatchCtrl and @condVarBatchCtrl), so that, in
     * order to avoid deadlocks, we accept that the number of bytes stored in @msgsWaitingToBeBroadcast is greater than
     * @maxBatchSize of @Param instance.
     */
    bool shortcutBatchCtrl;


    std::vector<std::string> msgsWaitingToBeBroadcast;

    int size;
    int rank;
    bool receive = true;

public :
    bool executeAndProducedStatistics() override;
    void totalOrderBroadcast(std::string && msg) override;
    void terminate() override;
    std::string toString() override;
};


#endif //FBAE_ALLGATHERVALGOLAYER_H


