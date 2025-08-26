// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapSubsystem.h"

#include "MinimapSettings.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

void UMinimapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMinimapSubsystem::Deinitialize()
{
    MinimapComponentRegistry.Empty();
    StaticMapPins.Empty();
    ShownMapPinsGuids.Empty();

    Super::Deinitialize();
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

FStaticMapPin UMinimapSubsystem::GetShownMinimapPin(FGuid Guid) const
{
    if (!Guid.IsValid())
    {
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
        return *StaticPtr;
    }
    
    return FStaticMapPin();
}

FGuid UMinimapSubsystem::AddStaticLocationPin(FStaticMapPin InPin)
{
    if (!InPin.IdentifyGuid.IsValid())
    {
        InPin.IdentifyGuid = FGuid::NewGuid();
    }
    if (InPin.bAlwaysOnMinimap)
    {
        AddMinimapPin(InPin.IdentifyGuid);
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
    RemoveMinimapPin(MapPinGuid);
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
        for (auto HotPoint : CurrentMinimapMapData->HotPointInfos)
        {
            FStaticMapPin NewPin = FStaticMapPin(
                HotPoint.Location,
                0.0f,
                UWidgetBlueprintLibrary::MakeBrushFromTexture(HotPoint.HotPointIcon, 32, 32),
                false,
                false,
                false);
            NewPin.IdentifyGuid = HotPoint.HotPointUniqueID;
            NewPin.PinName = HotPoint.HotPointName;
            NewPin.PinDescription = HotPoint.HotPointDescription;
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
           return HotPoint.HotPointUniqueID == Guid;
        });

        if (Ptr)
        {
            return *Ptr;
        }
    }

    return FHotPointInfo();
}

void UMinimapSubsystem::SetupLocalPlayer(AActor* LocalPlayerPawn)
{
    CurrentLocalPlayerActor = LocalPlayerPawn;
}

void UMinimapSubsystem::SetMinimapRadius(float Radius)
{
    MinimapRadius = Radius;
}

void UMinimapSubsystem::AddMinimapPin(FGuid Guid)
{
    if (ShownMapPinsGuids.Find(Guid) < 0)
    {
        ShownMapPinsGuids.Add(Guid);
        OnMapPinShowOnMinimap.Broadcast(Guid);
    }
}

void UMinimapSubsystem::RemoveMinimapPin(FGuid Guid)
{
    if (ShownMapPinsGuids.Find(Guid) >= 0)
    {
        OnMapPinHideOnMinimap.Broadcast(Guid);
        ShownMapPinsGuids.Remove(Guid);
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
        RemoveMinimapPin(Component->MinimapGuid);
        OnComponentUnregistered.Broadcast(Component);
        MinimapComponentRegistry.Remove(Component);
    }
}

void UMinimapSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!CurrentLocalPlayerActor)
    {
        return;
    }
    
    // Add pins guid and add always show pin
    TArray<FGuid> MapPinsGuidArray;
    for (auto Comp : MinimapComponentRegistry)
    {
        // ignore not visible component.
        if (!Comp->ShouldVisible())
        {
            continue;
        }
        
        if (!Comp->bAlwaysShow)
        {
            MapPinsGuidArray.AddUnique(Comp->MinimapGuid);
        }
        else if (Comp->bIsIndividual)
        {
            AddMinimapPin(Comp->MinimapGuid);
        }
    }
    for (auto Pin : StaticMapPins)
    {
        if (!Pin.bAlwaysOnMinimap)
        {
            MapPinsGuidArray.AddUnique(Pin.IdentifyGuid);
        }
        else
        {
            AddMinimapPin(Pin.IdentifyGuid);
        }
    }
    
    // Update visible
    for (auto MapPin : MapPinsGuidArray)
    {
        if (FVector::Dist2D(CurrentLocalPlayerActor->GetActorLocation(), GetShownMinimapPin(MapPin).Location) <= MinimapRadius / 2)
        {
            AddMinimapPin(MapPin);
        }
        else
        {
            RemoveMinimapPin(MapPin);
        }
    }
}

TStatId UMinimapSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMinimapSubsystem, STATGROUP_Tickables);
}
