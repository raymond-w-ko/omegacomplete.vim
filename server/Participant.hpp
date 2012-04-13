#pragma once

class Participant
{
public:
    Participant();
    virtual ~Participant();
private:
};

typedef boost::shared_ptr<Participant> ParticipantPtr;
