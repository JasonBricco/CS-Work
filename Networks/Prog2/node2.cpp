// Jason Bricco (jmbricco)
// CS4461 Program 2
// 11/10/20

#include <stdio.h>
#include "dv.h"
#include "shared.h"

extern int TraceLevel;
extern float clocktime;

static distance_table dt2;
static NeighborCosts* neighbor2;

static void SendToNeighbors()
{
    int count = neighbor2->NodesInNetwork;
    SendPacket(&dt2, 2, 0, count, clocktime);
    SendPacket(&dt2, 2, 1, count, clocktime);
    SendPacket(&dt2, 2, 3, count, clocktime);
}

void rtinit2() 
{
    printf("At time t=%.3f, rtinit2() called.\n", clocktime);

    neighbor2 = InitDistanceTable(2, &dt2);
    PrintTable(2, neighbor2, &dt2); 

    SendToNeighbors();
    printf("\n");
}

void rtupdate2(RoutePacket* rcvdpkt) 
{
    printf("rtupdate2 called at %.3f from sender %d\n", clocktime, rcvdpkt->sourceid);

    if (UpdateTable(&dt2, rcvdpkt, neighbor2->NodesInNetwork))
    {
        PrintTable(2, neighbor2, &dt2);
        SendToNeighbors();
    }
}
