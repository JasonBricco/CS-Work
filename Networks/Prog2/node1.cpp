// Jason Bricco (jmbricco)
// CS4461 Program 2
// 11/10/20

#include <stdio.h>
#include "dv.h"
#include "shared.h"

extern int TraceLevel;
extern float clocktime;

static distance_table dt1;
static NeighborCosts* neighbor1;

static void SendToNeighbors()
{
    int count = neighbor1->NodesInNetwork;
    SendPacket(&dt1, 1, 0, count, clocktime);
    SendPacket(&dt1, 1, 2, count, clocktime);
}

void rtinit1() 
{
    printf("At time t=%.3f, rtinit1() called.\n", clocktime);

    neighbor1 = InitDistanceTable(1, &dt1);
    PrintTable(1, neighbor1, &dt1);

    SendToNeighbors();
    printf("\n");
}

void rtupdate1(RoutePacket* rcvdpkt) 
{
    printf("rtupdate1 called at %.3f from sender %d\n", clocktime, rcvdpkt->sourceid);

    if (UpdateTable(&dt1, rcvdpkt, neighbor1->NodesInNetwork))
    {
        PrintTable(1, neighbor1, &dt1);
        SendToNeighbors();
    }
}
