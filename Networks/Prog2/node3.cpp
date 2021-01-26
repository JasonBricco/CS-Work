// Jason Bricco (jmbricco)
// CS4461 Program 2
// 11/10/20

#include <stdio.h>
#include "dv.h"
#include "shared.h"

extern int TraceLevel;
extern float clocktime;

static distance_table dt3;
static NeighborCosts* neighbor3;

static void SendToNeighbors()
{
    int count = neighbor3->NodesInNetwork;
    SendPacket(&dt3, 3, 0, count, clocktime);
    SendPacket(&dt3, 3, 2, count, clocktime);
}

void rtinit3()
{
    printf("At time t=%.3f, rtinit3() called.\n", clocktime);

    neighbor3 = InitDistanceTable(3, &dt3);
    PrintTable(3, neighbor3, &dt3); 

    SendToNeighbors();
    printf("\n");
}

void rtupdate3(RoutePacket *rcvdpkt) 
{
    printf("rtupdate3 called at %.3f from sender %d\n", clocktime, rcvdpkt->sourceid);

    if (UpdateTable(&dt3, rcvdpkt, neighbor3->NodesInNetwork))
    {
        PrintTable(3, neighbor3, &dt3); 
        SendToNeighbors();
    }
}
