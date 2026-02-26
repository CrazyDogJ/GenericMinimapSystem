// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapPinUserWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/UserWidgetPool.h"
#include "MinimapBaseUserWidget.generated.h"

class UOverlay;
class UMapPinUserWidget;
class UMinimapComponent_Player;
class UMinimapSubsystem;

UCLASS()
class MINIMAP_API UMinimapBaseUserWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	UPROPERTY(Transient)
	FUserWidgetPool WidgetPool;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	
public:
	explicit UMinimapBaseUserWidget(const FObjectInitializer& Initializer);
	
	void AddMapPin(FGuid Guid);
	void RemoveMapPin(FGuid Guid);

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	APawn* LocalPawn;
	
	virtual TSubclassOf<UMapPinUserWidget> GetCustomClass(const FGuid& Guid) { return MarkerWidgetClass; }
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	TSubclassOf<UMapPinUserWidget> MarkerWidgetClass;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	TMap<FGuid, UMapPinUserWidget*> Markers;

	UFUNCTION(BlueprintImplementableEvent,	Category = "Minimap|Widgets")
	UOverlay* GetMarkersOverlay() const;
	
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMinimapSubsystem* GetMinimapSubsystem() const;
	
	UFUNCTION(BlueprintPure, Category = "Minimap")
	AActor* GetLocalPlayerActor() const;
	
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMinimapComponent_Player* GetLocalPlayerMinimapComponent() const;
};
