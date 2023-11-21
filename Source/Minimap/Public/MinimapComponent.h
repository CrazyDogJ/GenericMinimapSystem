// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MinimapComponent.generated.h"

class UMinimapSubsystem;
class AMapPinActor;

UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class MINIMAP_API UMinimapComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	friend class UMinimapSubsystem;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	

	UMinimapSubsystem* GetMinimapSubsystem() const;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	FSlateBrush PinSlateBrush;

	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 UniqueColorIndex = -1;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	bool bRotate;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	bool bAlwaysShow;
	
	UPROPERTY(BlueprintReadWrite, Replicated)
	AMapPinActor* TempPin;

	UFUNCTION(BlueprintImplementableEvent)
	void OnCompReg(UMinimapComponent* Component);

	UFUNCTION(BlueprintImplementableEvent)
	void OnCompUnreg(UMinimapComponent* Component);
	
//helper functions
public:

	UFUNCTION(BlueprintPure)
	APlayerState* GetPlayerState() const;

	UFUNCTION(BlueprintPure)
	bool IsLocalControlled() const;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;	
};
