// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/MinimapComponent_Player.h"
#include "GameFramework/Actor.h"
#include "MapHotPointActor.generated.h"

UCLASS(Blueprintable)
class MINIMAP_API AMapHotPointActor : public AActor
{
	GENERATED_BODY()

public:
	AMapHotPointActor();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FString HotPointLevelName = FString("");
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FPoiInfo PoiInfo;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void FoundThisMapHotPoint(UMinimapComponent_Player* Player, bool Global);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USceneComponent* Root;
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UTexture2D* DefaultTexture;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	UBillboardComponent* BillboardComponent;
#endif

	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
};
