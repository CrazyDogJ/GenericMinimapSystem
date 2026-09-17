// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "MinimapWidgetInterface.generated.h"

class UMinimapMapData;
class UMapPinUserWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FMapPinChangeEvent, const FGuid& Id)

// This class does not need to be modified.
UINTERFACE()
class UMinimapWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MINIMAP_API IMinimapWidgetInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// Check the id is in pawn and the pawn is locally controlled.
	virtual bool IsLocalPlayer(const APawn* InPawn, const FGuid& Id) const { return false; }
	
	// Return current registered map pins guids. ( Not include hot points. )
	virtual bool GetRegisteredMapPins(TSet<FGuid>& OutGuid) const { return false; }
	
	// Return current found hot points. ( Recommend use with GetRegisteredMapPins() )
	virtual bool GetFoundHotPoints(TSet<FGuid>& OutGuid) const { return false; }
	
	// Return query hot points in a range (Only found)
	virtual bool QueryHotPoints(const FVector& Location, const float& Radius, TSet<FGuid>& OutGuid) const { return false; }
	
	// Return map pin class to create map pin widget.
	virtual bool GetMapPinClass(const FGuid& Id, const uint8 Type, TSubclassOf<UMapPinUserWidget>& OutClass) const { return false; }
	
	// Return map pin location.
	virtual bool GetLocation(const FGuid& Id, FVector& OutLocation) const { return false; }
	
	// Return map pin has rotation.
	virtual bool GetYaw(const FGuid& Id, float& OutYaw) const { return false; }
	
	// Return map pin brush.
	virtual bool GetBrush(const FGuid& Id, FSlateBrush& OutBrush) const { return false; }
	
	// Return map pin category.
	virtual bool GetCategoryTag(const FGuid& Id, FGameplayTag& OutTag) const { return false; }
	
	// Return is always on minimap.
	virtual bool GetIsAlwaysOnMinimap(const FGuid& Id) const { return false; }
	
	// Return map pin add event.
	virtual FMapPinChangeEvent* GetMapPinAddEvent() { return nullptr; }
	
	// Return map pin remove event.
	virtual FMapPinChangeEvent* GetMapPinRemoveEvent() { return nullptr; }
};
