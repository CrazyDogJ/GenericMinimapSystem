// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapMapData.h"
#include "MinimapStructs.h"
#include "ZoneGraphTypes.h"
#include "MinimapSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FZoneGraphLanePath_BP
{
	GENERATED_BODY()
	
	FZoneGraphLanePath Path;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapComponentEvent, UMinimapComponent*, Component);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStaticMapPinEvent, const FStaticMapPin&, StaticMapPin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapUserSettingsChangedEvent, UMinimapUserSettings*, MinimapUserSettings);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHotPointFoundEvent, const FHotPointInfo&, HotPointInfo);

/**
 * 
 */
UCLASS(DisplayName = "Minimap Subsystem")
class MINIMAP_API UMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
	friend class UMinimapComponent;
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
#pragma region Delegate
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

	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FMinimapUserSettingsChangedEvent OnMinimapUserSettingsChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FHotPointFoundEvent OnHotPointFoundEvent;
	
#pragma endregion 
	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem")
	UMinimapMapData* CurrentMinimapMapData;

public:
	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	TArray<UMinimapComponent*> GetRegisteredComponents() const;
	
	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	TArray<FStaticMapPin> GetRegisteredStaticMapPins() const;

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	FGuid AddStaticLocationPin(FStaticMapPin InPin);
	
	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	void RemoveStaticLocationPin(FGuid MapPinGuid);

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	UMinimapMapData* GetCurrentMinimapMapData();

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	FHotPointInfo GetHotPointInfoFromGuid(FGuid Guid, bool& bSuccess);

	UFUNCTION(BlueprintPure)
	FStaticMapPin GetShownMinimapPin(FGuid Guid, bool& Success) const;
	
	virtual void RegisterComponent(UMinimapComponent* Component);
	virtual void UnregisterComponent(UMinimapComponent* Component);

protected:
	/* All the Minimap Components currently existing in the world */
	TArray<TObjectPtr<UMinimapComponent>> MinimapComponentRegistry;

	/* All the static map pins in the world*/
	TArray<FStaticMapPin> StaticMapPins;
};
