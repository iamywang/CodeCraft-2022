#pragma once

#include "GraphInterface.hpp"
#include <cstring>
#include <list>
#include <queue>
#include <vector>

namespace Graph
{
namespace Algorithm
{

typedef struct
{
    int fromVertex;
    int toVertex;
    int flow; //正向流量
    // int reverse_flow; //反向流量 = capacity - flow
    int capacity;
} Edge;

class MaxFlowGraph : GraphInterface
{
  private:
    std::vector< std::vector< int > >
        adjLists; //每个list都是对应索引（顶点编号）的邻接表。
    //例如adjLists[0]表示编号为0的顶点的邻接表。
    // adjLists[0][0]表示编号为0的顶点的第一个邻接点的编号。

    std::vector< std::vector< Edge > >
        edges; //每一行都是包含对应顶点的所有边（有向边）
    std::vector< int > arrayLevel; //记录每个顶点的层次
    int currentVertex;
    int size;

  public:
    MaxFlowGraph( ) {}

    void initGraph( int numVertices )
    {
        this->size = numVertices;
        this->currentVertex = 0;
        this->adjLists.resize( numVertices );
        this->arrayLevel.resize( numVertices );
        this->edges.resize( numVertices );
    }

    void addEdge( int from, int to, int capacity )
    {
        adjLists[ from ].push_back( to );
        edges[ from ].push_back( { from, to, 0, capacity } );
        // edges[ to ].push_back( { from, to, 0, 0 } ); //感觉好像没啥用
    }

    int getMaxFlow( int source, int sink )
    {
        if ( source == sink )
        {
            return -1;
        }

        int maxFlow = 0;
        int visited[ size + 1 ];

        while ( isMoreFlowPossible( source, sink ) )
        {
            for ( int i = 0; i <= size; i++ )
            {
                visited[ i ] = 0;
            }

            int flow = 0;
            while ( ( flow = sendFlow( source, __INT32_MAX__, sink,
                                       visited ) ) != 0 )
            {
                maxFlow += flow;
            }
        }

        return maxFlow;
    }

    std::vector<std::vector<Edge>>& getEdges()
    {
        return edges;
    }

  private:
    /**
     * @brief
     * 判断是否还有更多流可以发送.使用的方法是BFS。从源头开始遍历，如果可以到达终点则说明
     * 可以发送更多的流。使用BFS的目的是建立层次图（level graph）。
     *
     * @param source
     * @param sink
     * @return int
     */
    int isMoreFlowPossible( int source, int sink )
    {
        auto &level = this->arrayLevel;
        int i;

        for ( i = 0; i < size; i++ )
        {
            level[ i ] = -1;
        }

        level[ source ] = 0;

        std::queue< int > q;
        q.push( source );

        while ( !q.empty( ) )
        {
            int u = q.front( );
            q.pop( );

            for ( int i = 0; i < adjLists[ u ].size( ); i++ )
            {
                Edge &edge = edges[ u ][ i ]; //参看addEdge函数

                if ( level[ edge.toVertex ] < 0   //保证不重复建立层次
                     && edge.flow < edge.capacity // possible to pass
                )
                {
                    level[ edge.toVertex ] = level[ u ] + 1; // set the level
                    q.push( edge.toVertex );
                }
            }
        }

        return ( level[ sink ] != -1 );
    }

    int sendFlow( int currentVertex, int currentFlow, int sink, int *visited )
    {
        if ( currentVertex == sink )
        {
            return currentFlow;
        }

        int sizeAdjList = adjLists[ currentVertex ].size( );
        auto &level = arrayLevel;

        while ( visited[ currentVertex ] < sizeAdjList )
        {
            int v = adjLists[ currentVertex ][ visited[ currentVertex ] ];
            Edge *edge = &edges[ currentVertex ][ visited[ currentVertex ] ];

            if ( level[ edge->toVertex ] ==
                     level[ currentVertex ] + 1 //按照层级访问
                 && edge->flow < edge->capacity //允许增加流量
            )
            {
                int minFlow = currentFlow;
                if ( edge->capacity - edge->flow < minFlow )
                {
                    minFlow = edge->capacity - edge->flow;
                }

                int tempFlow =
                    sendFlow( edge->toVertex, minFlow, sink, visited );

                if ( tempFlow > 0 )
                {

                    edge->flow += tempFlow;
                    // edges[currentVertex][ v ^ 1 ].flow -= tempFlow;

                    return tempFlow;
                }
            }
            visited[ currentVertex ]++;
        }

        return 0;
    }
};

} // namespace Algorithm

} // namespace Graph
