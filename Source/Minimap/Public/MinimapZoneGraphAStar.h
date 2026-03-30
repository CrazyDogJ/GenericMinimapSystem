#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "ZoneGraphTypes.h"
#include "MinimapZoneGraphAStar.generated.h"

// Zone Graph Minimap AStar ------------------------------------------------------------

struct FMinimapZoneGraphLaneNodeRef
{
	int32 LaneIndex = INDEX_NONE;
	float LaneDistance = -1;

	FMinimapZoneGraphLaneNodeRef() {}
	
	explicit FMinimapZoneGraphLaneNodeRef(int32 InInt) {}

	FMinimapZoneGraphLaneNodeRef(const int32& InLaneIndex, const float& InLaneDistance)
		: LaneIndex(InLaneIndex), LaneDistance(InLaneDistance)
	{}
	
	bool operator==(const FMinimapZoneGraphLaneNodeRef& Other) const
	{
		return Other.LaneIndex == LaneIndex && Other.LaneDistance == LaneDistance;
	}

	bool IsValid() const
	{
		return LaneIndex != INDEX_NONE && LaneDistance >= 0.0f;
	}
};

FORCEINLINE uint32 GetTypeHash(const FMinimapZoneGraphLaneNodeRef& NodeRef)
{
	uint32 Hash = GetTypeHash(NodeRef.LaneIndex);
	Hash = HashCombine(Hash, GetTypeHash(NodeRef.LaneDistance));
	return Hash;
}

struct FMinimapZoneGraphAStarNode;

/** Warpper around zone graph to be used by FGraphAStar */
struct FMinimapZoneGraphAStarWrapper
{
	FMinimapZoneGraphAStarWrapper(const FZoneGraphStorage& InZoneGraph, const FZoneGraphLaneLocation& InStartLocation, const FZoneGraphLaneLocation& InEndLocation)
		: ZoneGraph(InZoneGraph), StartLocation(InStartLocation), EndLocation(InEndLocation)
	{
		CachedZoneWidth = GetZoneWidth(ZoneGraph, StartLocation.LaneHandle.Index);
		SetStartSpecial();
		SetEndSpecial();
	}

	//////////////////////////////////////////////////////////////////////////
	// FGraphAStar: TGraph
	typedef FMinimapZoneGraphLaneNodeRef FNodeRef;

	FORCEINLINE bool IsValidRef(const FNodeRef NodeRef) const
	{
		return NodeRef.IsValid();
	}

	static float GetZoneWidth(const FZoneGraphStorage& ZoneGraph, int32 LaneIndex);
	static int32 GetLinkByType(const FZoneGraphStorage& ZoneGraph, EZoneLaneLinkType LinkType, int32 LaneIndex);
	FZoneGraphLaneLocation QueryLaneLocationByLocation(const FVector& CheckLocation, int32 TargetLaneIndex) const;

	void SetEndSpecial();
	void SetStartSpecial();
	
	int32 GetNeighbourCountV2(const FMinimapZoneGraphAStarNode& Node) const;
	FNodeRef GetNeighbour(const FMinimapZoneGraphAStarNode& Node, const int32 NeighbourIndex) const;
	//////////////////////////////////////////////////////////////////////////

protected:
	const FZoneGraphStorage& ZoneGraph;
	const FZoneGraphLaneLocation StartLocation;
	const FZoneGraphLaneLocation EndLocation;

	float CachedZoneWidth = 0.0f;
	FNodeRef StartLocationSpecial = FNodeRef(INDEX_NONE);
	FNodeRef EndLocationSpecial = FNodeRef(INDEX_NONE);
};

/** Node representation for FMinimapZoneGraphAStar */
struct FMinimapZoneGraphAStarNode : public FGraphAStarDefaultNode<FMinimapZoneGraphAStarWrapper>
{
	typedef FGraphAStarDefaultNode<FMinimapZoneGraphAStarWrapper> Super;
	typedef FMinimapZoneGraphLaneNodeRef FNodeRef;

	FORCEINLINE FMinimapZoneGraphAStarNode(const FNodeRef InNodeRef = FMinimapZoneGraphLaneNodeRef())
		: Super(InNodeRef)
	{}

	FMinimapZoneGraphAStarNode(const FMinimapZoneGraphAStarNode& Other) = default;
	FGraphAStarDefaultNode& operator=(const FGraphAStarDefaultNode& Other) = delete;
};

/** Context for FGraphAStar::FindPath() */
struct FMinimapZoneGraphPathFilter
{
	// @todo: rename FZoneGraphPathfindContext?

	typedef FMinimapZoneGraphLaneNodeRef FNodeRef;

	FMinimapZoneGraphPathFilter(const FZoneGraphStorage& InGraph, const FZoneGraphLaneLocation& InStartLocation, const FZoneGraphLaneLocation& InEndLocation, const FZoneGraphTagFilter InZoneTagFilter = FZoneGraphTagFilter())
		: ZoneStorage(InGraph) 
		, ZoneTagFilter(InZoneTagFilter)
		, StartLocation(InStartLocation)
		, EndLocation(InEndLocation)
	{}

	bool IsStart(const FMinimapZoneGraphAStarNode& Node) const;
	bool IsEnd(const FMinimapZoneGraphAStarNode& Node) const;

	FVector::FReal GetHeuristicScale() const;

	FVector::FReal GetHeuristicCost(const FMinimapZoneGraphAStarNode& NeighbourNode, const FMinimapZoneGraphAStarNode& EndNode) const;

	FVector::FReal GetTraversalCost(const FMinimapZoneGraphAStarNode& CurNode, const FMinimapZoneGraphAStarNode& NeighbourNode) const;

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
struct FMinimapZoneGraphAStar : public FGraphAStar<FMinimapZoneGraphAStarWrapper, FGraphAStarDefaultPolicy, FMinimapZoneGraphAStarNode>
{
	typedef FGraphAStar<FMinimapZoneGraphAStarWrapper, FGraphAStarDefaultPolicy, FMinimapZoneGraphAStarNode> Super;

	FMinimapZoneGraphAStar(const FMinimapZoneGraphAStarWrapper& Graph)
		: Super(Graph)
	{}
};

// Zone Graph Minimap Usage ----------------------------------------------------------

/** Minimap zone graph lane path for blueprint. */
USTRUCT(BlueprintType)
struct FMinimapZoneGraphLanePath
{
	GENERATED_BODY()
	
	FZoneGraphLanePath Path;
	TArray<float> Distances;
};

/** Minimap zone graph a star functions library. */
UCLASS()
class MINIMAP_API UMinimapZoneGraphAStarLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** Get zone actor width by lane index. */
	static float GetZoneWidthByLaneIndex(const UObject* WorldContext, const FZoneGraphStorage& ZoneStorage, int32 LaneIndex);

	/** Calculate a zone graph path for other usage. */
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContext"))
	static bool GetZoneGraphPathBP(const UObject* WorldContext, FVector StartPosition, FVector DestPosition, FVector SearchExtent, FMinimapZoneGraphLanePath& Path);

	/** Generate path points by given zone graph path bp. */
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContext"))
	static bool GetPathPoints(const UObject* WorldContext, const FMinimapZoneGraphLanePath& Path, TArray<FVector>& PathPoints);

	/** Convert lane start and end to a list of points. */
	static TArray<FVector> ConvertLaneToPoints(const FZoneGraphStorage& ZoneStorage, const FZoneGraphLaneLocation& InStartLocation, const FZoneGraphLaneLocation& InEndLocation);
};