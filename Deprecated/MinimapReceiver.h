// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapFastArray.h"
#include "Components/ActorComponent.h"
#include "Widgets/MinimapWidgetInterface.h"
#include "MinimapReceiver.generated.h"

// Add to player controller to receive map pin datas from server.
UCLASS(ClassGroup=(Minimap), meta=(BlueprintSpawnableComponent))
class MINIMAP_API UMinimapReceiver : public UActorComponent, public IMinimapWidgetInterface
{
	GENERATED_BODY()

public:
	UMinimapReceiver();
	
	// Local map pin list for players to add himself.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FMapPinStateList LocalMapPinList;
	
	// Replicated map pins from server.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated)
	FMapPinStateList ReplicatedMapPinList;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Replicated)
	TArray<uint8> Channels = {0};
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddChannel(uint8 InChannel);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveChannel(uint8 InChannel);
	
	UMinimapSubsystem* GetMinimapSubsystem() const;
	
	virtual bool ShouldReplicate(const FMapPinStateEntry& InEntry) const;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
#pragma region IMinimapWidgetInterface
	// IMinimapWidgetInterface
	virtual bool IsLocalPlayer(const APawn* InPawn, const FGuid& Id) const override;
	virtual bool GetRegisteredMapPins(TSet<FGuid>& OutGuid) const override;
	virtual bool GetFoundHotPoints(TSet<FGuid>& OutGuid) const override;
	virtual bool QueryHotPoints(const FVector& Location, const float& Radius, TSet<FGuid>& OutGuid) const override;
	virtual bool GetMapPinClass(const FGuid& Id, const uint8 Type, TSubclassOf<UMapPinUserWidget>& OutClass) const override;
	virtual bool GetLocation(const FGuid& Id, FVector& OutLocation) const override;
	virtual bool GetYaw(const FGuid& Id, float& OutYaw) const override;
	virtual bool GetBrush(const FGuid& Id, FSlateBrush& OutBrush) const override;
	virtual bool GetCategoryTag(const FGuid& Id, FGameplayTag& OutTag) const override;
	virtual bool GetIsAlwaysOnMinimap(const FGuid& Id) const override;
	virtual FMapPinChangeEvent* GetMapPinAddEvent() override;
	virtual FMapPinChangeEvent* GetMapPinRemoveEvent() override;
	virtual void AddTempPin(const FVector2D Location, const UMinimapMapData* MapData, const ECollisionChannel TraceChannel) override;
	// IMinimapWidgetInterface
#pragma endregion IMinimapWidgetInterface
};
