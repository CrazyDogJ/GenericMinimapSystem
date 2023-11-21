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
	
public:
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

	/**
	 * Used to do client side event.
	 * @param Pawn 
	 * @param OldController 
	 * @param NewController 
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnPossessed_Client(APawn* Pawn, AController* OldController, AController* NewController);

	bool GetHitResultAtScreenPosition(const FVector2D ScreenPosition, const ECollisionChannel TraceChannel, const FCollisionQueryParams& CollisionQueryParams, FHitResult& HitResult) const;
	
protected:
	//On possess in c++
	UFUNCTION(Client, Reliable)
	void OnPossessed(APawn* Pawn, AController* OldController, AController* NewController);
	
	virtual void BeginPlay() override;
};
