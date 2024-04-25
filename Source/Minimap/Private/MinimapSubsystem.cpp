// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapSubsystem.h"

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
    for (const TObjectPtr<UMinimapComponent>& comp : MinimapComponentRegistry)
    {
        if (comp)
        {
            Result.Add(comp.Get());
        }
    }

    return Result;
}

TArray<FStaticMapPin> UMinimapSubsystem::GetRegisteredStaticMapPins() const
{
    return StaticMapPins;
}

void UMinimapSubsystem::AddStaticLocationPin(FVector Location, FSlateBrush PinSlateBrush, bool bAddToOverlay)
{
    const auto mapPin = FStaticMapPin(Location, PinSlateBrush, bAddToOverlay);
    StaticMapPins.Add(mapPin);
    OnStaticRegistered.Broadcast(mapPin);
}

void UMinimapSubsystem::RemoveStaticLocationPin(FVector Location)
{
    int index = 0;
    for (const auto pin : StaticMapPins)
    {
        if (pin.Location == Location)
        {
            OnStaticUnregistered.Broadcast(pin);
            StaticMapPins.RemoveAt(index);
            break;
        }
        index++;
    }
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
        OnComponentUnregistered.Broadcast(Component);
        MinimapComponentRegistry.Remove(Component);
    }
}
