// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/Datatable.h"
#include "StructUtils/InstancedStruct.h"
#include "MinimapStructs.generated.h"

class UMapPinUserWidget;

USTRUCT(BlueprintType)
struct FMinimapStruct : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FString LevelName = FString(TEXT("Level Name Here"));
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	UTexture2D* MapTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float MapSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float TextureSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector CaptureActorLocation;

	bool IsValid() const;
};

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

USTRUCT(BlueprintType)
struct FStaticMapPin : public FMapPinBase
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FHotPointInfo : public FMapPinBase
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool bIsTeleportPoint = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (EditCondition = bIsTeleportPoint))
	FTransform TeleportTransform;
};
