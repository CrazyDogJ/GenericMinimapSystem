// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MinimapMapData.generated.h"

struct FHotPointInfo;

UCLASS(BlueprintType)
class MINIMAP_API UMinimapMapData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap")
	FString LevelName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	UTexture2D* MapTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	float MapSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	float TextureSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	FVector CaptureActorLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	TArray<FHotPointInfo> HotPointInfos;
};
