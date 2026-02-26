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
    	// Update virtual texture.
    	CurrentMinimapMapData->MapTexture->UpdateResource();
        for (auto HotPoint : CurrentMinimapMapData->HotPointInfos)
        {
            FStaticMapPin NewPin;
        	NewPin.Location = HotPoint.Location;
        	NewPin.Yaw = 0.0f;
        	NewPin.MapPinBrush = HotPoint.MapPinBrush;
            NewPin.IdentifyGuid = HotPoint.IdentifyGuid;
            NewPin.PinName = HotPoint.PinName;
            NewPin.PinDescription = HotPoint.PinDescription;
        	NewPin.CustomMinimapWidgetClass = HotPoint.CustomMinimapWidgetClass;
        	NewPin.CustomMainmapWidgetClass = HotPoint.CustomMainmapWidgetClass;
        	NewPin.CustomDatas = HotPoint.CustomDatas;
        	
            StaticMapPins.AddUnique(NewPin);
        }
        return CurrentMinimapMapData;
    }
    
    return nullptr;
}

FHotPointInfo UMinimapSubsystem::GetHotPointInfoFromGuid(FGuid Guid)
{
    if (CurrentMinimapMapData && Guid.IsValid())
    {
        auto Ptr = CurrentMinimapMapData->HotPointInfos.FindByPredicate([&] (const FHotPointInfo& HotPoint)
        {
           return HotPoint.IdentifyGuid == Guid;
        });

        if (Ptr)
        {
            return *Ptr;
        }
    }

    return FHotPointInfo();
}

FStaticMapPin UMinimapSubsystem::GetShownMinimapPin(FGuid Guid, bool& Success) const
{
    if (!Guid.IsValid())
    {
        Success = false;
        return FStaticMapPin();
    }
	
    auto NewStaticMapPins = StaticMapPins;
    
    for (auto RegisteredComp : MinimapComponentRegistry)
    {
        if (RegisteredComp->MinimapGuid.IsValid() && RegisteredComp->ShouldVisible())
        {
            auto StaticPtr = StaticMapPins.IndexOfByPredicate([&](const FStaticMapPin& Pin)
            {
               return Pin.IdentifyGuid == RegisteredComp->MinimapGuid; 
            });
            
            if (StaticPtr >= 0)
            {
                NewStaticMapPins[StaticPtr] = RegisteredComp->GetCurrentStaticMapPin();
            }
            else if (RegisteredComp->bIsIndividual)
            {
                NewStaticMapPins.Add(RegisteredComp->GetCurrentStaticMapPin());
            }
        }
    }
    
    auto StaticPtr = NewStaticMapPins.FindByPredicate([&](const FStaticMapPin& Pin)
    {
        return Pin.IdentifyGuid == Guid; 
    });

    if (StaticPtr)
    {
        Success = true;
        return *StaticPtr;
    }

    Success = false;
    return FStaticMapPin();
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
