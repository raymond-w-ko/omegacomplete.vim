#pragma once

#include "Participant.hpp"

class Room
{
public:
    void Join(ParticipantPtr participant);
    void Leave(ParticipantPtr participant);
private:
    std::set<ParticipantPtr> participants_;
};
