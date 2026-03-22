// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZoneGraphAStar_Custom.h"
#include "UObject/Object.h"
#include "ZoneGraphAStarBlueprint.generated.h"

USTRUCT(BlueprintType)
struct FZoneGraphStorage_BP
{
	GENERATED_BODY()

	FZoneGraphStorage Storage;
};

USTRUCT(BlueprintType)
struct FZoneLaneData_BP
{
	GENERATED_BODY()

	FZoneLaneData LaneData;
};

USTRUCT(BlueprintType)
struct FZoneLaneLinkData_BP
{
	GENERATED_BODY()

	FZoneLaneLinkData LaneLinkData;
};

USTRUCT(BlueprintType)
struct FZoneGraphLaneLocation_BP
{
	GENERATED_BODY()

	FZoneGraphLaneLocation LaneLocation;
};

UCLASS(Blueprintable)
class MINIMAP_API UZoneGraphAStarBlueprint : public UObject
{
	GENERATED_BODY()

public:
	UZoneGraphAStarBlueprint() {}

#pragma region Implementable Events
	UFUNCTION(BlueprintImplementableEvent)
	int32 GetNeighbourCountV2(const FZoneGraphStorage_BP& Storage, const FZoneGraphLaneNodeRef& Node,
		const FZoneGraphLaneLocation_BP& Start, const FZoneGraphLaneLocation_BP& End) const;

	UFUNCTION(BlueprintImplementableEvent)
	FZoneGraphLaneNodeRef GetNeighbour(const FZoneGraphStorage_BP& Storage, const FZoneGraphLaneNodeRef& Node, const int32 NeighbourIndex,
		const FZoneGraphLaneLocation_BP& Start, const FZoneGraphLaneLocation_BP& End) const;

	UFUNCTION(BlueprintImplementableEvent)
	float GetHeuristicCost(const FZoneGraphStorage_BP& Storage, const FZoneGraphLaneNodeRef& NeighbourNode, const FZoneGraphLaneNodeRef& EndNode,
		const FZoneGraphLaneLocation_BP& Start, const FZoneGraphLaneLocation_BP& End) const;

	UFUNCTION(BlueprintImplementableEvent)
	float GetTraversalCost(const FZoneGraphStorage_BP& Storage, const FZoneGraphLaneNodeRef& CurNode, const FZoneGraphLaneNodeRef& NeighbourNode,
		const FZoneGraphLaneLocation_BP& Start, const FZoneGraphLaneLocation_BP& End) const;
#pragma endregion
	
#pragma region BP Functions
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FZoneLaneData_BP GetLaneData(const FZoneGraphStorage_BP& Storage, int32 LaneIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static int32 GetLaneLinkCount(const FZoneLaneData_BP& LaneData);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static int32 GetZoneIndex(const FZoneLaneData_BP& LaneData);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FVector GetLaneLocationPosition(const FZoneGraphLaneLocation_BP& LaneLocation);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static int32 GetLaneLocationLaneIndex(const FZoneGraphLaneLocation_BP& LaneLocation);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float GetLaneLocationLaneDistance(const FZoneGraphLaneLocation_BP& LaneLocation);

	UFUNCTION(BlueprintCallable)
	static FZoneGraphLaneLocation_BP GetAdjacentLaneLocation(const FZoneGraphStorage_BP& Storage,
		const FZoneGraphLaneLocation_BP& CurrentLaneLocation, const int32 AdjacentLaneIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static int32 GetLaneLinkBeginIndex(const FZoneLaneData_BP& LaneData);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FZoneLaneLinkData_BP GetLaneLinkData(const FZoneGraphStorage_BP& Storage, const int32& LinkIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static int32 GetLaneLinkDestLaneIndex(const FZoneLaneLinkData_BP& Link);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static EZoneLaneLinkType GetLaneLinkType(const FZoneLaneLinkData_BP& Link);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FZoneGraphLaneLocation_BP GetLocationAlongDistance(const FZoneGraphStorage_BP& Storage, const int32& LaneIndex, const float& Distance);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float GetLaneLength(const FZoneGraphStorage_BP& Storage, const int32& LaneIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float GetLaneWidth(const FZoneLaneData_BP& LaneData);
#pragma endregion
	
};
