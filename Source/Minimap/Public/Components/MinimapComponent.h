// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MinimapSubsystem.h"
#include "MinimapComponent.generated.h"

class UMinimapGlobal;
class UMinimapSubsystem;
class AMapPinActor;

UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class MINIMAP_API UMinimapComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	friend class UMinimapSubsystem;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	

	/** Used to identify the component */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated)
	FGuid MinimapGuid;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ExposeOnSpawn))
	FSlateBrush PinSlateBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ExposeOnSpawn))
	bool bAddToOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag MinimapCategory;
	
	UPROPERTY(BlueprintReadWrite)
	int32 UniqueColorIndex = -1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRotate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAlwaysShow;
	
	UFUNCTION(BlueprintPure)
	FMapPinStateEntry MakeMapPinEntry() const;
	
	UFUNCTION(BlueprintNativeEvent)
	bool ShouldVisible();

	UFUNCTION(BlueprintNativeEvent)
	void GetDisplayNameAndDescription(FText& DisplayName, FText& Description) const;
	
	virtual void NativeGetDisplayNameAndDescription(FText& DisplayName, FText& Description) const {}
	
	void RegisterGlobalMinimap() const;
	void UnregisterGlobalMinimap() const;

//helper functions
public:
	UMinimapSubsystem* GetMinimapSubsystem() const;

	UFUNCTION(BlueprintPure)
	APlayerState* GetPlayerState() const;

	UFUNCTION(BlueprintPure)
	bool IsLocalControlled() const;
	
	UFUNCTION(BlueprintPure)
	UMinimapGlobal* GetGlobalMinimapComponent() const;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
