// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapSubsystem.h"

#include "GameFramework/PlayerState.h"

void UMinimapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{

}

void UMinimapSubsystem::Deinitialize()
{
    MinimapComponentRegistry.Empty();
}

TArray<UMinimapComponent*> UMinimapSubsystem::GetRegisteredComponents() const
{
    TArray<UMinimapComponent*> Result;
    for (const TWeakObjectPtr<UMinimapComponent>& comp : MinimapComponentRegistry)
    {
        if (comp.IsValid())
        {
            Result.Add(comp.Get());
        }
    }

    return Result;
}

void UMinimapSubsystem::RegisterComponent(UMinimapComponent* Component)
{
    if (Component != nullptr)
    {
        MinimapComponentRegistry.Add(Component);
        OnComponentRegistered.Broadcast(Component);
    }
}

void UMinimapSubsystem::UnregisterComponent(UMinimapComponent* Component)
{
    if (Component != nullptr)
    {
        MinimapComponentRegistry.Remove(Component);
        OnComponentUnregistered.Broadcast(Component);
    }
}
