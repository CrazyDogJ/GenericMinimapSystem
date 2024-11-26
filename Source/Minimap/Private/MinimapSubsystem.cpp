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
    
    const auto Ptr = MinimapComponentRegistry.FindByPredicate([&](const TObjectPtr<UMinimapComponent>& Comp)
    {
        return Comp->MinimapGuid == Guid;
    });

    if (Ptr)
    {
        auto Comp = *Ptr;
        return FStaticMapPin(Comp->GetOwner()->GetActorLocation(), Comp->GetOwner()->GetActorRotation().Yaw,
                             Comp->PinSlateBrush, Comp->bRotate, Comp->bAddToOverlay);
    }
    
    auto StaticPtr = StaticMapPins.FindByPredicate([&](const FStaticMapPin& Pin)
    {
       return Pin.IdentifyGuid == Guid; 
    });

    if (StaticPtr)
    {
        return *StaticPtr;
    }
    
    return FStaticMapPin();
}

FGuid UMinimapSubsystem::AddStaticLocationPin(FVector Location, float Yaw, FSlateBrush PinSlateBrush, bool bHasRotation, bool bAddToOverlay)
{
    auto MapPin = FStaticMapPin(Location, Yaw, PinSlateBrush, bHasRotation, bAddToOverlay);
    MapPin.IdentifyGuid = FGuid::NewGuid();
    StaticMapPins.Add(MapPin);
    OnStaticRegistered.Broadcast(MapPin);
    return MapPin.IdentifyGuid;
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
                false);
            NewPin.IdentifyGuid = HotPoint.HotPointUniqueID;
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
    
    // Dynamic update
    TMap<FGuid, FVector> TempMap;
    for (auto Comp : MinimapComponentRegistry)
    {
        if (CurrentLocalPlayerActor != Comp->GetOwner())
        {
            TempMap.Add(Comp->MinimapGuid, Comp->GetOwner()->GetActorLocation());
        }
    }

    for (auto Pin : StaticMapPins)
    {
        TempMap.Add(Pin.IdentifyGuid, Pin.Location);
    }
    
    for (auto MapPin : TempMap)
    {
        if (FVector::Dist2D(CurrentLocalPlayerActor->GetActorLocation(), MapPin.Value) <= MinimapRadius / 2)
        {
            AddMinimapPin(MapPin.Key);
        }
        else
        {
            RemoveMinimapPin(MapPin.Key);
        }
    }
}

TStatId UMinimapSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMinimapSubsystem, STATGROUP_Tickables);
}
