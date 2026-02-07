// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapSubsystem.h"

#include "Components/MinimapComponent.h"
#include "MinimapSettings.h"
#include "ZoneGraphAStar_Custom.h"
#include "ZoneGraphQuery.h"
#include "ZoneGraphSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UMinimapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

	NavQueryPeriod = GetDefault<UMinimapSettings>()->NavQueryPeriod;
}

void UMinimapSubsystem::Deinitialize()
{
    MinimapComponentRegistry.Empty();
    StaticMapPins.Empty();
    ShownMapPinsGuids.Empty();

    Super::Deinitialize();
}

bool UMinimapSubsystem::ShouldShowNavPath() const
{
	return bShouldUpdateNavQuery && bPathPointsValid;
}

TArray<UMinimapComponent*> UMinimapSubsystem::GetRegisteredComponents() const
{
    TArray<UMinimapComponent*> Result;
    for (const TObjectPtr<UMinimapComponent>& comp : MinimapComponentRegistry)
    {
        if (comp)
        {
            Result.Add(comp.Get());
        }
    }

    return Result;
}

TArray<FStaticMapPin> UMinimapSubsystem::GetRegisteredStaticMapPins() const
{
    return StaticMapPins;
}

FStaticMapPin UMinimapSubsystem::GetShownMinimapPin(FGuid Guid, bool& Success) const
{
    if (!Guid.IsValid())
    {
    	Success = false;
        return FStaticMapPin();
    }

    auto NewStaticMapPins = StaticMapPins;
    
    for (auto RegisteredComp : MinimapComponentRegistry)
    {
        if (RegisteredComp->MinimapGuid.IsValid() && RegisteredComp->ShouldVisible())
        {
            auto StaticPtr = StaticMapPins.IndexOfByPredicate([&](const FStaticMapPin& Pin)
            {
               return Pin.IdentifyGuid == RegisteredComp->MinimapGuid; 
            });
            
            if (StaticPtr >= 0)
            {
                NewStaticMapPins[StaticPtr] = RegisteredComp->GetCurrentStaticMapPin();
            }
            else if (RegisteredComp->bIsIndividual)
            {
                NewStaticMapPins.Add(RegisteredComp->GetCurrentStaticMapPin());
            }
        }
    }
    
    auto StaticPtr = NewStaticMapPins.FindByPredicate([&](const FStaticMapPin& Pin)
    {
        return Pin.IdentifyGuid == Guid; 
    });

    if (StaticPtr)
    {
    	Success = true;
        return *StaticPtr;
    }

	Success = false;
    return FStaticMapPin();
}

FGuid UMinimapSubsystem::AddStaticLocationPin(FStaticMapPin InPin)
{
    if (!InPin.IdentifyGuid.IsValid())
    {
        InPin.IdentifyGuid = FGuid::NewGuid();
    }
    if (InPin.bAlwaysOnMinimap)
    {
        AddMinimapPin(InPin.IdentifyGuid);
    }
    StaticMapPins.Add(InPin);
    OnStaticRegistered.Broadcast(InPin);
    return InPin.IdentifyGuid;
}

void UMinimapSubsystem::RemoveStaticLocationPin(FGuid MapPinGuid)
{
    FStaticMapPin Pin;
    Pin.IdentifyGuid = MapPinGuid;
    auto Index = StaticMapPins.Find(Pin);
    if (Index >= 0)
    {
        auto Result = StaticMapPins[Index];
        OnStaticUnregistered.Broadcast(Result);
        StaticMapPins.Remove(Pin);
    }
    RemoveMinimapPin(MapPinGuid);
}

UMinimapMapData* UMinimapSubsystem::GetCurrentMinimapMapData()
{
    if (CurrentMinimapMapData)
    {
        if (CurrentMinimapMapData->LevelName == UGameplayStatics::GetCurrentLevelName(GetWorld()))
        {
            return CurrentMinimapMapData;
        }
    }
    
    UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>();
    if (auto Value = Settings->MapsInfos.Find(UGameplayStatics::GetCurrentLevelName(GetWorld())))
    {
        CurrentMinimapMapData = Value->LoadSynchronous();
        for (auto HotPoint : CurrentMinimapMapData->HotPointInfos)
        {
            FStaticMapPin NewPin;
        	NewPin.Location = HotPoint.Location;
        	NewPin.Yaw = 0.0f;
        	NewPin.MapPinBrush = HotPoint.MapPinBrush;
            NewPin.IdentifyGuid = HotPoint.IdentifyGuid;
            NewPin.PinName = HotPoint.PinName;
            NewPin.PinDescription = HotPoint.PinDescription;
        	NewPin.CustomMinimapWidgetClass = HotPoint.CustomMinimapWidgetClass;
        	NewPin.CustomMainmapWidgetClass = HotPoint.CustomMainmapWidgetClass;
        	NewPin.CustomDatas = HotPoint.CustomDatas;
        	
            StaticMapPins.AddUnique(NewPin);
        }
        return CurrentMinimapMapData;
    }
    
    return nullptr;
}

FHotPointInfo UMinimapSubsystem::GetHotPointInfoFromGuid(FGuid Guid)
{
    if (CurrentMinimapMapData && Guid.IsValid())
    {
        auto Ptr = CurrentMinimapMapData->HotPointInfos.FindByPredicate([&] (const FHotPointInfo& HotPoint)
        {
           return HotPoint.IdentifyGuid == Guid;
        });

        if (Ptr)
        {
            return *Ptr;
        }
    }

    return FHotPointInfo();
}

void UMinimapSubsystem::SetupLocalPlayer(AActor* LocalPlayerPawn)
{
    CurrentLocalPlayerActor = LocalPlayerPawn;
}

void UMinimapSubsystem::SetMinimapRadius(float Radius)
{
    MinimapRadius = Radius;
}

float UMinimapSubsystem::GetZoneWidthByLaneIndex(const FZoneGraphStorage& ZoneStorage, int32 LaneIndex) const
{
	const auto ZoneGraph = UWorld::GetSubsystem<UZoneGraphSubsystem>(GetWorld());
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

int UMinimapSubsystem::GetPathLaneCount(const FZoneGraphLanePath_BP& Path)
{
	return Path.Path.Lanes.Num();
}

bool UMinimapSubsystem::GetZoneGraphPathBP(FVector StartPosition, FVector DestPosition, FVector SearchExtent,
	FZoneGraphLanePath_BP& Path)
{
	const auto ZoneGraph = UWorld::GetSubsystem<UZoneGraphSubsystem>(GetWorld());
    
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
        FZoneGraphCustomAStarWrapper Graph(ZoneGraphStorage);
        FZoneGraphCustomAStar Pathfinder(Graph);
        // @todo: pass FZoneGraphLaneLocation directly to the constructor
        FZoneGraphCustomAStarNode StartNode(StartNearestLaneLocation.LaneHandle.Index, StartNearestLaneLocation.Position);
        FZoneGraphCustomAStarNode EndNode(EndNearestLaneLocation.LaneHandle.Index, EndNearestLaneLocation.Position);
        FZoneGraphCustomPathFilter PathFilter(ZoneGraphStorage, StartNearestLaneLocation, EndNearestLaneLocation, FZoneGraphTagFilter());
				
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
    			LanePath.Add(FZoneGraphLaneHandle(Node, StartNearestLaneLocation.LaneHandle.DataHandle));
    		}
        	
        	Path.Path = LanePath;
        	return true;
    	}
    }
	
	return false;
}

bool UMinimapSubsystem::GetPathPoints(const FZoneGraphLanePath_BP& Path, TArray<FVector>& PathPoints)
{
	PathPoints.Empty();
	
	if (Path.Path.Lanes.Num() <= 0)
	{
		return false;
	}
	
	const auto ZoneGraph = UWorld::GetSubsystem<UZoneGraphSubsystem>(GetWorld());
	const auto DataHandle = Path.Path.Lanes[0].DataHandle;
	const FZoneGraphStorage& ZoneStorage = *ZoneGraph->GetZoneGraphStorage(DataHandle);
	
	// 同一车道
	if (Path.Path.Lanes.Num() == 1)
	{
		// 目的地在正前方
		if (Path.Path.StartLaneLocation.DistanceAlongLane <= Path.Path.EndLaneLocation.DistanceAlongLane)
		{
			// 直接画
			PathPoints.Add(Path.Path.StartLaneLocation.Position);
			PathPoints.Append(ConvertLaneToPoints(ZoneStorage, Path.Path.StartLaneLocation, Path.Path.EndLaneLocation));
			return true;
		}
		// 目的地在正后方
		else
		{
			// 最近的逆向车道
			auto StartZoneData = ZoneStorage.GetZoneDataFromLaneIndex(Path.Path.StartLaneLocation.LaneHandle.Index);
			// 获取起点区域宽度（可以是函数）
			float ZoneWidth = GetZoneWidthByLaneIndex(ZoneStorage, Path.Path.StartLaneLocation.LaneHandle.Index);
			// 遍历并判断是否为对向车道且距离最近
			float BestDist = MAX_FLT;
			FZoneGraphLaneLocation BestOppoLaneLocation = FZoneGraphLaneLocation();
			for (int32 LaneIdx = StartZoneData.LanesBegin; LaneIdx < StartZoneData.LanesEnd; LaneIdx++)
			{
				// 不遍历自己
				if (LaneIdx == Path.Path.StartLaneLocation.LaneHandle.Index) continue;
				
				FZoneGraphLaneLocation OutLaneLocation;
				float OutLaneDistSqr;
				ZoneGraph->FindNearestLocationOnLane(FZoneGraphLaneHandle(LaneIdx, DataHandle), Path.Path.StartLaneLocation.Position, ZoneWidth, OutLaneLocation, OutLaneDistSqr);
				const float CurrentDistance = FVector::Distance(OutLaneLocation.Position, Path.Path.StartLaneLocation.Position);
				const bool bIsOppositeLane = FVector::DotProduct(OutLaneLocation.Direction, Path.Path.StartLaneLocation.Direction) < -0.9f;
				if (bIsOppositeLane && CurrentDistance < BestDist)
				{
					BestDist = CurrentDistance;
					BestOppoLaneLocation = OutLaneLocation;
				}
			}
			
			// 现在道路有最近的逆向车道
			if (BestOppoLaneLocation.IsValid())
			{
				// 起始点
				PathPoints.Add(Path.Path.StartLaneLocation.Position);
				PathPoints.Add(BestOppoLaneLocation.Position);

				FBox EndBounds(Path.Path.EndLaneLocation.Position, Path.Path.EndLaneLocation.Position);
				EndBounds = EndBounds.ExpandBy(ZoneWidth);
				FZoneGraphLaneLocation OutEndLocation;
				float EndDistanceSqr = 0.f;
				ZoneGraph->FindNearestLocationOnLane(BestOppoLaneLocation.LaneHandle, EndBounds, OutEndLocation, EndDistanceSqr);

				// 反向车道路径点
				PathPoints.Append(ConvertLaneToPoints(ZoneStorage, BestOppoLaneLocation, OutEndLocation));
				// 终点
				PathPoints.Add(Path.Path.EndLaneLocation.Position);
				return true;
			}
			// 道路没有逆向车道
			else
			{
				// 直接画
				auto out = ConvertLaneToPoints(ZoneStorage, Path.Path.EndLaneLocation, Path.Path.StartLaneLocation);
				Algo::Reverse(out);
				PathPoints.Append(out);
				return true;
			}
		}
	}

	auto LanePath = Path.Path;
	const int32 LanePathCount = LanePath.Lanes.Num();
	const int32 LastLanePathIndex = LanePathCount - 1;
	auto CurLocation = Path.Path.StartLaneLocation;
	PathPoints.Add(CurLocation.Position);
	for (int32 LanePathIndex = 0; LanePathIndex < LanePathCount; ++LanePathIndex)
	{
		// 尝试先跨车道
		// If the next lane is in the same zone, draw a line to it to show that we are changing lane
		if (LanePathIndex+1 < LanePathCount)
		{
			const int32 CurLaneIndex = LanePath.Lanes[LanePathIndex].Index;
			const int32 NextLaneIndex = LanePath.Lanes[LanePathIndex+1].Index;
			const int32 CurZoneIndex = ZoneStorage.Lanes[CurLaneIndex].ZoneIndex;
			const int32 NextZoneIndex = ZoneStorage.Lanes[NextLaneIndex].ZoneIndex;
			if (CurZoneIndex == NextZoneIndex)
			{
				// 即将抵达终点
				if (LanePathIndex+1 == LastLanePathIndex)
				{
					float Width = 0.f;
					UE::ZoneGraph::Query::GetLaneWidth(ZoneStorage, NextLaneIndex, Width);
					const float SearchDistance = 5.f * Width; // Arbitrary search dist
					FZoneGraphLaneLocation FoundLocation;
					float DistanceSqr = 0.f;
					FBox Bounds(LanePath.EndLaneLocation.Position, LanePath.EndLaneLocation.Position);
					Bounds = Bounds.ExpandBy(SearchDistance);
					const bool bFound = UE::ZoneGraph::Query::FindNearestLocationOnLane( // Find nearest location on the next lane
						ZoneStorage,
						FZoneGraphLaneHandle(CurLaneIndex, ZoneStorage.DataHandle),
						Bounds,
						FoundLocation,
						DistanceSqr
					);
					if (bFound && FoundLocation.DistanceAlongLane > CurLocation.DistanceAlongLane)
					{
						// 如果终点在前面，就这样绘制然后结束！
						PathPoints.Append(ConvertLaneToPoints(ZoneStorage, CurLocation, FoundLocation));
						PathPoints.Add(LanePath.EndLaneLocation.Position);
						return true;
					}
				}
				
				float Width = 0.f;
				UE::ZoneGraph::Query::GetLaneWidth(ZoneStorage, CurLaneIndex, Width);
				const float SearchDistance = 5.f * Width; // Arbitrary search dist
				FZoneGraphLaneLocation FoundLocation;
				float DistanceSqr = 0.f;
				FBox Bounds(CurLocation.Position, CurLocation.Position);
				Bounds = Bounds.ExpandBy(SearchDistance);
				const bool bFound = UE::ZoneGraph::Query::FindNearestLocationOnLane( // Find nearest location on the next lane
					ZoneStorage,
					FZoneGraphLaneHandle(NextLaneIndex, ZoneStorage.DataHandle),
					Bounds,
					FoundLocation,
					DistanceSqr
				);

				if (bFound)
				{
					PathPoints.Add(FoundLocation.Position);
					CurLocation = FoundLocation;
					continue;
				}
				else
				{
					// 保底检测，也就是说路径中间的换逆向，就会立马过去！
					FBox NewBounds(PathPoints.Last(), PathPoints.Last());
					NewBounds = NewBounds.ExpandBy(SearchDistance);
					const bool bNewFound = UE::ZoneGraph::Query::FindNearestLocationOnLane( // Find nearest location on the next lane
						ZoneStorage,
						FZoneGraphLaneHandle(NextLaneIndex, ZoneStorage.DataHandle),
						NewBounds,
						FoundLocation,
						DistanceSqr
					);
					if (bNewFound)
					{
						//PathPoints.Add(FoundLocation.Position);
						CurLocation = FoundLocation;
						continue;
					}
				}
			}
		}

		if (CurLocation.IsValid())
		{
			// If we have a CurLocation, draw from the CurLocation location to the end of the lane
			FZoneGraphLaneLocation TempLocation;
			TempLocation.Reset();
			if (LanePathIndex == LastLanePathIndex)
			{
				TempLocation = LanePath.EndLaneLocation;
			}

			// Draw the lane between cur and temp locations
			PathPoints.Append(ConvertLaneToPoints(ZoneStorage, CurLocation, TempLocation));
			CurLocation.Reset(); // Clear CurLocation for next iterations
		}
		else if (LanePathIndex < LastLanePathIndex)
		{
			// No current location, draw the full lane
			PathPoints.Append(ConvertLaneToPoints(ZoneStorage, LanePath.Lanes[LanePathIndex]));
		}
		else
		{
			// Last lane of the path, draw from the begining of the lane to the end location
			check(LanePathIndex == LastLanePathIndex);
			const FZoneGraphLaneLocation& Location = LanePath.EndLaneLocation;
			if (!ensure(Location.LaneHandle.IsValid()))
			{
				break;
			}

			float ZoneWidth = GetZoneWidthByLaneIndex(ZoneStorage, Location.LaneHandle.Index);
			FZoneGraphLaneLocation LastLocation;
			float LastDistSqr;
			ZoneGraph->FindNearestLocationOnLane(Location.LaneHandle, PathPoints.Last(), ZoneWidth, LastLocation, LastDistSqr);
			PathPoints.Append(ConvertLaneToPoints(ZoneStorage, LastLocation, Location));
		}
	}
	
	return true;
}

TArray<FVector> UMinimapSubsystem::ConvertPathToPoints(const FZoneGraphStorage& ZoneStorage, const FZoneGraphLanePath& LanePath)
{
	TArray<FVector> Result;
	
    // Draw start and end locations
	FZoneGraphLaneLocation CurLocation = LanePath.StartLaneLocation;
	Result.Add(CurLocation.Position);
	if (!CurLocation.IsValid())
	{
		// LanePath is not set properly
		return Result;
	}

	const int32 LanePathCount = LanePath.Lanes.Num();
	const int32 LastLanePathIndex = LanePathCount - 1;
	
	// 遍历Lane路线
	for (int32 LanePathIndex = 0; LanePathIndex < LanePathCount; ++LanePathIndex)
	{
		// If the next lane is in the same zone, draw a line to it to show that we are changing lane
		if (LanePathIndex+1 < LanePathCount)
		{
			const int32 CurLaneIndex = LanePath.Lanes[LanePathIndex].Index;
			const int32 NextLaneIndex = LanePath.Lanes[LanePathIndex+1].Index;
			const int32 CurZoneIndex = ZoneStorage.Lanes[CurLaneIndex].ZoneIndex;
			const int32 NextZoneIndex = ZoneStorage.Lanes[NextLaneIndex].ZoneIndex;
			// 如果下一个路线在同一个zone，那就直接横穿马路！！！！
			if (CurZoneIndex == NextZoneIndex)
			{
				// 如果是最后快到达了，则先在正向走到终点附近，再横穿过去
				if (LanePathIndex+1 == LastLanePathIndex)
				{
					float Width = 0.f;
					UE::ZoneGraph::Query::GetLaneWidth(ZoneStorage, NextLaneIndex, Width);
					const float SearchDistance = 5.f * Width; // Arbitrary search dist
					FZoneGraphLaneLocation FoundLocation;
					float DistanceSqr = 0.f;
					FBox Bounds(LanePath.EndLaneLocation.Position, LanePath.EndLaneLocation.Position);
					Bounds = Bounds.ExpandBy(SearchDistance);
					const bool bFound = UE::ZoneGraph::Query::FindNearestLocationOnLane( // Find nearest location on the next lane
						ZoneStorage,
						FZoneGraphLaneHandle(CurLaneIndex, ZoneStorage.DataHandle),
						Bounds,
						FoundLocation,
						DistanceSqr
					);
					if (bFound)
					{
						// 结束！
						Result.Append(ConvertLaneToPoints(ZoneStorage, CurLocation, FoundLocation));
						Result.Add(LanePath.EndLaneLocation.Position);
						return Result;
					}
				}
				// 以最短路径进入反向车道
				else
				{
					float Width = 0.f;
					UE::ZoneGraph::Query::GetLaneWidth(ZoneStorage, CurLaneIndex, Width);
					const float SearchDistance = 5.f * Width; // Arbitrary search dist
					FZoneGraphLaneLocation FoundLocation;
					float DistanceSqr = 0.f;
					FBox Bounds(CurLocation.Position, CurLocation.Position);
					Bounds = Bounds.ExpandBy(SearchDistance);
					const bool bFound = UE::ZoneGraph::Query::FindNearestLocationOnLane( // Find nearest location on the next lane
						ZoneStorage,
						FZoneGraphLaneHandle(NextLaneIndex, ZoneStorage.DataHandle),
						Bounds,
						FoundLocation,
						DistanceSqr
					);
					if (bFound)
					{
						Result.Add(FoundLocation.Position);
						CurLocation = FoundLocation;
						continue;
					}
					else
					{
						//保底检测
						FBox NewBounds(Result.Last(), Result.Last());
						NewBounds = NewBounds.ExpandBy(SearchDistance);
						const bool bNewFound = UE::ZoneGraph::Query::FindNearestLocationOnLane( // Find nearest location on the next lane
							ZoneStorage,
							FZoneGraphLaneHandle(NextLaneIndex, ZoneStorage.DataHandle),
							NewBounds,
							FoundLocation,
							DistanceSqr
						);
						if (bNewFound)
						{
							Result.Add(FoundLocation.Position);
							CurLocation = FoundLocation;
							continue;
						}
					}
				}
			}
		}
		
		if (CurLocation.IsValid())
		{
			// 一般是开始的时候的绘制
			// If we have a CurLocation, draw from the CurLocation location to the end of the lane
			FZoneGraphLaneLocation TempLocation;
			TempLocation.Reset();
			if (LanePathIndex == LastLanePathIndex)
			{
				TempLocation = LanePath.EndLaneLocation;
			}

			// Draw the lane between cur and temp locations
			Result.Append(ConvertLaneToPoints(ZoneStorage, CurLocation, TempLocation));
			CurLocation.Reset(); // Clear CurLocation for next iterations
		}
		else if (LanePathIndex < LastLanePathIndex)
		{
			// 下一段路是完整的车道，所以画完整
			// No current location, draw the full lane
			auto FullLanePoints = ConvertLaneToPoints(ZoneStorage, LanePath.Lanes[LanePathIndex]);
			FullLanePoints.RemoveAt(0);
			Result.Append(FullLanePoints);
		}
		else
		{
			// 正常情况下的最后一个前向车道（画到正常的终点）
			// Last lane of the path, draw from the begining of the lane to the end location
			check(LanePathIndex == LastLanePathIndex);
			const FZoneGraphLaneLocation& Location = LanePath.EndLaneLocation;
			if (!ensure(Location.LaneHandle.IsValid()))
			{
				break;
			}

			const int32 SegEnd = Location.LaneSegment;
			const FZoneLaneData& Lane = ZoneStorage.Lanes[Location.LaneHandle.Index];
			for (int32 SegIndex = Lane.PointsBegin + 1; SegIndex < SegEnd; SegIndex++)
			{
				const FVector Point = ZoneStorage.LanePoints[SegIndex];
				Result.Add(Point);
			}
		}
	}
	
	return Result;
}

TArray<FVector> UMinimapSubsystem::ConvertLaneToPoints(const FZoneGraphStorage& ZoneStorage,
	const FZoneGraphLaneHandle& LaneHandle)
{
	TArray<FVector> Result;
	if (!ensure(LaneHandle.IsValid()))
	{
		return Result;
	}
	
	const FZoneLaneData& Lane = ZoneStorage.Lanes[LaneHandle.Index];
	for (int32 i = Lane.PointsBegin + 1; i < Lane.PointsEnd; i++)
	{
		const FVector Point = ZoneStorage.LanePoints[i];
		Result.Add(Point);
	}

	return Result;
}

TArray<FVector> UMinimapSubsystem::ConvertLaneToPoints(const FZoneGraphStorage& ZoneStorage,
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

		//if (InStartLocation.DistanceAlongLane > InEndLocation.DistanceAlongLane)
		//{
		//	// Swap
		//	StartLocation = InEndLocation;
		//	EndLocation   = InStartLocation;
		//}
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

void UMinimapSubsystem::AddMinimapPin(FGuid Guid)
{
    if (ShownMapPinsGuids.Find(Guid) < 0)
    {
        ShownMapPinsGuids.Add(Guid);
        OnMapPinShowOnMinimap.Broadcast(Guid);
    }
}

void UMinimapSubsystem::RemoveMinimapPin(FGuid Guid)
{
    if (ShownMapPinsGuids.Find(Guid) >= 0)
    {
        OnMapPinHideOnMinimap.Broadcast(Guid);
        ShownMapPinsGuids.Remove(Guid);
    }
}

void UMinimapSubsystem::RegisterComponent(UMinimapComponent* Component)
{
    if (Component != nullptr)
    {
        MinimapComponentRegistry.Add(Component);
        OnComponentRegistered.Broadcast(Component);
    }
}

void UMinimapSubsystem::UnregisterComponent(UMinimapComponent* Component)
{
    if (Component != nullptr)
    {
        RemoveMinimapPin(Component->MinimapGuid);
        OnComponentUnregistered.Broadcast(Component);
        MinimapComponentRegistry.Remove(Component);
    }
}

void UMinimapSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!CurrentLocalPlayerActor)
    {
        return;
    }

	// Should update nav query.
    if (bShouldUpdateNavQuery)
    {
    	// Update nav query start position using local player actor location.
    	if (bAutoUpdateStartLocation)
    	{
    		NavQueryStartPosition = CurrentLocalPlayerActor->GetActorLocation();
    	}
    	// Update nav query period. Using task to do async task update.
    	NavQueryTime += DeltaTime;
    	if (NavQueryTime >= NavQueryPeriod)
    	{
    		NavQueryTime = 0.0f;
    		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
			{
    			FZoneGraphLanePath_BP OutPath;
    			bPathPointsValid = GetZoneGraphPathBP(NavQueryStartPosition, NavQueryEndPosition, NavQueryExtend, OutPath);
    			GetPathPoints(OutPath, NavQueryOutPathPoints);
			});
    	}
    }
	
    // Add pins guid and add always show pin
    TArray<FGuid> MapPinsGuidArray;
    for (auto Comp : MinimapComponentRegistry)
    {
        // ignore not visible component.
        if (!Comp->ShouldVisible())
        {
            continue;
        }
        
        if (!Comp->bAlwaysShow)
        {
            MapPinsGuidArray.AddUnique(Comp->MinimapGuid);
        }
        else if (Comp->bIsIndividual)
        {
            AddMinimapPin(Comp->MinimapGuid);
        }
    }
    for (auto Pin : StaticMapPins)
    {
        if (!Pin.bAlwaysOnMinimap)
        {
            MapPinsGuidArray.AddUnique(Pin.IdentifyGuid);
        }
        else
        {
            AddMinimapPin(Pin.IdentifyGuid);
        }
    }
    
    // Update visible
    for (auto MapPin : MapPinsGuidArray)
    {
    	bool Success;
    	const auto MapPinStruct = GetShownMinimapPin(MapPin, Success);
	    if (Success)
	    {
	    	if (FVector::Dist2D(CurrentLocalPlayerActor->GetActorLocation(), MapPinStruct.Location) <= MinimapRadius / 2)
	    	{
	    		AddMinimapPin(MapPin);
	    	}
	    	else
	    	{
	    		RemoveMinimapPin(MapPin);
	    	}
	    }
    }
}

TStatId UMinimapSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMinimapSubsystem, STATGROUP_Tickables);
}
