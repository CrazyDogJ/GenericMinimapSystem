// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MinimapComponent.h"
#include "MinimapSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapComponentEvent, UMinimapComponent*, Component);

USTRUCT(BlueprintType)
struct FStaticMapPin
{
	GENERATED_BODY()

	FStaticMapPin()
		:Location(FVector::ZeroVector), MapPinBrush(FSlateBrush())
	{
	}

	FStaticMapPin(FVector a, FSlateBrush b)
		:Location(a), MapPinBrush(b)
	{
	}
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Location;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSlateBrush MapPinBrush;
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
	 * @return Id that reference to the static pin
	 */
	UFUNCTION(BlueprintCallable, Category=MinimapSubsystem)
	int AddStaticLocationPin(FVector Location, FSlateBrush PinSlateBrush);

	UFUNCTION(BlueprintCallable, Category=MinimapSubsystem)
	bool RemoveStaticLocationPin(int Id);

protected:
	/* All the Minimap Components currently existing in the world */
	TArray<TWeakObjectPtr<UMinimapComponent>> MinimapComponentRegistry;

	/* All the static map pins in the world*/
	TArray<FStaticMapPin> StaticMapPins;

protected:
	virtual void RegisterComponent(UMinimapComponent* Component);
	virtual void UnregisterComponent(UMinimapComponent* Component);
};
