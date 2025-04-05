// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/Datatable.h"
#include "MinimapStructs.generated.h"

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
struct FStaticMapPin
{
	GENERATED_BODY()

	FStaticMapPin()
		: Location(FVector::ZeroVector), MapPinBrush(FSlateBrush())
	{
	}

	FStaticMapPin(const FVector& Loc, float Yaw, const FSlateBrush& Brush, bool HasRotation, bool AddOverlay, bool AlwaysOnMinimap)
		: Location(Loc), Yaw(Yaw), MapPinBrush(Brush), bHasRotation(HasRotation), bAddToOverlay(AddOverlay), bAlwaysOnMinimap(AlwaysOnMinimap)
	{
	}

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGuid IdentifyGuid;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Location;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Yaw = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSlateBrush MapPinBrush;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTag CategoryTag;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHasRotation = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddToOverlay = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAlwaysOnMinimap = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText PinName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText PinDescription;
	
	bool operator==(const FStaticMapPin& Other) const
	{
		return IdentifyGuid == Other.IdentifyGuid;
	}
};
