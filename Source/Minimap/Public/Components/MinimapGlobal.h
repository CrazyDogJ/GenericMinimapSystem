// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapStructs.h"
#include "NetRelevantGlobalComponent.h"
#include "Widgets/MinimapWidgetInterface.h"
#include "MinimapGlobal.generated.h"

class AMapPinActor;
class UMinimapPinObject;
class AGameState;

// Put this on game state.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class MINIMAP_API UMinimapGlobal : public UNetRelevantGlobalComponent, public IMinimapWidgetInterface
{
	GENERATED_BODY()

public:
	UMinimapGlobal();

	/** Poi found record */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated, Category = "Minimap|POI")
	FPoiStateList PoiStateList;
	
	UPROPERTY()
	TMap<AController*, AMapPinActor*> TempMapPinMapping;
	
	UPROPERTY(BlueprintReadOnly)
	TMap<FGuid, FMinimapPinData> LocalMapPins;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	FGuid AddReplicatedMapPin(TSubclassOf<UMinimapPinObject> PinClass, APlayerController* OwnerController, const FName GroupName, const FMinimapPinData& PinData);
	
	UFUNCTION(BlueprintCallable)
	FGuid AddLocalMapPin(const FMinimapPinData& PinData);
	
	UFUNCTION(BlueprintCallable)
	void AddLocalMapPinWithId(const FGuid Id, const FMinimapPinData& PinData);
	
	UFUNCTION(BlueprintCallable)
	void RemoveLocalMapPin(const FGuid Id);
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void AddTempPin(AController* Controller, const FVector& Location, const FSlateBrush& Brush);
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ChangeTempPinLocation(AController* Controller, const FVector& Location);
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void RemoveTempPin(AController* Controller);
	
public:
#pragma region IMinimapWidgetInterface
	
	FMapPinChangeEvent OnMapPinAddEvent;
	
	FMapPinChangeEvent OnMapPinRemoveEvent;
	
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
	// IMinimapWidgetInterface
#pragma endregion IMinimapWidgetInterface
	
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
