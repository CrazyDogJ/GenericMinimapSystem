#include "ZoneGraphAStar_Custom.h"
#include "ZoneGraphTypes.h"
#include "ZoneGraphQuery.h"

int32 FZoneGraphCustomAStarWrapper::GetNeighbourCountV2(const FZoneGraphCustomAStarNode& Node) const
{
	// @todo: Make a special case to allow picking side neighbours when starting in an intersection.
	//  Use ZoneData.Tags to identify where this should apply and return the correct neighbour count here.

	// Return max link count, we'll return invalid link for the ones that we do not want to traverse.
	const FZoneLaneData& Lane = ZoneGraph.Lanes[Node.NodeRef];
	return Lane.GetLinkCount();
}

FZoneGraphCustomAStarWrapper::FNodeRef FZoneGraphCustomAStarWrapper::GetNeighbour(const FZoneGraphCustomAStarNode& Node, const int32 NeighbourIndex) const
{
	// @todo: Make a special case to allow picking side neighbours when starting in an intersections (use ZoneData.Tags to identify where this should apply).

	const FZoneLaneData& Lane = ZoneGraph.Lanes[Node.NodeRef];
	check(NeighbourIndex < Lane.GetLinkCount());

	const int32 LinkIndex = Lane.LinksBegin + NeighbourIndex;
	const FZoneLaneLinkData& Link = ZoneGraph.LaneLinks[Lane.LinksBegin + NeighbourIndex];

	// Allow to pick left/right adjacent flags at start/end.
	if (Link.Type == EZoneLaneLinkType::Adjacent && Link.HasFlags(EZoneLaneLinkFlags::Left | EZoneLaneLinkFlags::Right))
	{
		return Link.DestLaneIndex;
	}
	
	// Normally allow only outgoing lanes
	if (Link.Type == EZoneLaneLinkType::Outgoing)
	{
		return Link.DestLaneIndex;
	}

	return INDEX_NONE;
}

bool FZoneGraphCustomAStarNode::IsStartOrIsEnd() const
{
	return Position != FVector(TNumericLimits<FVector::FReal>::Max());
}

bool FZoneGraphCustomPathFilter::IsStart(const FZoneGraphCustomAStarNode& Node) const
{
	return Node.NodeRef == StartLocation.LaneHandle.Index;
}

bool FZoneGraphCustomPathFilter::IsEnd(const FZoneGraphCustomAStarNode& Node) const
{
	return Node.NodeRef == EndLocation.LaneHandle.Index;
}

FVector::FReal FZoneGraphCustomPathFilter::GetHeuristicScale() const
{
	return 1.;
}

FVector::FReal FZoneGraphCustomPathFilter::GetHeuristicCost(const FZoneGraphCustomAStarNode& NeighbourNode,
	const FZoneGraphCustomAStarNode& EndNode) const
{
	static const FVector InvalidPosition(TNumericLimits<FVector::FReal>::Max());
		
	// Except for start to end nodes, compute heurisitic from the last point of lanes
	if (NeighbourNode.Position == InvalidPosition)
	{
		// Neighbor nodes dont have position
		const uint32 LaneIndex = NeighbourNode.NodeRef;
		const FZoneLaneData& LaneData = ZoneStorage.Lanes[LaneIndex];
		const FVector LaneEndPoint = ZoneStorage.LanePoints[LaneData.PointsEnd - 1];

		ensure(EndNode.Position != InvalidPosition);
		return FVector::Distance(LaneEndPoint, EndNode.Position);
	}
	else
	{
		ensure(NeighbourNode.Position != InvalidPosition && EndNode.Position != InvalidPosition);
		return FVector::Distance(NeighbourNode.Position, EndNode.Position);
	}
}

FVector::FReal FZoneGraphCustomPathFilter::GetTraversalCost(const FZoneGraphCustomAStarNode& CurNode,
	const FZoneGraphCustomAStarNode& NeighbourNode) const
{
	const FZoneLaneData& CurLane = ZoneStorage.Lanes[CurNode.NodeRef];
	const FZoneLaneData& NeighbourLane = ZoneStorage.Lanes[NeighbourNode.NodeRef];
	const bool bDifferentZones = (CurLane.ZoneIndex != NeighbourLane.ZoneIndex);

	if (bDifferentZones)
	{
		// Nodes are in different zones

		float TravelLength = 0.f;
		if (IsStart(CurNode))
		{
			float CurLaneLength = -1.f;
			UE::ZoneGraph::Query::GetLaneLength(ZoneStorage, StartLocation.LaneHandle.Index, CurLaneLength);
			TravelLength += (CurLaneLength - StartLocation.DistanceAlongLane);
		}

		if (IsEnd(NeighbourNode))
		{
			TravelLength += EndLocation.DistanceAlongLane;
		}
		else
		{
			// Most common case, take the lane length of NeighbourNode.
			// The traversal cost is the cost of reaching the end of the neighbour node.
			float LaneLength = -1.f;
			UE::ZoneGraph::Query::GetLaneLength(ZoneStorage, NeighbourNode.NodeRef, LaneLength);
			TravelLength += LaneLength;
		}
		return TravelLength;
	}
	else
	{
		// Special case, nodes are in the same zone
		// Rare case, same zone, get nearest point on NeighbourNode 
		const float SearchDistance = 5.f * CurLane.Width; // Arbitrary search dist
		FZoneGraphLaneLocation LocationOnNeighbourNode;
		float DistanceSqr = 0.f;
		FBox Bounds(CurNode.Position, CurNode.Position);
		Bounds = Bounds.ExpandBy(SearchDistance);
		UE::ZoneGraph::Query::FindNearestLocationOnLane(
			ZoneStorage,
			FZoneGraphLaneHandle(NeighbourNode.NodeRef, ZoneStorage.DataHandle),
			Bounds,
			LocationOnNeighbourNode,
			DistanceSqr
		);

		if (IsEnd(NeighbourNode))
		{
			return FMath::Abs(EndLocation.DistanceAlongLane - LocationOnNeighbourNode.DistanceAlongLane);
		}
		else
		{
			// Get distance from closest point on NeighbourNode to the end
			float Length = -1.f;
			UE::ZoneGraph::Query::GetLaneLength(ZoneStorage, NeighbourNode.NodeRef, Length);
			return Length - LocationOnNeighbourNode.DistanceAlongLane;
		}
	}
}

bool FZoneGraphCustomPathFilter::IsTraversalAllowed(const FNodeRef StartNodeRef, const FNodeRef& Neighbour) const
{
	const FZoneGraphTagMask& LaneTagMask = ZoneStorage.Lanes[Neighbour].Tags;
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
