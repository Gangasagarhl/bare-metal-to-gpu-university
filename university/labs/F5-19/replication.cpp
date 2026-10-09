// Log replication: catch-up of a lagging follower and repair of a conflicting log.
#include "replication_story.h"

int main()
{
    return replicationStory(false);
}
