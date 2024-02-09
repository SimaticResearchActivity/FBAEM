#include "Param.h"
#include <fstream>
#include <iostream>
#include <cereal/archives/json.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>

Param::Param(mlib::OptParserExtended const& parser)
: nbMsg{parser.getoptIntRequired('n')}
, sizeMsg{parser.getoptIntRequired('s')}
, verbose{parser.hasopt ('v')}
{
    if (parser.hasopt('f')) {
        frequency = parser.getoptIntRequired('f');
        if (frequency == 0) {
            std::cerr << "ERROR: Argument for frequency must be greater than 0 (zero)"
                      << std::endl
                      << parser.synopsis () << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    if (parser.hasopt('m')) {
        maxBatchSize = parser.getoptIntRequired('m');
        if (maxBatchSize < sizeMsg) {
            std::cerr << "ERROR: Argument for maxBatchSize must be greater than argument for sizeMsg"
                      << std::endl
                      << parser.synopsis () << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    if (parser.hasopt('w')) {
        warmupCooldown = parser.getoptIntRequired('w');
        if (warmupCooldown > 99) {
            std::cerr << "ERROR: Argument for warmupCooldown must be in [0,99]"
                      << std::endl
                      << parser.synopsis () << std::endl;
            exit(EXIT_FAILURE);
        }
    }

}

[[nodiscard]] std::string
Param::asCsv(std::string const &algoStr, std::string const &rankStr) const
{
    return std::string {
        algoStr + ","
        + "MPI" + ","
        + std::to_string(frequency) + ","
        + std::to_string(maxBatchSize) + ","
        + std::to_string(nbMsg) + ","
        + std::to_string(warmupCooldown) + "%,"
        + rankStr  + ","
        + std::to_string(sizeMsg) + ","
        + "Nosite"
    };
}

std::string Param::csvHeadline()
{
    return std::string { "algoLayer,commLayer,frequency,maxBatchSize,nbMsg,warmupCooldown,rank,sizeMsg,siteFile"};
}

int Param::getFrequency() const {
    return frequency;
}

int Param::getMaxBatchSize() const {
    return maxBatchSize;
}

int64_t Param::getNbMsg() const {
    return nbMsg;
}



int Param::getSizeMsg() const {
    return sizeMsg;
}
bool Param::getVerbose() const {
    return verbose;
}

int Param::getWarmupCooldown() const {
    return warmupCooldown;
}

