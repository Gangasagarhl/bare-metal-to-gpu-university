// incident_fixed.cpp - the same run with the option restored (forensic answer key).
#include "incident.h"

int main()
{
    raft::Options restored;  // heartbeat 50 ms: the library default
    incidentReport("same run, heartbeat restored", restored, 21);
    return 0;
}
