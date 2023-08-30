#pragma once

#include "AlgoLayer.h"

class SequencerAlgoLayer : public AlgoLayer {
private:
    /**
     * @brief @CommPeer used to send messages to sequencer.
     */
    std::unique_ptr<CommPeer> sequencerPeer;
public:
    bool executeAndProducedStatistics() override;
    void handleMessageAsHost(std::unique_ptr<CommPeer> peer, const std::string &msgString) override;
    void handleMessageAsNonHostPeer(std::unique_ptr<CommPeer> peer, const std::string &msgString) override;
    void totalOrderBroadcast(const std::string &msg) override;
    void terminate() override;
    std::string toString() override;
};