#pragma once

#include "AlgoLayer.h"

class SequencerAlgoLayer : public AlgoLayer {
private:
    /**
     * @brief @CommPeer used to send messages to sequencer.
     */
    std::unique_ptr<CommPeer> sequencerPeer;
public:
    void callbackHandleMessageAsHost(std::unique_ptr<CommPeer> peer, const std::string &msgString) override;
    void callbackHandleMessageAsNonHostPeer(std::unique_ptr<CommPeer> peer, const std::string &msgString) override;
    bool executeAndProducedStatistics() override;
    void totalOrderBroadcast(const std::string &msg) override;
    void terminate() override;
    std::string toString() override;
};