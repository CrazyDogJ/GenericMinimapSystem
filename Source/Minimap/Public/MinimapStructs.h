// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/Datatable.h"
#include "StructUtils/InstancedStruct.h"
#include "MinimapStructs.generated.h"

class UMinimapMapData;
class UMapPinUserWidget;

/** Minimap indices container struct. */
USTRUCT(BlueprintType)
struct FMinimapIndices
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	TSet<FGuid> Indices;

	bool Find(const FGuid& InIndex) const
	{
		return Indices.Contains(InIndex);
	}
	
	bool Add(const FGuid& InIndex)
	{
		if (InIndex.IsValid())
		{
			if (Find(InIndex))
			{
				return false;
			}
			
			Indices.Add(InIndex);
			return true;
		}

		return false;
	}
};

/** Base struct for map pin info. */
USTRUCT(BlueprintType)
struct FMapPinBase
{
	GENERATED_BODY()

public:
	FMapPinBase()
		: Location(FVector::ZeroVector), MapPinBrush(FSlateBrush())
	{
	}

	FMapPinBase(const FVector& Loc, float Yaw, const FSlateBrush& Brush, bool HasRotation, bool AddOverlay, bool AlwaysOnMinimap)
		: Location(Loc), Yaw(Yaw), MapPinBrush(Brush), bHasRotation(HasRotation), bAddToOverlay(AddOverlay), bAlwaysOnMinimap(AlwaysOnMinimap)
	{
	}
	
	// Unique id
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGuid IdentifyGuid;
	// Map pin location
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Location;
	// Map pin yaw angle
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Yaw = 0.0f;
	// Default slate brush
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSlateBrush MapPinBrush;
	// Category tag for custom usage
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTag CategoryTag;
	// Should rotate.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHasRotation = false;
	// Should add to screen
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddToOverlay = false;
	// Should minimap always display
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAlwaysOnMinimap = false;
	// Map pin name
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText PinName;
	// Map pin desc
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText PinDescription;
	// Custom widget for minimap
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UMapPinUserWidget> CustomMinimapWidgetClass;
	// Custom widget for main map
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UMapPinUserWidget> CustomMainmapWidgetClass;
	// Custom datas.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FInstancedStruct CustomDatas;

	bool operator==(const FMapPinBase& Other) const
	{
		return IdentifyGuid == Other.IdentifyGuid;
	}
};

/** Static map pin struct. */
USTRUCT(BlueprintType)
struct FStaticMapPin : public FMapPinBase
{
	GENERATED_BODY()
};

/** Hot point map pin info struct. */
USTRUCT(BlueprintType)
struct FHotPointInfo : public FMapPinBase
{
	GENERATED_BODY()

public:

	// Used for localization text.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HotPointId;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsTeleportPoint = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = bIsTeleportPoint))
	FTransform TeleportTransform;
};
