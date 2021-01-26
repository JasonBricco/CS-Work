// Jason Bricco (jmbricco)
// CS4461 Program 2
// 11/10/20

#include <stdio.h>
#include "dv.h"
#include "shared.h"

extern int TraceLevel;
extern float clocktime;

static distance_table dt0;
static NeighborCosts* neighbor0;

void rtinit4() {}
void rtinit5() {}
void rtinit6() {}
void rtinit7() {}
void rtinit8() {}
void rtinit9() {}
void rtupdate4(RoutePacket* rcvdpkt) {}
void rtupdate5(RoutePacket* rcvdpkt) {}
void rtupdate6(RoutePacket* rcvdpkt) {}
void rtupdate7(RoutePacket* rcvdpkt) {}
void rtupdate8(RoutePacket* rcvdpkt) {}
void rtupdate9(RoutePacket* rcvdpkt) {}

static void SendToNeighbors()
{
    int count = neighbor0->NodesInNetwork;
    SendPacket(&dt0, 0, 1, count, clocktime);
    SendPacket(&dt0, 0, 2, count, clocktime);
    SendPacket(&dt0, 0, 3, count, clocktime);
}

void rtinit0() 
{
    printf("At time t=%.3f, rtinit0() called.\n", clocktime);

    neighbor0 = InitDistanceTable(0, &dt0);
    PrintTable(0, neighbor0, &dt0); 

    SendToNeighbors();
    printf("\n");
}

void rtupdate0(RoutePacket* rcvdpkt) 
{
    printf("rtupdate0 called at %.3f from sender %d\n", clocktime, rcvdpkt->sourceid);

    if (UpdateTable(&dt0, rcvdpkt, neighbor0->NodesInNetwork))
    {
        PrintTable(0, neighbor0, &dt0); 
        SendToNeighbors();
    }
}
