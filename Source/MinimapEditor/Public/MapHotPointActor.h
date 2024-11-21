// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapMapData.h"
#include "GameFramework/Actor.h"
#include "MapHotPointActor.generated.h"

UCLASS()
class MINIMAPEDITOR_API AMapHotPointActor : public AActor
{
	GENERATED_BODY()

public:
	AMapHotPointActor();

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FHotPointInfo Info;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UTexture2D* DefaultTexture;
	
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif

	virtual void OnConstruction(const FTransform& Transform) override;
};
