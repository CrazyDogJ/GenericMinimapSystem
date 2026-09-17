// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/UserWidgetPool.h"
#include "MinimapBaseUserWidget.generated.h"

class IMinimapWidgetInterface;
class UOverlay;
class UMapPinUserWidget;
class UMinimapSubsystem;

#define TYPE_MINIMAP 0
#define TYPE_MAINMAP 1

/** Base class of minimap system widgets. */
UCLASS()
class MINIMAP_API UMinimapBaseUserWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	UPROPERTY(Transient)
	FUserWidgetPool WidgetPool;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	
public:
	UMinimapBaseUserWidget(const FObjectInitializer& ObjectInitializer);
	
	void AddMapPin(FGuid Guid);
	void RemoveMapPin(FGuid Guid);

	// Interface pointer to get data.
	UPROPERTY()
	TWeakObjectPtr<UObject> MinimapDataSourceObject;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	APawn* LocalPawn;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	TMap<FGuid, UMapPinUserWidget*> Markers;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	TSubclassOf<UMapPinUserWidget> MarkerWidgetClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	uint8 MarkerClassType = -1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	TSet<FGameplayTag> HiddenCategoryTags;
	
	TSubclassOf<UMapPinUserWidget> GetCustomClass(const FGuid& Guid);

	IMinimapWidgetInterface* TryGetDataInterface() const;
	
	UFUNCTION(BlueprintImplementableEvent,	Category = "Minimap|Widgets")
	UOverlay* GetMarkersOverlay() const;
	
	bool ShouldHide(const FGuid& Id) const;
	bool ShouldHide(const FGameplayTag& InTag) const;
	
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMinimapSubsystem* GetMinimapSubsystem() const;
	
	UFUNCTION(BlueprintPure, Category = "Minimap")
	AActor* GetLocalPlayerActor() const;
};
