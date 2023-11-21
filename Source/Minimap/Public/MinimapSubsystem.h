// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MinimapComponent.h"
#include "MinimapSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinimapComponentEvent, UMinimapComponent*, Component);

/**
 * 
 */
UCLASS(DisplayName = "Minimap Subsystem")
class MINIMAP_API UMinimapSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	// 开始子系统
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// 结束子系统

	friend class UMinimapComponent;

	/* Called when actor with Minimap Component appears in the world */
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FMinimapComponentEvent OnComponentRegistered;

	/* Called when actor with Minimap Component disappears from the world */
	UPROPERTY(BlueprintAssignable, Category = "MinimapSubsystem")
	FMinimapComponentEvent OnComponentUnregistered;

	UFUNCTION(BlueprintPure, Category = "MinimapSubsystem")
	TArray<UMinimapComponent*> GetRegisteredComponents() const;

protected:
	/* All the Minimap Components currently existing in the world */
	TArray<TWeakObjectPtr<UMinimapComponent>> MinimapComponentRegistry;

protected:
	virtual void RegisterComponent(UMinimapComponent* Component);
	virtual void UnregisterComponent(UMinimapComponent* Component);
};
