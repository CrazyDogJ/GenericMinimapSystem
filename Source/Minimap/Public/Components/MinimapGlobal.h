// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapFastArray.h"
#include "Components/ActorComponent.h"
#include "MinimapGlobal.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class MINIMAP_API UMinimapGlobal : public UActorComponent
{
	GENERATED_BODY()

public:
	UMinimapGlobal();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated)
	FPoiStateList PoiStateList;
	
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
