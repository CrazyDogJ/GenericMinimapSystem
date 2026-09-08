// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapFastArray.h"
#include "Components/ActorComponent.h"
#include "MinimapGlobal.generated.h"

class UMinimapComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class MINIMAP_API UMinimapGlobal : public UActorComponent
{
	GENERATED_BODY()

public:
	UMinimapGlobal();

	/** Poi found record */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated, Category = "Minimap|POI")
	FPoiStateList PoiStateList;

	/** Replicated pin state update period time. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Minimap|Map Pins")
	float PinStateUpdatePeriod = 0.5f;
	
	/** Timer of pin state update event. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|Map Pins")
	FTimerHandle PinStateUpdateTimer;
	
	/** Runtime pin state list. ( Minimap Component List ) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated, Category = "Minimap|Map Pins")
	FMapPinStateList PinStateList;
	
protected:
	void UpdatePinStateList();
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
