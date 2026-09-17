// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapStructs.h"
#include "NetRelevantObject.h"
#include "MinimapPinObject.generated.h"

class UMinimapGlobal;
class UMapPinUserWidget;

UCLASS()
class MINIMAP_API UMinimapPinObject : public UNetRelevantObject
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UMinimapGlobal* GetOwningMinimapGlobal() const;
	
	virtual void NativeBeginPlay() override;
	virtual void NativeEndPlay() override;
	
	void UpdateTransform();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, ReplicatedUsing=OnRep_ReplicatingData)
	FMinimapPinData ReplicatingData;
	
	UFUNCTION()
	virtual void OnRep_ReplicatingData();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnReplicatingDataChanged();
};
