#include "MinimapZoneGraphAStar.h"

#include "MinimapSettings.h"
#include "ZoneGraphTypes.h"
#include "ZoneGraphQuery.h"
#include "ZoneGraphSubsystem.h"

float FMinimapZoneGraphAStarWrapper::GetZoneWidth(const FZoneGraphStorage& ZoneGraph, int32 LaneIndex)
{
	if (LaneIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	
	const auto ZoneData = ZoneGraph.Zones[ZoneGraph.Lanes[LaneIndex].ZoneIndex];
	const auto LaneCount = ZoneData.GetLaneCount();
	float Result = 0.0f;
	for (int i = 0; i < LaneCount; ++i)
	{
		float OutWidth;
		UE::ZoneGraph::Query::GetLaneWidth(ZoneGraph, ZoneData.LanesBegin + i, OutWidth);
		Result += OutWidth;
	}

	return Result;
}

int32 FMinimapZoneGraphAStarWrapper::GetLinkByType(const FZoneGraphStorage& ZoneGraph, EZoneLaneLinkType LinkType,
	int32 LaneIndex)
{
	if (LaneIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	
	const auto LaneData = ZoneGraph.Lanes[LaneIndex];
	const auto LaneLinkCount = LaneData.GetLinkCount();
	for (int i = 0; i < LaneLinkCount; ++i)
	{
		const auto ItrLinkIndex = LaneData.LinksBegin + i;
		const auto LaneLink = ZoneGraph.LaneLinks[ItrLinkIndex];
		if (LaneLink.Type == LinkType)
		{
			return ItrLinkIndex;
		}
	}

	return INDEX_NONE;
}

FZoneGraphLaneLocation FMinimapZoneGraphAStarWrapper::QueryLaneLocationByLocation(const FVector& CheckLocation, int32 TargetLaneIndex) const
{
	const float SearchDistance = CachedZoneWidth; // Arbitrary search dist
	FZoneGraphLaneLocation LocationOnCurrentLane;
	float DistanceSqr = 0.f;
	FBox Bounds(CheckLocation, CheckLocation);
	Bounds = Bounds.ExpandBy(SearchDistance);
	UE::ZoneGraph::Query::FindNearestLocationOnLane(
		ZoneGraph,
		FZoneGraphLaneHandle(TargetLaneIndex, ZoneGraph.DataHandle),
		Bounds,
		LocationOnCurrentLane,
		DistanceSqr
	);

	return LocationOnCurrentLane;
}

void FMinimapZoneGraphAStarWrapper::SetEndSpecial()
{
	// Get adjacent lane
	const auto AdjacentLinkIndex = GetLinkByType(ZoneGraph, EZoneLaneLinkType::Adjacent, EndLocation.LaneHandle.Index);
	if (AdjacentLinkIndex >= 0)
	{
		const auto AdjacentLaneIndex = ZoneGraph.LaneLinks[AdjacentLinkIndex].DestLaneIndex;
		const auto OutLaneLocation = QueryLaneLocationByLocation(EndLocation.Position, AdjacentLaneIndex);
		EndLocationSpecial = FMinimapZoneGraphLaneNodeRef(OutLaneLocation.LaneHandle.Index, OutLaneLocation.DistanceAlongLane);
	}
}

void FMinimapZoneGraphAStarWrapper::SetStartSpecial()
{
	// Get adjacent lane
	const auto AdjacentLinkIndex = GetLinkByType(ZoneGraph, EZoneLaneLinkType::Adjacent, StartLocation.LaneHandle.Index);
	if (AdjacentLinkIndex >= 0)
	{
		const auto AdjacentLaneIndex = ZoneGraph.LaneLinks[AdjacentLinkIndex].DestLaneIndex;
		const auto OutLaneLocation = QueryLaneLocationByLocation(StartLocation.Position, AdjacentLaneIndex);
		StartLocationSpecial = FMinimapZoneGraphLaneNodeRef(OutLaneLocation.LaneHandle.Index, OutLaneLocation.DistanceAlongLane);
	}
}

int32 FMinimapZoneGraphAStarWrapper::GetNeighbourCountV2(const FMinimapZoneGraphAStarNode& Node) const
{
	const FZoneLaneData& Lane = ZoneGraph.Lanes[Node.NodeRef.LaneIndex];
	int32 Result = Lane.GetLinkCount();

	// Same lane with end location and end location is forward.
	if (Node.NodeRef.LaneIndex == EndLocation.LaneHandle.Index &&
		EndLocation.DistanceAlongLane > Node.NodeRef.LaneDistance)
	{
		// Check has outgoing, no outgoing we add one neighbour.
		if (GetLinkByType(ZoneGraph, EZoneLaneLinkType::Outgoing, Node.NodeRef.LaneIndex) == INDEX_NONE)
		{
			Result += 1;
		}
	}

	// If same zone with end location
	const auto& EndLane = ZoneGraph.Lanes[EndLocation.LaneHandle.Index];
	if (Lane.ZoneIndex == EndLane.ZoneIndex &&
		GetLinkByType(ZoneGraph, EZoneLaneLinkType::Outgoing, Node.NodeRef.LaneIndex) == INDEX_NONE &&
		Node.NodeRef.LaneIndex != StartLocation.LaneHandle.Index)
	{
		Result += 1;
	}

	return Result;
}

FMinimapZoneGraphAStarWrapper::FNodeRef FMinimapZoneGraphAStarWrapper::GetNeighbour(const FMinimapZoneGraphAStarNode& Node, const int32 NeighbourIndex) const
{
	const FZoneLaneData& Lane = ZoneGraph.Lanes[Node.NodeRef.LaneIndex];
	const int32 LinkIndex = Lane.LinksBegin + NeighbourIndex;
	const FZoneLaneLinkData& Link = ZoneGraph.LaneLinks[LinkIndex];
	
	// Special(End node)
	if (NeighbourIndex == Lane.GetLinkCount())
	{
		// Special same lane with end
		if (Node.NodeRef.LaneIndex == EndLocation.LaneHandle.Index &&
			EndLocation.DistanceAlongLane > Node.NodeRef.LaneDistance)
		{
			return FMinimapZoneGraphLaneNodeRef(EndLocation.LaneHandle.Index, EndLocation.DistanceAlongLane);
		}

		// Special same zone with end
		return EndLocationSpecial;
	}
	
	// Allow to pick left/right adjacent flags at start/end.
	if (Link.Type == EZoneLaneLinkType::Adjacent)
	{
		// Adjacent end
		if (Link.DestLaneIndex == EndLocation.LaneHandle.Index)
		{
			if (Node.NodeRef == EndLocationSpecial)
			{
				return FMinimapZoneGraphLaneNodeRef(EndLocation.LaneHandle.Index, EndLocation.DistanceAlongLane);
			}

			// Special fix.
			if (StartLocationSpecial.LaneDistance < EndLocation.DistanceAlongLane)
			{
				return StartLocationSpecial;
			}
			
			return EndLocationSpecial;
		}
		
		// Adjacent start
		if (Node.NodeRef.LaneIndex == StartLocation.LaneHandle.Index)
		{
			return StartLocationSpecial;
		}
	}
	
	// Normally allow only outgoing lanes
	if (Link.Type == EZoneLaneLinkType::Outgoing)
	{
		// Same lane end node.
		if (Node.NodeRef.LaneIndex == EndLocation.LaneHandle.Index &&
			EndLocation.DistanceAlongLane > Node.NodeRef.LaneDistance)
		{
			return FMinimapZoneGraphLaneNodeRef(EndLocation.LaneHandle.Index, EndLocation.DistanceAlongLane);
		}

		const auto& EndLane = ZoneGraph.Lanes[EndLocation.LaneHandle.Index];
		if (Lane.ZoneIndex == EndLane.ZoneIndex)
		{
			return EndLocationSpecial;
		}

		// Next zone shape node.
		return FMinimapZoneGraphLaneNodeRef(Link.DestLaneIndex, 0.0f);
	}

	return FMinimapZoneGraphLaneNodeRef();
}

bool FMinimapZoneGraphPathFilter::IsStart(const FMinimapZoneGraphAStarNode& Node) const
{
	return Node.NodeRef.LaneIndex == StartLocation.LaneHandle.Index;
}

bool FMinimapZoneGraphPathFilter::IsEnd(const FMinimapZoneGraphAStarNode& Node) const
{
	return Node.NodeRef.LaneIndex == EndLocation.LaneHandle.Index;
}

FVector::FReal FMinimapZoneGraphPathFilter::GetHeuristicScale() const
{
	return GetDefault<UMinimapSettings>()->HeuristicScale;
}

FVector::FReal FMinimapZoneGraphPathFilter::GetHeuristicCost(const FMinimapZoneGraphAStarNode& NeighbourNode,
	const FMinimapZoneGraphAStarNode& EndNode) const
{
	FZoneGraphLaneLocation NeighbourNodePosition;
	UE::ZoneGraph::Query::CalculateLocationAlongLane(ZoneStorage, NeighbourNode.NodeRef.LaneIndex, NeighbourNode.NodeRef.LaneDistance, NeighbourNodePosition);
	const FVector EndNodePosition = EndLocation.Position;

	// Traversal cost is important to get the shortest path.
	return FVector::Distance(NeighbourNodePosition.Position, EndNodePosition);
}

FVector::FReal FMinimapZoneGraphPathFilter::GetTraversalCost(const FMinimapZoneGraphAStarNode& CurNode,
	const FMinimapZoneGraphAStarNode& NeighbourNode) const
{
	const FZoneLaneData& CurLane = ZoneStorage.Lanes[CurNode.NodeRef.LaneIndex];
	const FZoneLaneData& NeighbourLane = ZoneStorage.Lanes[NeighbourNode.NodeRef.LaneIndex];

	if (CurLane.ZoneIndex != NeighbourLane.ZoneIndex)
	{
		// In different zones, we use neighbour start point as out distance end point.
		
		float CurLaneLength = -1.f;
		UE::ZoneGraph::Query::GetLaneLength(ZoneStorage, StartLocation.LaneHandle.Index, CurLaneLength);
		
		if (IsStart(CurNode))
		{
			return CurLaneLength - StartLocation.DistanceAlongLane;
		}
		
		return CurLaneLength;
	}
	else
	{
		// In same zones
		
		if (CurNode.NodeRef.LaneIndex == NeighbourNode.NodeRef.LaneIndex)
		{
			return FMath::Abs(NeighbourNode.NodeRef.LaneDistance - CurNode.NodeRef.LaneDistance);
		}
		else
		{
			// Cross lane.
			return CurLane.Width/2 + NeighbourLane.Width/2;
		}
	}
}

bool FMinimapZoneGraphPathFilter::IsTraversalAllowed(const FNodeRef StartNodeRef, const FNodeRef& Neighbour) const
{
	const FZoneGraphTagMask& LaneTagMask = ZoneStorage.Lanes[Neighbour.LaneIndex].Tags;
	return ZoneTagFilter.Pass(LaneTagMask);
}

bool FMinimapZoneGraphPathFilter::WantsPartialSolution() const
{
	return false;
}

bool FMinimapZoneGraphPathFilter::ShouldIncludeStartNodeInPath() const
{
	return true;
}

float UMinimapZoneGraphAStarLibrary::GetZoneWidthByLaneIndex(const UObject* WorldContext,
	const FZoneGraphStorage& ZoneStorage, int32 LaneIndex)
{
	if (!WorldContext) return false;
	const UWorld* World = WorldContext->GetWorld();
	if (!World) return false;
	const auto ZoneGraph = UWorld::GetSubsystem<UZoneGraphSubsystem>(World);
	auto StartZoneData = ZoneStorage.GetZoneDataFromLaneIndex(LaneIndex);
	
	float ZoneWidth = 0.0f;
	for (int32 LaneIdx = StartZoneData.LanesBegin; LaneIdx < StartZoneData.LanesEnd; LaneIdx++)
	{
		float OutWidth;
		ZoneGraph->GetLaneWidth(FZoneGraphLaneHandle(LaneIdx, ZoneStorage.DataHandle), OutWidth);
		ZoneWidth += OutWidth;
	}

	return ZoneWidth;
}

bool UMinimapZoneGraphAStarLibrary::GetZoneGraphPathBP(const UObject* WorldContext, FVector StartPosition,
	FVector DestPosition, FVector SearchExtent, FMinimapZoneGraphLanePath& Path)
{
	if (!WorldContext) return false;
	const UWorld* World = WorldContext->GetWorld();
	if (!World) return false;
	const auto ZoneGraph = UWorld::GetSubsystem<UZoneGraphSubsystem>(World);
    
    FZoneGraphLaneLocation StartOutLaneLocation;
    float StartOutDistanceSqr;
    ZoneGraph->FindNearestLane(FBox(StartPosition - SearchExtent, StartPosition + SearchExtent), FZoneGraphTagFilter(), StartOutLaneLocation, StartOutDistanceSqr);
	
    FZoneGraphLaneLocation StartNearestLaneLocation;
    float StartNearestDistanceSqr;
    ZoneGraph->FindNearestLocationOnLane(StartOutLaneLocation.LaneHandle, FBox(StartPosition - SearchExtent, StartPosition + SearchExtent), StartNearestLaneLocation, StartNearestDistanceSqr);

    FZoneGraphLaneLocation EndOutLaneLocation;
    float EndOutDistanceSqr;
    ZoneGraph->FindNearestLane(FBox(DestPosition - SearchExtent, DestPosition + SearchExtent), FZoneGraphTagFilter(), EndOutLaneLocation, EndOutDistanceSqr);
    
    FZoneGraphLaneLocation EndNearestLaneLocation;
    float EndNearestDistanceSqr;
    ZoneGraph->FindNearestLocationOnLane(EndOutLaneLocation.LaneHandle, FBox(DestPosition - SearchExtent, DestPosition + SearchExtent), EndNearestLaneLocation, EndNearestDistanceSqr);

    if (const AZoneGraphData* Data = ZoneGraph->GetZoneGraphData(StartOutLaneLocation.LaneHandle.DataHandle))
    {
        const FZoneGraphStorage& ZoneGraphStorage = Data->GetStorage();
    	FMinimapZoneGraphAStarNode StartNode(FMinimapZoneGraphLaneNodeRef(StartNearestLaneLocation.LaneHandle.Index, StartNearestLaneLocation.DistanceAlongLane));
    	FMinimapZoneGraphAStarNode EndNode(FMinimapZoneGraphLaneNodeRef(EndNearestLaneLocation.LaneHandle.Index, EndNearestLaneLocation.DistanceAlongLane));
    	FMinimapZoneGraphPathFilter PathFilter(ZoneGraphStorage, StartNearestLaneLocation, EndNearestLaneLocation, FZoneGraphTagFilter());
        FMinimapZoneGraphAStarWrapper Graph(ZoneGraphStorage, StartNearestLaneLocation, EndNearestLaneLocation);
        FMinimapZoneGraphAStar Pathfinder(Graph);
				
        // @todo: see if we can return directly a path of lane handles
        TArray<FMinimapZoneGraphAStarWrapper::FNodeRef> ResultPath;

        if (EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, PathFilter, ResultPath); Result == SearchSuccess)
        {
	        FZoneGraphLanePath LanePath;
        	//Store the resulting lanes
        	LanePath.Reset(ResultPath.Num());

        	LanePath.StartLaneLocation = StartNearestLaneLocation;
        	LanePath.EndLaneLocation = EndNearestLaneLocation;
        	for (FMinimapZoneGraphAStarWrapper::FNodeRef Node : ResultPath)
        	{
        		LanePath.Lanes.Add(FZoneGraphLaneHandle(Node.LaneIndex, ZoneGraphStorage.DataHandle));
        		Path.Distances.Add(Node.LaneDistance);
        	}
        	
        	Path.Path = LanePath;
        	return true;
    	}
    }
	
	return false;
}

bool UMinimapZoneGraphAStarLibrary::GetPathPoints(const UObject* WorldContext, const FMinimapZoneGraphLanePath& Path,
	TArray<FVector>& PathPoints)
{
	PathPoints.Empty();
	
	if (Path.Path.Lanes.Num() <= 0)
	{
		return false;
	}
	
	if (!WorldContext) return false;
	const UWorld* World = WorldContext->GetWorld();
	if (!World) return false;
	const auto ZoneGraph = UWorld::GetSubsystem<UZoneGraphSubsystem>(World);
	const auto DataHandle = Path.Path.Lanes[0].DataHandle;
	const FZoneGraphStorage& ZoneStorage = *ZoneGraph->GetZoneGraphStorage(DataHandle);

	PathPoints.Add(Path.Path.StartLaneLocation.Position);
	
	for (int i = 0; i < Path.Distances.Num(); ++i)
	{
		const auto LaneNode = Path.Distances[i];
		FZoneGraphLaneLocation NodeLocation;
		ZoneGraph->CalculateLocationAlongLane(Path.Path.Lanes[i], LaneNode, NodeLocation);
		
		// Stop when next is not valid.
		if (!Path.Distances.IsValidIndex(i + 1))
		{
			break;
		}
		
		const auto NextLaneNode = Path.Distances[i + 1];

		// If same lane.
		if (Path.Path.Lanes[i].Index == Path.Path.Lanes[i + 1].Index && LaneNode < NextLaneNode)
		{
			FZoneGraphLaneLocation NextNodeLocation;
			ZoneGraph->CalculateLocationAlongLane(Path.Path.Lanes[i], NextLaneNode, NextNodeLocation);
			PathPoints.Append(ConvertLaneToPoints(ZoneStorage, NodeLocation, NextNodeLocation));
			continue;
		}
		
		// If adjacent
		TArray<FZoneGraphLinkedLane> AdjacentLinkedLanes;
		ZoneGraph->GetLinkedLanes(Path.Path.Lanes[i], EZoneLaneLinkType::Adjacent, EZoneLaneLinkFlags::All, EZoneLaneLinkFlags::None, AdjacentLinkedLanes);
		if (AdjacentLinkedLanes.FindByPredicate([Path, i](const FZoneGraphLinkedLane& LinkedLane)
		{
			return LinkedLane.DestLane.Index == Path.Path.Lanes[i + 1].Index;
		}))
		{
			FZoneGraphLaneLocation NextNodeLocation;
			ZoneGraph->CalculateLocationAlongLane(Path.Path.Lanes[i + 1], NextLaneNode, NextNodeLocation);
			PathPoints.Add(NextNodeLocation.Position);
			continue;
		}
		
		// If outgoing.
		TArray<FZoneGraphLinkedLane> LinkedLanes;
		ZoneGraph->GetLinkedLanes(Path.Path.Lanes[i], EZoneLaneLinkType::Outgoing, EZoneLaneLinkFlags::All, EZoneLaneLinkFlags::None, LinkedLanes);
		if (LinkedLanes.FindByPredicate([Path, i](const FZoneGraphLinkedLane& LinkedLane)
		{
			return LinkedLane.DestLane.Index == Path.Path.Lanes[i + 1].Index;
		}))
		{
			PathPoints.Append(ConvertLaneToPoints(ZoneStorage, NodeLocation, FZoneGraphLaneLocation()));
			continue;
		}
	}
	
	return true;
}

TArray<FVector> UMinimapZoneGraphAStarLibrary::ConvertLaneToPoints(const FZoneGraphStorage& ZoneStorage,
	const FZoneGraphLaneLocation& InStartLocation, const FZoneGraphLaneLocation& InEndLocation)
{
	TArray<FVector> Result;
	
	FZoneGraphLaneLocation StartLocation = InStartLocation;
	FZoneGraphLaneLocation EndLocation = InEndLocation;

	// At least one location must be valid
	ensure(StartLocation.LaneHandle.IsValid() || EndLocation.LaneHandle.IsValid());
	const FZoneLaneData& Lane = StartLocation.IsValid() ? ZoneStorage.Lanes[StartLocation.LaneHandle.Index] : ZoneStorage.Lanes[EndLocation.LaneHandle.Index];

	// If both are valid, they must be on the same lane
	if (StartLocation.LaneHandle.IsValid() && EndLocation.LaneHandle.IsValid())
	{
		ensure(StartLocation.LaneHandle.Index == EndLocation.LaneHandle.Index);
	}
	
	const int32 PointsBegin = StartLocation.LaneHandle.IsValid() ? StartLocation.LaneSegment : Lane.PointsBegin;
	const int32 PointsEnd = EndLocation.LaneHandle.IsValid() ? EndLocation.LaneSegment + 1 : Lane.PointsEnd;

	for (int32 i = PointsBegin + 1; i < PointsEnd; i++)
	{
		const FVector Point = ZoneStorage.LanePoints[i];
		Result.Add(Point);
	}

	if (EndLocation.LaneHandle.IsValid())
	{
		// Last (partial segment)
		Result.Add(EndLocation.Position);
	}

	return Result;
}
