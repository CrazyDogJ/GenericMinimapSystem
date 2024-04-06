// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MinimapSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapComponentEvent, UMinimapComponent*, Component);

USTRUCT(BlueprintType)
struct FStaticMapPin
{
	GENERATED_BODY()

	FStaticMapPin()
		: Location(FVector::ZeroVector), MapPinBrush(FSlateBrush()), bAddToOverlay(false)
	{
	}

	FStaticMapPin(FVector a, FSlateBrush b, bool c)
		: Location(a), MapPinBrush(b), bAddToOverlay(c)
	{
	}

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Location;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSlateBrush MapPinBrush;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddToOverlay;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStaticMapPinEvent, const FStaticMapPin&, StaticMapPin);
/**
 * 
 */
UCLASS(DisplayName = "Minimap Subsystem")
class MINIMAP_API UMinimapSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	friend class UMinimapComponent;

	/* Called when actor with Minimap Component appears in the world */
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FMinimapComponentEvent OnComponentRegistered;

	/* Called when actor with Minimap Component disappears from the world */
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FMinimapComponentEvent OnComponentUnregistered;

	/* Called when static map pin add in the world */
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FStaticMapPinEvent OnStaticRegistered;

	/* Called when static map pin removed from the world */
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FStaticMapPinEvent OnStaticUnregistered;

	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	TArray<UMinimapComponent*> GetRegisteredComponents() const;

	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	TArray<FStaticMapPin> GetRegisteredStaticMapPins() const;
	
	/**
	 * Add a static location pin on map.
	 * @param Location Static location
	 * @param PinSlateBrush Map pin brush
	 * @param bAddToOverlay Add to overlay
	 * @return Id that reference to the static pin
	 */
	UFUNCTION(BlueprintCallable, Category=MinimapSubsystem)
	void AddStaticLocationPin(FVector Location, FSlateBrush PinSlateBrush, bool bAddToOverlay);

	UFUNCTION(BlueprintCallable, Category=MinimapSubsystem)
	void RemoveStaticLocationPin(FVector Location);

protected:
	/* All the Minimap Components currently existing in the world */
	TArray<TWeakObjectPtr<UMinimapComponent>> MinimapComponentRegistry;

	/* All the static map pins in the world*/
	TArray<FStaticMapPin> StaticMapPins;

protected:
	virtual void RegisterComponent(UMinimapComponent* Component);
	virtual void UnregisterComponent(UMinimapComponent* Component);
};
