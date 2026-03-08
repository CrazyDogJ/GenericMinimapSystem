#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "ZoneGraphTypes.h"

struct FZoneGraphLaneNodeRef
{
	int32 LaneIndex = INDEX_NONE;
	float LaneDistance = -1;

	FZoneGraphLaneNodeRef() {}
	
	explicit FZoneGraphLaneNodeRef(int32 InInt) {}

	FZoneGraphLaneNodeRef(const int32& InLaneIndex, const float& InLaneDistance)
		: LaneIndex(InLaneIndex), LaneDistance(InLaneDistance)
	{}
	
	bool operator==(const FZoneGraphLaneNodeRef& Other) const
	{
		return Other.LaneIndex == LaneIndex && Other.LaneDistance == LaneDistance;
	}

	bool IsValid() const
	{
		return LaneIndex != INDEX_NONE && LaneDistance >= 0.0f;
	}
};

FORCEINLINE uint32 GetTypeHash(const FZoneGraphLaneNodeRef& NodeRef)
{
	uint32 Hash = GetTypeHash(NodeRef.LaneIndex);
	Hash = HashCombine(Hash, GetTypeHash(NodeRef.LaneDistance));
	return Hash;
}

struct FZoneGraphCustomAStarNode;

/** Warpper around zone graph to be used by FGraphAStar */
struct FZoneGraphCustomAStarWrapper
{
	FZoneGraphCustomAStarWrapper(const FZoneGraphStorage& InZoneGraph, const FZoneGraphLaneLocation& InStartLocation, const FZoneGraphLaneLocation& InEndLocation)
		: ZoneGraph(InZoneGraph), StartLocation(InStartLocation), EndLocation(InEndLocation)
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// FGraphAStar: TGraph
	typedef FZoneGraphLaneNodeRef FNodeRef;

	FORCEINLINE bool IsValidRef(const FNodeRef NodeRef) const
	{
		return NodeRef.IsValid();
	}

	static float GetZoneWidth(const FZoneGraphStorage& ZoneGraph, int32 ZoneIndex);
	static int32 GetOutgoingLink(const FZoneGraphStorage& ZoneGraph, int32 LaneIndex);
	
	int32 GetNeighbourCountV2(const FZoneGraphCustomAStarNode& Node) const;
	FNodeRef GetNeighbour(const FZoneGraphCustomAStarNode& Node, const int32 NeighbourIndex) const;
	//////////////////////////////////////////////////////////////////////////

protected:
	const FZoneGraphStorage& ZoneGraph;
	const FZoneGraphLaneLocation StartLocation;
	const FZoneGraphLaneLocation EndLocation;

	mutable FNodeRef EndLocationSpecial = FNodeRef(INDEX_NONE);
};

/** Node representation for FZoneGraphCustomAStar */
struct FZoneGraphCustomAStarNode : public FGraphAStarDefaultNode<FZoneGraphCustomAStarWrapper>
{
	typedef FGraphAStarDefaultNode<FZoneGraphCustomAStarWrapper> Super;
	typedef FZoneGraphLaneNodeRef FNodeRef;

	FORCEINLINE FZoneGraphCustomAStarNode(const FNodeRef InNodeRef = FZoneGraphLaneNodeRef())
		: Super(InNodeRef)
	{}

	FZoneGraphCustomAStarNode(const FZoneGraphCustomAStarNode& Other) = default;
	FGraphAStarDefaultNode& operator=(const FGraphAStarDefaultNode& Other) = delete;
};

/** Context for FGraphAStar::FindPath() */
struct FZoneGraphCustomPathFilter
{
	// @todo: rename FZoneGraphPathfindContext?

	typedef FZoneGraphLaneNodeRef FNodeRef;

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
