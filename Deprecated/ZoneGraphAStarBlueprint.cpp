// Fill out your copyright notice in the Description page of Project Settings.


#include "ZoneGraphAStarBlueprint.h"

#include "ZoneGraphQuery.h"

FZoneLaneData_BP UZoneGraphAStarBlueprint::GetLaneData(const FZoneGraphStorage_BP& Storage, int32 LaneIndex)
{
	if (Storage.Storage.Lanes.IsValidIndex(LaneIndex))
	{
		return FZoneLaneData_BP(Storage.Storage.Lanes[LaneIndex]);
	}

	return FZoneLaneData_BP();
}

int32 UZoneGraphAStarBlueprint::GetLaneLinkCount(const FZoneLaneData_BP& LaneData)
{
	return LaneData.LaneData.GetLinkCount();
}

int32 UZoneGraphAStarBlueprint::GetZoneIndex(const FZoneLaneData_BP& LaneData)
{
	return LaneData.LaneData.ZoneIndex;
}

FVector UZoneGraphAStarBlueprint::GetLaneLocationPosition(const FZoneGraphLaneLocation_BP& LaneLocation)
{
	return LaneLocation.LaneLocation.Position;
}

int32 UZoneGraphAStarBlueprint::GetLaneLocationLaneIndex(const FZoneGraphLaneLocation_BP& LaneLocation)
{
	return LaneLocation.LaneLocation.LaneHandle.Index;
}

float UZoneGraphAStarBlueprint::GetLaneLocationLaneDistance(const FZoneGraphLaneLocation_BP& LaneLocation)
{
	return LaneLocation.LaneLocation.DistanceAlongLane;
}

FZoneGraphLaneLocation_BP UZoneGraphAStarBlueprint::GetAdjacentLaneLocation(const FZoneGraphStorage_BP& Storage,
	const FZoneGraphLaneLocation_BP& CurrentLaneLocation, const int32 AdjacentLaneIndex)
{
	const auto LaneIndex = CurrentLaneLocation.LaneLocation.LaneHandle.Index;
	if (Storage.Storage.Lanes.IsValidIndex(LaneIndex))
	{
		const auto ZoneIndex = Storage.Storage.Lanes[LaneIndex].ZoneIndex;
		const float ZoneWidth = FZoneGraphCustomAStarWrapper::GetZoneWidth(Storage.Storage, ZoneIndex);
		// Rare case, same zone, get nearest point on NeighbourNode 
		const float SearchDistance = ZoneWidth; // Arbitrary search dist
		FZoneGraphLaneLocation LocationOnCurrentLane;
		float DistanceSqr = 0.f;
		FBox Bounds(CurrentLaneLocation.LaneLocation.Position, CurrentLaneLocation.LaneLocation.Position);
		Bounds = Bounds.ExpandBy(SearchDistance);
		UE::ZoneGraph::Query::FindNearestLocationOnLane(
			Storage.Storage,
			FZoneGraphLaneHandle(AdjacentLaneIndex, Storage.Storage.DataHandle),
			Bounds,
			LocationOnCurrentLane,
			DistanceSqr
		);

		return FZoneGraphLaneLocation_BP(LocationOnCurrentLane);
	}
	
	return FZoneGraphLaneLocation_BP();
}

int32 UZoneGraphAStarBlueprint::GetLaneLinkBeginIndex(const FZoneLaneData_BP& LaneData)
{
	return LaneData.LaneData.LinksBegin;
}

FZoneLaneLinkData_BP UZoneGraphAStarBlueprint::GetLaneLinkData(const FZoneGraphStorage_BP& Storage,
	const int32& LinkIndex)
{
	if (Storage.Storage.LaneLinks.IsValidIndex(LinkIndex))
	{
		return FZoneLaneLinkData_BP(Storage.Storage.LaneLinks[LinkIndex]);
	}

	return FZoneLaneLinkData_BP();
}

int32 UZoneGraphAStarBlueprint::GetLaneLinkDestLaneIndex(const FZoneLaneLinkData_BP& Link)
{
	return Link.LaneLinkData.DestLaneIndex;
}

EZoneLaneLinkType UZoneGraphAStarBlueprint::GetLaneLinkType(const FZoneLaneLinkData_BP& Link)
{
	return Link.LaneLinkData.Type;
}

FZoneGraphLaneLocation_BP UZoneGraphAStarBlueprint::GetLocationAlongDistance(const FZoneGraphStorage_BP& Storage,
	const int32& LaneIndex, const float& Distance)
{
	FZoneGraphLaneLocation OutLocation;
	UE::ZoneGraph::Query::CalculateLocationAlongLane(Storage.Storage, LaneIndex, Distance, OutLocation);
	return FZoneGraphLaneLocation_BP(OutLocation);
}

float UZoneGraphAStarBlueprint::GetLaneLength(const FZoneGraphStorage_BP& Storage, const int32& LaneIndex)
{
	float CurLaneLength;
	UE::ZoneGraph::Query::GetLaneLength(Storage.Storage, LaneIndex, CurLaneLength);
	return CurLaneLength;
}

float UZoneGraphAStarBlueprint::GetLaneWidth(const FZoneLaneData_BP& LaneData)
{
	return LaneData.LaneData.Width;
}
