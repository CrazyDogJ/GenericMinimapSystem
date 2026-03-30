// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapSubsystem.h"

#include "Components/MinimapComponent.h"
#include "MinimapSettings.h"
#include "Kismet/GameplayStatics.h"

void UMinimapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMinimapSubsystem::Deinitialize()
{
    MinimapComponentRegistry.Empty();
    StaticMapPins.Empty();
	
    Super::Deinitialize();
}

TArray<UMinimapComponent*> UMinimapSubsystem::GetRegisteredComponents() const
{
    return MinimapComponentRegistry;
}

TArray<FStaticMapPin> UMinimapSubsystem::GetRegisteredStaticMapPins() const
{
    return StaticMapPins;
}

FGuid UMinimapSubsystem::AddStaticLocationPin(FStaticMapPin InPin)
{
    if (!InPin.IdentifyGuid.IsValid())
    {
        InPin.IdentifyGuid = FGuid::NewGuid();
    }
    StaticMapPins.Add(InPin);
    OnStaticRegistered.Broadcast(InPin);
    return InPin.IdentifyGuid;
}

void UMinimapSubsystem::RemoveStaticLocationPin(FGuid MapPinGuid)
{
    FStaticMapPin Pin;
    Pin.IdentifyGuid = MapPinGuid;
    auto Index = StaticMapPins.Find(Pin);
    if (Index >= 0)
    {
        auto Result = StaticMapPins[Index];
        OnStaticUnregistered.Broadcast(Result);
        StaticMapPins.Remove(Pin);
    }
}

UMinimapMapData* UMinimapSubsystem::GetCurrentMinimapMapData()
{
    if (CurrentMinimapMapData)
    {
        if (CurrentMinimapMapData->LevelName == UGameplayStatics::GetCurrentLevelName(GetWorld()))
        {
            return CurrentMinimapMapData;
        }
    }
    
    UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>();
    if (auto Value = Settings->MapsInfos.Find(UGameplayStatics::GetCurrentLevelName(GetWorld())))
    {
        CurrentMinimapMapData = Value->LoadSynchronous();
        if (!CurrentMinimapMapData)
        {
            return nullptr;
        }
    	// Update virtual texture.
    	CurrentMinimapMapData->MapTexture->UpdateResource();
        return CurrentMinimapMapData;
    }
    
    return nullptr;
}

FHotPointInfo UMinimapSubsystem::GetHotPointInfoFromGuid(FGuid Guid, bool& bSuccess)
{
    if (CurrentMinimapMapData && Guid.IsValid())
    {
        if (const auto Ptr = CurrentMinimapMapData->HotPointInfos.Find(Guid))
        {
            bSuccess = true;
            return *Ptr;
        }
    }

    bSuccess = false;
    return FHotPointInfo();
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
