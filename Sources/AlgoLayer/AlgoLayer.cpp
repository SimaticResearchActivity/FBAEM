#include "AlgoLayer.h"

void AlgoLayer::setSession(SessionLayer *aSession)
{
    session = aSession;
}

SessionLayer* AlgoLayer::getSession() const {
    return session;
}

const int &AlgoLayer::getBroadcasters() const {
    return broadcasters;
}

void AlgoLayer::setBroadcasters(int broadcastersSize) {
    broadcasters = broadcastersSize;
}
