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
    for (const TWeakObjectPtr<UMinimapComponent>& comp : MinimapComponentRegistry)
    {
        if (comp.IsValid())
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

int UMinimapSubsystem::AddStaticLocationPin(FVector Location, FSlateBrush PinSlateBrush)
{
    const auto mapPin = FStaticMapPin(Location, PinSlateBrush);
    const auto result = StaticMapPins.Add(mapPin);
    OnStaticRegistered.Broadcast(mapPin);
    return result;
}

bool UMinimapSubsystem::RemoveStaticLocationPin(int Id)
{
    if (StaticMapPins.IsValidIndex(Id))
    {
        const auto MapPin = StaticMapPins[Id];
        StaticMapPins.RemoveAt(Id);
        OnStaticUnregistered.Broadcast(MapPin);
        return true;
    }
    return false;
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
