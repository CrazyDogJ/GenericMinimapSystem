// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapComponent_Player.h"
#include "MinimapMapData.h"
#include "GameFramework/Actor.h"
#include "MapHotPointActor.generated.h"

UCLASS(Blueprintable)
class MINIMAP_API AMapHotPointActor : public AActor
{
	GENERATED_BODY()

public:
	AMapHotPointActor();

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FHotPointInfo Info;

	UFUNCTION(BlueprintCallable)
	void FoundThisMapHotPoint(UMinimapComponent_Player* Player);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USceneComponent* Root;
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UTexture2D* DefaultTexture;
	
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif

	virtual void OnConstruction(const FTransform& Transform) override;
};
