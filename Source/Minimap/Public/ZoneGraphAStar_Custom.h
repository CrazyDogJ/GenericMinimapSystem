#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "ZoneGraphTypes.h"

struct FZoneGraphCustomAStarNode;

/** Warpper around zone graph to be used by FGraphAStar */
struct FZoneGraphCustomAStarWrapper
{
	FZoneGraphCustomAStarWrapper(const FZoneGraphStorage& InZoneGraph) 
		: ZoneGraph(InZoneGraph) 
	{}

	//////////////////////////////////////////////////////////////////////////
	// FGraphAStar: TGraph
	typedef int32/*lane index type*/ FNodeRef;

	FORCEINLINE bool IsValidRef(const FNodeRef NodeRef) const
	{
		return NodeRef != INDEX_NONE;
	}

	int32 GetNeighbourCountV2(const FZoneGraphCustomAStarNode& Node) const;
	FNodeRef GetNeighbour(const FZoneGraphCustomAStarNode& Node, const int32 NeighbourIndex) const;
	//////////////////////////////////////////////////////////////////////////

protected:
	const FZoneGraphStorage& ZoneGraph;
};

/** Node representation for FZoneGraphCustomAStar */
struct FZoneGraphCustomAStarNode : public FGraphAStarDefaultNode<FZoneGraphCustomAStarWrapper>
{
	typedef FGraphAStarDefaultNode<FZoneGraphCustomAStarWrapper> Super;
	typedef int32/*lane index type*/ FNodeRef;

	FORCEINLINE FZoneGraphCustomAStarNode(const FNodeRef InNodeRef = INDEX_NONE, const FVector InPosition = FVector(TNumericLimits<FVector::FReal>::Max()))
		: Super(InNodeRef)
		, Position(InPosition)
	{}

	FZoneGraphCustomAStarNode(const FZoneGraphCustomAStarNode& Other) = default;
	FGraphAStarDefaultNode& operator=(const FGraphAStarDefaultNode& Other) = delete;

	bool IsStartOrIsEnd() const;

	FVector Position; //@todo: this will likely change to be a position along the lane
};

/** Context for FGraphAStar::FindPath() */
struct FZoneGraphCustomPathFilter
{
	// @todo: rename FZoneGraphPathfindContext?

	typedef int32/*lane index type*/ FNodeRef;

	FZoneGraphCustomPathFilter(const FZoneGraphStorage& InGraph, const FZoneGraphLaneLocation& InStartLocation, const FZoneGraphLaneLocation& InEndLocation, const FZoneGraphTagFilter InZoneTagFilter = FZoneGraphTagFilter())
		: ZoneStorage(InGraph) 
		, ZoneTagFilter(InZoneTagFilter)
		, StartLocation(InStartLocation)
		, EndLocation(InEndLocation)
	{}

	bool IsStart(const FZoneGraphCustomAStarNode& Node) const;
	bool IsEnd(const FZoneGraphCustomAStarNode& Node) const;

	FVector::FReal GetHeuristicScale() const;

	FVector::FReal GetHeuristicCost(const FZoneGraphCustomAStarNode& NeighbourNode, const FZoneGraphCustomAStarNode& EndNode) const;

	FVector::FReal GetTraversalCost(const FZoneGraphCustomAStarNode& CurNode, const FZoneGraphCustomAStarNode& NeighbourNode) const;

	bool IsTraversalAllowed(const FNodeRef StartNodeRef, const FNodeRef& Neighbour) const;

	bool WantsPartialSolution() const;
	bool ShouldIncludeStartNodeInPath() const;

protected:
	const FZoneGraphStorage& ZoneStorage;
	const FZoneGraphTagFilter ZoneTagFilter;
	const FZoneGraphLaneLocation StartLocation;
	const FZoneGraphLaneLocation EndLocation;
};

/** A Star algorithm using lanes on zone graph */
struct FZoneGraphCustomAStar : public FGraphAStar<FZoneGraphCustomAStarWrapper, FGraphAStarDefaultPolicy, FZoneGraphCustomAStarNode>
{
	typedef FGraphAStar<FZoneGraphCustomAStarWrapper, FGraphAStarDefaultPolicy, FZoneGraphCustomAStarNode> Super;

	FZoneGraphCustomAStar(const FZoneGraphCustomAStarWrapper& Graph)
		: Super(Graph)
	{}
};
