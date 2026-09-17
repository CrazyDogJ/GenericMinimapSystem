// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapSubsystem.h"

#include "MinimapMapData.h"
#include "MinimapSettings.h"
#include "Components/MinimapGlobal.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"

void UMinimapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMinimapSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UMinimapSubsystem::SetLocalMinimapMapData(UMinimapMapData* InLocalMinimapMapData)
{
    if (LocalMinimapMapData != InLocalMinimapMapData)
    {
        LocalMinimapMapData = InLocalMinimapMapData;
        OnLocalMapDataChangeEvent.Broadcast(LocalMinimapMapData);
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
        if (CurrentMinimapMapData->MapTexture)
        {
            CurrentMinimapMapData->MapTexture->UpdateResource();
        }
    	
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

bool UMinimapSubsystem::HasAuthority() const
{
    if (GetWorld()->GetAuthGameMode())
    {
        return true;
    }
    
    return false;
}
