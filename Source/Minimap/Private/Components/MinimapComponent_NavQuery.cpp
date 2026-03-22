
#include "Components/MinimapComponent_Player.h"

#include "ZoneGraphAStar_Custom.h"
#include "ZoneGraphQuery.h"
#include "ZoneGraphSubsystem.h"
#include "Actors/MapPinActor.h"

bool UMinimapComponent_Player::ShouldShowNavPath() const
{
	return bShouldUpdateNavQuery && bPathPointsValid;
}

void UMinimapComponent_Player::UpdateNavPath(const float& DeltaTime)
{
	// Update end position.
	if (TempPin)
	{
		NavQueryEndPosition = TempPin->GetActorLocation();
		bShouldUpdateNavQuery = true;
	}
	else
	{
		bShouldUpdateNavQuery = false;
	}
	
	// Should update nav query.
	if (bShouldUpdateNavQuery)
	{
		// Update nav query start position using local player actor location.
		if (bAutoUpdateStartLocation)
		{
			NavQueryStartPosition = GetOwner()->GetActorLocation();
		}
		// Update nav query period. Using task to do async task update.
		NavQueryTime += DeltaTime;
		if (NavQueryTime >= NavQueryPeriod)
		{
			NavQueryTime = 0.0f;
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
			{
				FZoneGraphLanePath_BP OutPath;
				bPathPointsValid = GetZoneGraphPathBP(this, NavQueryStartPosition, NavQueryEndPosition, NavQueryExtend, OutPath);
				GetPathPoints(this, OutPath, NavQueryOutPathPoints);
			});
		}
	}
}

float UMinimapComponent_Player::GetZoneWidthByLaneIndex(const UObject* WorldContext, const FZoneGraphStorage& ZoneStorage, int32 LaneIndex)
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

bool UMinimapComponent_Player::GetZoneGraphPathBP(const UObject* WorldContext, FVector StartPosition, FVector DestPosition,
	FVector SearchExtent, FZoneGraphLanePath_BP& Path)
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
    	FZoneGraphCustomAStarNode StartNode(FZoneGraphLaneNodeRef(StartNearestLaneLocation.LaneHandle.Index, StartNearestLaneLocation.DistanceAlongLane));
    	FZoneGraphCustomAStarNode EndNode(FZoneGraphLaneNodeRef(EndNearestLaneLocation.LaneHandle.Index, EndNearestLaneLocation.DistanceAlongLane));
    	FZoneGraphCustomPathFilter PathFilter(ZoneGraphStorage, StartNearestLaneLocation, EndNearestLaneLocation, FZoneGraphTagFilter());
        FZoneGraphCustomAStarWrapper Graph(ZoneGraphStorage, StartNearestLaneLocation, EndNearestLaneLocation);
        FZoneGraphCustomAStar Pathfinder(Graph);
				
        // @todo: see if we can return directly a path of lane handles
        TArray<FZoneGraphCustomAStarWrapper::FNodeRef> ResultPath;

        if (EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, PathFilter, ResultPath); Result == SearchSuccess)
        {
	        FZoneGraphLanePath LanePath;
        	//Store the resulting lanes
        	LanePath.Reset(ResultPath.Num());

        	LanePath.StartLaneLocation = StartNearestLaneLocation;
        	LanePath.EndLaneLocation = EndNearestLaneLocation;
        	for (FZoneGraphCustomAStarWrapper::FNodeRef Node : ResultPath)
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

bool UMinimapComponent_Player::GetPathPoints(const UObject* WorldContext, const FZoneGraphLanePath_BP& Path, TArray<FVector>& PathPoints)
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

TArray<FVector> UMinimapComponent_Player::ConvertLaneToPoints(const FZoneGraphStorage& ZoneStorage,
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