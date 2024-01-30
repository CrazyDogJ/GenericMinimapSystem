// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapComponent.h"
#include "MinimapComponent_Player.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class MINIMAP_API UMinimapComponent_Player : public UMinimapComponent
{
	GENERATED_BODY()

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UMinimapComponent_Player(const FObjectInitializer& ObjectInitializer);
	
public:
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	FSlateBrush TempPinBrush;

	UPROPERTY(BlueprintReadOnly)
	APawn* OwnerPawn;
	
	/**
	 * Add temp pin at the mid of screen.
	 */
	UFUNCTION(BlueprintCallable)
	void AddTempPin();

	UFUNCTION(BlueprintCallable)
	void AddTempPin_MainMap(const FVector2D Location, const FMinimapStruct MapData, const ECollisionChannel TraceChannel);

	UFUNCTION(BlueprintCallable)
	void RemoveTempPin_MainMap(AMapPinActor* PinActor);
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetUniqueColorIndex();
	
	UFUNCTION(Server, Reliable)
	void AddTempPinExec(FVector Location);

	UFUNCTION(Server, Reliable)
	void RemoveTempPinExec(AMapPinActor* PinActor);

	bool GetHitResultAtScreenPosition(const FVector2D ScreenPosition, const ECollisionChannel TraceChannel, const FCollisionQueryParams& CollisionQueryParams, FHitResult& HitResult) const;
	
protected:
	virtual void BeginPlay() override;

	virtual void PostLoad() override;
};
