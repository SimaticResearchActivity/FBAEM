#pragma once

#include <string>
#include <tuple>
#include <vector>
#include "OptParserExtended.h"

// The following value has been found experimentally when filler field of SenderMessageToBroadcast has size 0
constexpr int minSizeClientMessageToBroadcast{22};

// Maximum length of a UDP packet
constexpr size_t maxLength{65515};

constexpr int specialRankToRequestExecutionInTasks{99};

using HostTuple = std::tuple<std::string, int>;

// Constexpr to access elements of HostTuple
constexpr int HOSTNAME{0};
constexpr int PORT{1};

class Param {
private:
    int frequency{0};
    int maxBatchSize{INT32_MAX};
    int64_t nbMsg{0};
    int sizeMsg{0};
    std::vector<HostTuple> sites;
    bool verbose{false};
    int warmupCooldown{0};

public:
    explicit Param(mlib::OptParserExtended const& parser);
    [[nodiscard]] std::string
    asCsv(std::string const &algoStr, std::string const &rankStr) const;
    static std::string csvHeadline();
    [[nodiscard]] int getFrequency() const;
    [[nodiscard]] int getMaxBatchSize() const;
    [[nodiscard]] int64_t getNbMsg() const;
    [[nodiscard]] int getSizeMsg() const;
    [[nodiscard]] bool getVerbose() const;
    [[nodiscard]] int getWarmupCooldown() const;
};

