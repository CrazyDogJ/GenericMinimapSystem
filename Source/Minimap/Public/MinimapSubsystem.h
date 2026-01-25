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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShownMapPinEvent, FGuid, Guid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapUserSettingsChangedEvent, UMinimapUserSettings*, MinimapUserSettings);

/**
 * 
 */
UCLASS(DisplayName = "Minimap Subsystem")
class MINIMAP_API UMinimapSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	friend class UMinimapComponent;

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
	FShownMapPinEvent OnMapPinShowOnMinimap;

	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FShownMapPinEvent OnMapPinHideOnMinimap;

	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FMinimapUserSettingsChangedEvent OnMinimapUserSettingsChangedEvent;
	
#pragma endregion 
	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem")
	UMinimapMapData* CurrentMinimapMapData;

	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem")
	AActor* CurrentLocalPlayerActor;

	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem", meta=(Units = "cm"))
	float MinimapRadius = 10000.0f;

public:
	// Nav query for background query.
	/** Should auto update nav query start location. */
	UPROPERTY(BlueprintReadWrite, Category = "MinimapSubsystem|Nav Query")
	bool bAutoUpdateStartLocation = true;

	/** Should do nav query update. */
	UPROPERTY(BlueprintReadWrite, Category = "MinimapSubsystem|Nav Query")
	bool bShouldUpdateNavQuery = false;

	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem|Nav Query")
	bool bPathPointsValid = false;
	
	UPROPERTY(BlueprintReadWrite, Category = "MinimapSubsystem|Nav Query")
	FVector NavQueryStartPosition;

	UPROPERTY(BlueprintReadWrite, Category = "MinimapSubsystem|Nav Query")
	FVector NavQueryEndPosition;

	UPROPERTY(BlueprintReadWrite, Category = "MinimapSubsystem|Nav Query")
	FVector NavQueryExtend = FVector(10000.0f);
	
	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem|Nav Query")
	TArray<FVector> NavQueryOutPathPoints;

	float NavQueryTime = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, Category = "MinimapSubsystem|Nav Query")
	float NavQueryPeriod = 1.0f;

	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem|Nav Query")
	bool ShouldShowNavPath() const;
	// Nav query for background query.
	
	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	TArray<UMinimapComponent*> GetRegisteredComponents() const;

	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	TArray<FStaticMapPin> GetRegisteredStaticMapPins() const;

	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	FStaticMapPin GetShownMinimapPin(FGuid Guid) const;

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	FGuid AddStaticLocationPin(FStaticMapPin InPin);
	
	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	void RemoveStaticLocationPin(FGuid MapPinGuid);

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	UMinimapMapData* GetCurrentMinimapMapData();

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	FHotPointInfo GetHotPointInfoFromGuid(FGuid Guid);
	
	// These three functions are used to track nearby map pins.
	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	void SetupLocalPlayer(AActor* LocalPlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	void SetMinimapRadius(float Radius);

	// Zone Graph helper functions -------------------------------------------------------------------------------------
	// TODO : These are not so important so we can do these in another thread.
	/**
	 * Get zone actor width by lane index
	 */
	float GetZoneWidthByLaneIndex(const FZoneGraphStorage& ZoneStorage, int32 LaneIndex) const;
	
	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	int GetPathLaneCount(const FZoneGraphLanePath_BP& Path);

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	bool GetZoneGraphPathBP(FVector StartPosition, FVector DestPosition, FVector SearchExtent, FZoneGraphLanePath_BP& Path);

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	bool GetPathPoints(const FZoneGraphLanePath_BP& Path, TArray<FVector>& PathPoints);

	TArray<FVector> ConvertPathToPoints(const FZoneGraphStorage& ZoneStorage, const FZoneGraphLanePath& Path);
	TArray<FVector> ConvertLaneToPoints(const FZoneGraphStorage& ZoneStorage, const FZoneGraphLaneHandle& LaneHandle);
	TArray<FVector> ConvertLaneToPoints(const FZoneGraphStorage& ZoneStorage, const FZoneGraphLaneLocation& InStartLocation, const FZoneGraphLaneLocation& InEndLocation);
	// Zone Graph helper functions -------------------------------------------------------------------------------------
	
	//Helper functions
	void AddMinimapPin(FGuid Guid);
	void RemoveMinimapPin(FGuid Guid);
	
	virtual void RegisterComponent(UMinimapComponent* Component);
	virtual void UnregisterComponent(UMinimapComponent* Component);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
protected:
	/* All the Minimap Components currently existing in the world */
	TArray<TObjectPtr<UMinimapComponent>> MinimapComponentRegistry;

	/* All the static map pins in the world*/
	TArray<FStaticMapPin> StaticMapPins;

	/* All map pins shows on minimap */
	TArray<FGuid> ShownMapPinsGuids;
};
