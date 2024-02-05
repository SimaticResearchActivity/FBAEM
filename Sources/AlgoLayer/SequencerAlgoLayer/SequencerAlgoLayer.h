#pragma once

#include "../AlgoLayer.h"

class SequencerAlgoLayer : public AlgoLayer {
private:

    int rank;
    int size;
    bool receive = true;

public:
    bool executeAndProducedStatistics() override;
    void totalOrderBroadcast(std::string && msg) override;
    void terminate() override;
    std::string toString() override;
};