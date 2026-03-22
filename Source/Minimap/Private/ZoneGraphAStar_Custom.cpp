#include "ZoneGraphAStar_Custom.h"

#include "MinimapSettings.h"
#include "ZoneGraphTypes.h"
#include "ZoneGraphQuery.h"

float FZoneGraphCustomAStarWrapper::GetZoneWidth(const FZoneGraphStorage& ZoneGraph, int32 LaneIndex)
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

int32 FZoneGraphCustomAStarWrapper::GetLinkByType(const FZoneGraphStorage& ZoneGraph, EZoneLaneLinkType LinkType,
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

FZoneGraphLaneLocation FZoneGraphCustomAStarWrapper::QueryLaneLocationByLocation(const FVector& CheckLocation, int32 TargetLaneIndex) const
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

void FZoneGraphCustomAStarWrapper::SetEndSpecial()
{
	// Get adjacent lane
	const auto AdjacentLinkIndex = GetLinkByType(ZoneGraph, EZoneLaneLinkType::Adjacent, EndLocation.LaneHandle.Index);
	if (AdjacentLinkIndex >= 0)
	{
		const auto AdjacentLaneIndex = ZoneGraph.LaneLinks[AdjacentLinkIndex].DestLaneIndex;
		const auto OutLaneLocation = QueryLaneLocationByLocation(EndLocation.Position, AdjacentLaneIndex);
		EndLocationSpecial = FZoneGraphLaneNodeRef(OutLaneLocation.LaneHandle.Index, OutLaneLocation.DistanceAlongLane);
	}
}

void FZoneGraphCustomAStarWrapper::SetStartSpecial()
{
	// Get adjacent lane
	const auto AdjacentLinkIndex = GetLinkByType(ZoneGraph, EZoneLaneLinkType::Adjacent, StartLocation.LaneHandle.Index);
	if (AdjacentLinkIndex >= 0)
	{
		const auto AdjacentLaneIndex = ZoneGraph.LaneLinks[AdjacentLinkIndex].DestLaneIndex;
		const auto OutLaneLocation = QueryLaneLocationByLocation(StartLocation.Position, AdjacentLaneIndex);
		StartLocationSpecial = FZoneGraphLaneNodeRef(OutLaneLocation.LaneHandle.Index, OutLaneLocation.DistanceAlongLane);
	}
}

int32 FZoneGraphCustomAStarWrapper::GetNeighbourCountV2(const FZoneGraphCustomAStarNode& Node) const
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

FZoneGraphCustomAStarWrapper::FNodeRef FZoneGraphCustomAStarWrapper::GetNeighbour(const FZoneGraphCustomAStarNode& Node, const int32 NeighbourIndex) const
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
			return FZoneGraphLaneNodeRef(EndLocation.LaneHandle.Index, EndLocation.DistanceAlongLane);
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
				return FZoneGraphLaneNodeRef(EndLocation.LaneHandle.Index, EndLocation.DistanceAlongLane);
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
			return FZoneGraphLaneNodeRef(EndLocation.LaneHandle.Index, EndLocation.DistanceAlongLane);
		}

		const auto& EndLane = ZoneGraph.Lanes[EndLocation.LaneHandle.Index];
		if (Lane.ZoneIndex == EndLane.ZoneIndex)
		{
			return EndLocationSpecial;
		}

		// Next zone shape node.
		return FZoneGraphLaneNodeRef(Link.DestLaneIndex, 0.0f);
	}

	return FZoneGraphLaneNodeRef();
}

bool FZoneGraphCustomPathFilter::IsStart(const FZoneGraphCustomAStarNode& Node) const
{
	return Node.NodeRef.LaneIndex == StartLocation.LaneHandle.Index;
}

bool FZoneGraphCustomPathFilter::IsEnd(const FZoneGraphCustomAStarNode& Node) const
{
	return Node.NodeRef.LaneIndex == EndLocation.LaneHandle.Index;
}

FVector::FReal FZoneGraphCustomPathFilter::GetHeuristicScale() const
{
	return GetDefault<UMinimapSettings>()->HeuristicScale;
}

FVector::FReal FZoneGraphCustomPathFilter::GetHeuristicCost(const FZoneGraphCustomAStarNode& NeighbourNode,
	const FZoneGraphCustomAStarNode& EndNode) const
{
	FZoneGraphLaneLocation NeighbourNodePosition;
	UE::ZoneGraph::Query::CalculateLocationAlongLane(ZoneStorage, NeighbourNode.NodeRef.LaneIndex, NeighbourNode.NodeRef.LaneDistance, NeighbourNodePosition);
	const FVector EndNodePosition = EndLocation.Position;

	// Traversal cost is important to get shortest path.
	return FVector::Distance(NeighbourNodePosition.Position, EndNodePosition);
}

FVector::FReal FZoneGraphCustomPathFilter::GetTraversalCost(const FZoneGraphCustomAStarNode& CurNode,
	const FZoneGraphCustomAStarNode& NeighbourNode) const
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

bool FZoneGraphCustomPathFilter::IsTraversalAllowed(const FNodeRef StartNodeRef, const FNodeRef& Neighbour) const
{
	const FZoneGraphTagMask& LaneTagMask = ZoneStorage.Lanes[Neighbour.LaneIndex].Tags;
	return ZoneTagFilter.Pass(LaneTagMask);
}

bool FZoneGraphCustomPathFilter::WantsPartialSolution() const
{
	return false;
}

bool FZoneGraphCustomPathFilter::ShouldIncludeStartNodeInPath() const
{
	return true;
}
