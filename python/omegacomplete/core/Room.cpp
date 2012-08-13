#include "stdafx.hpp"

#include "Room.hpp"

void Room::Join(ParticipantPtr participant)
{
    participants_.insert(participant);
}

void Room::Leave(ParticipantPtr participant)
{
    participants_.erase(participant);
}
