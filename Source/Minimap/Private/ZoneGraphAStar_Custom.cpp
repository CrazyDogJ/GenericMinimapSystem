#include "ZoneGraphAStar_Custom.h"

#include "MinimapSettings.h"
#include "ZoneGraphTypes.h"
#include "ZoneGraphQuery.h"

float FZoneGraphCustomAStarWrapper::GetZoneWidth(const FZoneGraphStorage& ZoneGraph, int32 ZoneIndex)
{
	if (ZoneIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	
	const auto ZoneData = ZoneGraph.Zones[ZoneIndex];
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

int32 FZoneGraphCustomAStarWrapper::GetOutgoingLink(const FZoneGraphStorage& ZoneGraph, int32 LaneIndex)
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
		if (LaneLink.Type == EZoneLaneLinkType::Outgoing)
		{
			return ItrLinkIndex;
		}
	}

	return INDEX_NONE;
}

int32 FZoneGraphCustomAStarWrapper::GetNeighbourCountV2(const FZoneGraphCustomAStarNode& Node) const
{
	const FZoneLaneData& Lane = ZoneGraph.Lanes[Node.NodeRef.LaneIndex];
	int32 Result = Lane.GetLinkCount();
	const auto EndZoneIndex = ZoneGraph.Lanes[EndLocation.LaneHandle.Index].ZoneIndex;
	const auto CurrentZoneIndex = ZoneGraph.Lanes[Node.NodeRef.LaneIndex].ZoneIndex;

	// Special same zone
	if (CurrentZoneIndex == EndZoneIndex)
	{
		// Special same lane
		float EndLaneDistance;
		if (Node.NodeRef.LaneIndex == EndLocation.LaneHandle.Index)
		{
			EndLaneDistance = EndLocation.DistanceAlongLane;
		}
		// Special other lane
		else
		{
			const float ZoneWidth = GetZoneWidth(ZoneGraph, CurrentZoneIndex);
			// Rare case, same zone, get nearest point on NeighbourNode 
			const float SearchDistance = ZoneWidth; // Arbitrary search dist
			FZoneGraphLaneLocation LocationOnCurrentLane;
			float DistanceSqr = 0.f;
			FBox Bounds(EndLocation.Position, EndLocation.Position);
			Bounds = Bounds.ExpandBy(SearchDistance);
			UE::ZoneGraph::Query::FindNearestLocationOnLane(
				ZoneGraph,
				FZoneGraphLaneHandle(Node.NodeRef.LaneIndex, ZoneGraph.DataHandle),
				Bounds,
				LocationOnCurrentLane,
				DistanceSqr
			);
			
			EndLaneDistance = LocationOnCurrentLane.DistanceAlongLane;
			if (!EndLocationSpecial.IsValid())
			{
				EndLocationSpecial = FNodeRef(Node.NodeRef.LaneIndex, EndLaneDistance);
			}
		}
		// Check should add one more neighbour count.
		const auto CurrentLaneDistance = Node.NodeRef.LaneDistance;
		if (EndLaneDistance > CurrentLaneDistance)
		{
			const auto OutgoingLinkIndex = GetOutgoingLink(ZoneGraph, Node.NodeRef.LaneIndex);
			if (OutgoingLinkIndex == INDEX_NONE)
			{
				Result += 1;
			}
		}
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
		return FZoneGraphLaneNodeRef(Node.NodeRef.LaneIndex, EndLocation.DistanceAlongLane);
	}
	
	// Allow to pick left/right adjacent flags at start/end.
	if (Link.Type == EZoneLaneLinkType::Adjacent)
	{
		// End change lane.
		if (Node.NodeRef == EndLocationSpecial)
		{
			return FZoneGraphLaneNodeRef(Link.DestLaneIndex, EndLocation.DistanceAlongLane);
		}
		
		// Start change lane.
		const float ZoneWidth = GetZoneWidth(ZoneGraph, Lane.ZoneIndex);
		// Rare case, same zone, get nearest point on NeighbourNode 
		const float SearchDistance = ZoneWidth; // Arbitrary search dist
		FZoneGraphLaneLocation LocationOnCurrentLane;
		float DistanceSqr = 0.f;
		FBox Bounds(StartLocation.Position, StartLocation.Position);
		Bounds = Bounds.ExpandBy(SearchDistance);
		UE::ZoneGraph::Query::FindNearestLocationOnLane(
			ZoneGraph,
			FZoneGraphLaneHandle(Link.DestLaneIndex, ZoneGraph.DataHandle),
			Bounds,
			LocationOnCurrentLane,
			DistanceSqr
		);

		return FZoneGraphLaneNodeRef(Link.DestLaneIndex, LocationOnCurrentLane.DistanceAlongLane);
	}
	
	// Normally allow only outgoing lanes
	if (Link.Type == EZoneLaneLinkType::Outgoing)
	{
		// Same lane end node.
		if (Node.NodeRef.LaneIndex == EndLocation.LaneHandle.Index)
		{
			return FZoneGraphLaneNodeRef(EndLocation.LaneHandle.Index, EndLocation.DistanceAlongLane);
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
	const bool bDifferentZones = (CurLane.ZoneIndex != NeighbourLane.ZoneIndex);
	
	if (bDifferentZones)
	{
		// In different zones, we use neighbour start point as out distance end point.
		
		float CurLaneLength = -1.f;
		UE::ZoneGraph::Query::GetLaneLength(ZoneStorage, StartLocation.LaneHandle.Index, CurLaneLength);
		
		if (IsStart(CurNode))
		{
			return CurLaneLength - StartLocation.DistanceAlongLane;
		}

		// Same zone as start node.
		if (CurLane.ZoneIndex == ZoneStorage.Lanes[StartLocation.LaneHandle.Index].ZoneIndex)
		{
			return CurLaneLength - CurNode.NodeRef.LaneDistance;
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
