// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapFastArray.h"
#include "MinimapSubsystem.generated.h"

class UMinimapGlobal;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapUserSettingsChangedEvent, UMinimapUserSettings*, MinimapUserSettings);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHotPointChangeEvent, const FString&, LevelName, const FGuid&, Guid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapPinStateChangeEvent, const FGuid&, MapPinId);

UCLASS(DisplayName = "Minimap Subsystem")
class MINIMAP_API UMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
	friend class UMinimapComponent;

public:
#pragma region Delegate
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FMinimapUserSettingsChangedEvent OnMinimapUserSettingsChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FHotPointChangeEvent OnHotPointFoundEvent;
	
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FHotPointChangeEvent OnHotPointRemoveEvent;
	
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FOnMapPinStateChangeEvent OnMapPinAddEvent;
	
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FOnMapPinStateChangeEvent OnMapPinRemoveEvent;
#pragma endregion 
	
	UPROPERTY(BlueprintReadOnly, Category = "MinimapSubsystem")
	UMinimapMapData* CurrentMinimapMapData;

public:
	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	FMapPinStateList GetLocalPinStateList() const;
	
	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	FMapPinStateList GetGlobalPinStateList() const;

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	FGuid AddStaticMapPin(const FMapPinStateEntry& InEntry);
	
	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	void RemoveStaticMapPin(FGuid MapPinGuid);

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	UMinimapMapData* GetCurrentMinimapMapData();

	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	bool GetHotPointInfoFromGuid(FGuid Guid, FPoiInfo& OutInfo);
	
	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	UMinimapGlobal* GetMinimapGlobal() const;
	
	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	bool GetMapPinCurrentState(FGuid Id, FMapPinStateEntry& OutEntry);
	
	UFUNCTION(BlueprintCallable, Category = "MinimapSubsystem")
	void SetMapPinCurrentState(const FMapPinStateEntry& InEntry);
	
protected:
	/** Local pin state list */
	FMapPinStateList LocalPinStateList;
};
