#pragma once

#include "Participant.hpp"

class Room
{
public:
    void Join(ParticipantPtr participant);
    void Leave(ParticipantPtr participant);
private:
    set<ParticipantPtr> participants_;
};
