// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapMapData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MinimapSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FStaticMapPin
{
	GENERATED_BODY()

	FStaticMapPin()
		: Location(FVector::ZeroVector), MapPinBrush(FSlateBrush()), bAddToOverlay(false)
	{
	}

	FStaticMapPin(FVector Loc, float Yaw, FSlateBrush Brush, bool bHasRotation, bool AddOverlay)
		: Location(Loc), Yaw(Yaw), MapPinBrush(Brush), bHasRotation(bHasRotation), bAddToOverlay(AddOverlay)
	{
	}

public:
	UPROPERTY(BlueprintReadOnly)
	FGuid IdentifyGuid;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Location;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Yaw = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSlateBrush MapPinBrush;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHasRotation = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddToOverlay;

	bool operator==(const FStaticMapPin& Other) const
	{
		return IdentifyGuid == Other.IdentifyGuid;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapComponentEvent, UMinimapComponent*, Component);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStaticMapPinEvent, const FStaticMapPin&, StaticMapPin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShownMapPinEvent, FGuid, Guid);
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
	
	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem")
	UMinimapMapData* CurrentMinimapMapData;

	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem")
	AActor* CurrentLocalPlayerActor;

	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem", meta=(Units = "cm"))
	float MinimapRadius = 10000.0f;
	
	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	TArray<UMinimapComponent*> GetRegisteredComponents() const;

	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	TArray<FStaticMapPin> GetRegisteredStaticMapPins() const;

	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	FStaticMapPin GetShownMinimapPin(FGuid Guid) const;
	/**
	 * Add a static location pin on map.
	 * @param Location Static location
	 * @param PinSlateBrush Map pin brush
	 * @param bAddToOverlay Add to overlay
	 * @return Id that reference to the static pin
	 */
	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	FGuid AddStaticLocationPin(FVector Location, float Yaw, FSlateBrush PinSlateBrush, bool bHasRotation, bool bAddToOverlay);

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
