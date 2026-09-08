// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapSubsystem.h"

#include "MinimapMapData.h"
#include "MinimapSettings.h"
#include "Components/MinimapGlobal.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"

FMapPinStateList UMinimapSubsystem::GetLocalPinStateList() const
{
    return LocalPinStateList;
}

FMapPinStateList UMinimapSubsystem::GetGlobalPinStateList() const
{
    if (const auto GS = GetMinimapGlobal())
    {
        return GS->PinStateList;
    }
    
    return FMapPinStateList();
}

FGuid UMinimapSubsystem::AddStaticMapPin(const FMapPinStateEntry& InEntry)
{
    auto Copy = InEntry;
    if (!Copy.Id.IsValid())
    {
        Copy.Id = FGuid::NewGuid();
    }
    
    LocalPinStateList.AddMapPinState(Copy);
    return Copy.Id;
}

void UMinimapSubsystem::RemoveStaticMapPin(FGuid MapPinGuid)
{
    LocalPinStateList.RemoveMapPinState(MapPinGuid);
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

bool UMinimapSubsystem::GetHotPointInfoFromGuid(FGuid Guid, FPoiInfo& OutInfo)
{
    if (CurrentMinimapMapData && Guid.IsValid())
    {
        if (const auto Ptr = CurrentMinimapMapData->HotPointInfos.Find(Guid))
        {
            OutInfo = *Ptr;
            return true;
        }
    }

    OutInfo = FPoiInfo();
    return false;
}

UMinimapGlobal* UMinimapSubsystem::GetMinimapGlobal() const
{
    if (const auto World = GetWorld())
    {
        if (const auto GS = World->GetGameState())
        {
            return GS->GetComponentByClass<UMinimapGlobal>();
        }
    }
    
    return nullptr;
}

bool UMinimapSubsystem::GetMapPinCurrentState(const FGuid Id, FMapPinStateEntry& OutEntry)
{
    if (const auto MG = GetMinimapGlobal())
    {
        if (MG->PinStateList.GetCurrentState(Id, OutEntry))
        {
            return true;
        }
    }
    
    return LocalPinStateList.GetCurrentState(Id, OutEntry);
}

void UMinimapSubsystem::SetMapPinCurrentState(const FMapPinStateEntry& InEntry)
{
    LocalPinStateList.SetMapPinState(InEntry);
}
