#pragma once
#include <atomic>
#include <chrono>
#include <string>
#include <vector>

class Measures {
public:
    explicit Measures(size_t nb_rtts_max);
    void add(std::chrono::duration<double, std::milli> const &elapsed);
    std::string asCsv(int argNbMsg, size_t argNbClients);
    static std::string csvHeadline();
private:
    std::vector<std::chrono::duration<double, std::milli>> rtts; // Round-Trip Time
    std::atomic_size_t nbRtts{0};
};