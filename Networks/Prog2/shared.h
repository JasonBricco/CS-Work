// Jason Bricco (jmbricco)
// CS4461 Program 2
// 11/10/20

struct distance_table 
{
    int costs[MAX_NODES][MAX_NODES];
};

// Initializes the distance table with direct costs from this node to 
// each other node.
inline NeighborCosts* InitDistanceTable(int node, distance_table* dt)
{
	NeighborCosts* costs = getNeighborCosts(node);

	int nodeCount = costs->NodesInNetwork;

	for (int i = 0; i < nodeCount; ++i)
	{
		for (int j = 0; j < nodeCount; ++j)
			dt->costs[i][j] = INFINITY;
	}

    for (int i = 0; i < costs->NodesInNetwork; ++i)
        dt->costs[i][i] = costs->NodeCosts[i];

    return costs;
}

// Returns the minimum cost from 'src' to 'dst' node.
inline int GetMinCost(distance_table* dt, int src, int dst, int nodeCount)
{
    int cost = INFINITY;

    for (int j = 0; j < nodeCount; ++j)
    {
        if (j == src) continue;

        if (dt->costs[dst][j] < cost)
            cost = dt->costs[dst][j];
    }

    return cost;
}

// Sends a packet from 'src' to 'dst' node with the minimum cost information from
// 'src' to all other nodes.
inline void SendPacket(distance_table* dt, int src, int dst, int nodeCount, float time)
{
	RoutePacket packet = { src, dst };

	char buf[128] = {};
	int len = sprintf(buf, "At time t=%.3f, node %d sends packet to node %d with: ", time, src, dst);

	for (int i = 0; i < nodeCount; ++i)
	{
        if (i == src) continue;

		packet.mincost[i] = GetMinCost(dt, src, i, nodeCount);
		len += sprintf(buf + len, "%d ", packet.mincost[i]);
	}

	printf("%s\n", buf);
	toLayer2(packet);
}

// Updates this node's distance table by determining if routing through another
// node provides a lower cost path than the current known cost.
inline bool UpdateTable(distance_table* dt, RoutePacket* received, int nodeCount)
{
	bool update = false;
	
    int otherNode = received->sourceid;
    int curNode = received->destid;

    // Cost from this node to the node that sent this packet.
    int curToOther = GetMinCost(dt, curNode, otherNode, nodeCount);

    for (int i = 0; i < nodeCount; ++i)
    {
        if (i == curNode) continue;

        // Min cost from the other node that sent this packet to node i.
        int otherToDst = received->mincost[i];

        int newCost = curToOther + otherToDst;

        // If going through the other node to node i has a lower
        // cost, update this distance table with that cost.
        if (newCost < dt->costs[i][otherNode])
        {
            dt->costs[i][otherNode] = newCost;
            update = true;
        }
    }

    return update;
}

/////////////////////////////////////////////////////////////////////
//  printdt
//  This routine is being supplied to you.  It is the same code in
//  each node and is tailored based on the input arguments.
//  Required arguments:
//  MyNodeNumber:  This routine assumes that you know your node
//                 number and supply it when making this call.
//  struct NeighborCosts *neighbor:  A pointer to the structure 
//                 that's supplied via a call to getNeighborCosts().
//                 It tells this print routine the configuration
//                 of nodes surrounding the node we're working on.
//  struct distance_table *dtptr: This is the running record of the
//                 current costs as seen by this node.  It is 
//                 constantly updated as the node gets new
//                 messages from other nodes.
/////////////////////////////////////////////////////////////////////
inline void PrintTable(int myNode, NeighborCosts* neighbor, distance_table* dtptr) 
{
    int totalNodes = neighbor->NodesInNetwork;
    int numNeighbors = 0;
    int neighbors[MAX_NODES];

    // Determine our neighbors,
    for (int i = 0; i < totalNodes; i++)  
    {
        if ((neighbor->NodeCosts[i] != INFINITY) && i != myNode)
        {
            neighbors[numNeighbors] = i;
            numNeighbors++;
        }
    }

    // Print the header.
    printf("                via     \n");
    printf("   D%d |", myNode);

    for (int i = 0; i < numNeighbors; i++)
        printf("     %d", neighbors[i]);

    printf("\n");
    printf("  ----|-------------------------------\n");

    // For each node, print the cost by traveling through each of our neighbors.
    for (int i = 0; i < totalNodes; i++)
    {
        if (i != myNode)
        {
            printf("dest %d|", i);

            for (int j = 0; j < numNeighbors; j++)
                printf( "  %4d", dtptr->costs[i][neighbors[j]]);

            printf("\n");
        }
    }

    printf("\n");
}
