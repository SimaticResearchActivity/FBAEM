#pragma once

#include <string>
#include <tuple>
#include <vector>
#include "OptParserExtended.h"

// The following value has been found experimentally when filler field of ClientMessageToBroadcast has size 0
constexpr int minSizeClientMessageToBroadcast{22};

// Maximum length of a UDP packet
constexpr size_t maxLength{65515};

// Constexpr to access elements of tuple sites
constexpr int HOST{0};
constexpr int PORT{1};

class Param {
private:
    int nbMsg{0};
    int rank{0};
    int sizeMsg{0};
    std::string siteFile{};
    std::vector<std::tuple<std::string, int>> sites;
    bool verbose{false};

public:
    explicit Param(mlib::OptParserExtended const& parser);
    static std::string csvHeadline();
    [[nodiscard]] std::string asCsv(std::string const& algoStr, std::string const& commLayerStr) const;
};

