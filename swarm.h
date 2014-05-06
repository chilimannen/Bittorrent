#ifndef _swarm_h
#define _swarm_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "peerwire.h"
#include "protocol_meta.h"


 //return a free swarm
int swarm_select(char* info_hash, char* trackers[MAX_TRACKERS]);
//lock the mutex of swarm index
void swarm_lock(int index);
//unlock the mutex of swarm index
void swarm_unlock(int index);
//clear the swarm, before announcing to get rid of stale peers.
void swarm_reset(swarm_t* swarm);
//sets the swarm to listen mode
void swarm_listen(swarm_t* swarm);
//release the swarm, tracker stopped or failure.
void swarm_release(swarm_t* swarm);

#endif