// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapStructs.h"
#include "MinimapSubsystem.generated.h"

class AMapPinActor;
class UMinimapGlobal;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapUserSettingsChangedEvent, UMinimapUserSettings*, MinimapUserSettings);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHotPointChangeEvent, const FString&, LevelName, const FGuid&, Guid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapGlobalReadyEvent, UMinimapGlobal*, MinimapGlobal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalMapDataChangeEvent, const UMinimapMapData*, MinimapMapData);

UCLASS(DisplayName = "Minimap Subsystem")
class MINIMAP_API UMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
#pragma region Delegate
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FMinimapUserSettingsChangedEvent OnMinimapUserSettingsChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FHotPointChangeEvent OnHotPointFoundEvent;
	
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FHotPointChangeEvent OnHotPointRemoveEvent;
	
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FMinimapGlobalReadyEvent OnMinimapGlobalReadyEvent;
	
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FLocalMapDataChangeEvent OnLocalMapDataChangeEvent;
	
#pragma endregion 
	
	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem")
	UMinimapMapData* CurrentMinimapMapData;

	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem")
	UMinimapMapData* LocalMinimapMapData;
	
	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem")
	AMapPinActor* LocalTempPinActor;
	
public:
	void SetLocalMinimapMapData(UMinimapMapData* LocalMinimapMapData);
	
	/** Get current minimap data for this level. */
	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	UMinimapMapData* GetCurrentMinimapMapData();

	/** Get current map's hot point info by guid. */
	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	bool GetHotPointInfoFromGuid(FGuid Guid, FPoiInfo& OutInfo);
	
	/** Try get minimap global component. */
	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	UMinimapGlobal* GetMinimapGlobal() const;
	
	bool HasAuthority() const;
};
