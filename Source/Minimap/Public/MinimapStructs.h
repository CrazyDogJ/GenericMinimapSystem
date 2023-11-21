// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
