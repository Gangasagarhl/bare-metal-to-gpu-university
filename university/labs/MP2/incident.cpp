// incident.cpp - forensic evidence: the run after "the change that reduced chatter".
#include "incident.h"

int main()
{
    raft::Options deployed;
    deployed.heartbeat = 200;  // the change under suspicion is in this line (see the key)
    incidentReport("game day 2, scenario 'quiet afternoon': SLO alert fired", deployed, 21);
    return 0;
}
