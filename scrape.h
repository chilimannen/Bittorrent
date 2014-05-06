#ifndef _scrape_h
#define _scrape_h

 #include <netdb.h>
 #include <unistd.h>
 #include <errno.h>
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <netinet/in.h>
 #include <arpa/inet.h> 
 #include "urlparse.h" 
 #include "protocol_meta.h"

//scrapes the trackerlist and stores the scrape data per tracker.
void tracker_scrape(swarm_t* swarm);


#endif