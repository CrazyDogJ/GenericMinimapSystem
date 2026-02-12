// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MinimapSubsystem.h"
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

	// Properties for minimap static pin struct
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated)
	FGuid MinimapGuid;

	/* If individual, minimap subsystem will treat it as a single map pin instance
	 * else we use it override the static map pin, and it will disappear if static map pin was removed.
	 **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated)
	bool bIsIndividual = true;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, meta=(ExposeOnSpawn))
	FSlateBrush PinSlateBrush;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, meta=(ExposeOnSpawn))
	bool bAddToOverlay;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	FGameplayTag MinimapCategory;
	
	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 UniqueColorIndex = -1;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	bool bRotate;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	bool bAlwaysShow;

	//Get properties to struct
	UFUNCTION(BlueprintPure)
	FStaticMapPin GetCurrentStaticMapPin();

	UFUNCTION(BlueprintNativeEvent)
	bool ShouldVisible();

	UFUNCTION(BlueprintNativeEvent)
	void GetDisplayNameAndDescription(FText& DisplayName, FText& Description);
	
	virtual void NativeGetDisplayNameAndDescription(FText& DisplayName, FText& Description) {}
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
