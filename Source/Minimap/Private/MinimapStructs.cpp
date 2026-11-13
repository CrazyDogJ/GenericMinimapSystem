// Fill out your copyright notice in the Description page of Project Settings.

#include "MinimapStructs.h"

#include "MinimapMapData.h"
#include "Engine/GameInstance.h"
#include "MinimapSettings.h"
#include "MinimapSubsystem.h"
#include "Components/MinimapPlayerComponent.h"
#include "GameFramework/PlayerController.h"
#include "Serialization/ArchiveProxy.h"

bool FMinimapSavedTempPin::Serialize(FArchive& Ar)
{
	if (!Ar.IsSaveGame()) return false;
	// All fields in this save record are persistent, including Slate's nested brush fields.
	struct FPinArchive : FArchiveProxy
	{
		explicit FPinArchive(FArchive& Inner) : FArchiveProxy(Inner) { ArIsSaveGame = false; }
	} PinArchive(Ar);
	StaticStruct()->SerializeTaggedProperties(PinArchive, reinterpret_cast<uint8*>(this), StaticStruct(), nullptr);
	return true;
}

namespace
{
void NotifyPoiChange(FPoiStateList& List, const FString& Level, FGuid Id, bool bAdded)
{
	auto* Sub = List.GetSubsystem();
	if (!Sub) return;
	const auto* Player = Cast<UMinimapPlayerComponent>(List.WorldContextObject);
	const auto* PC = Player ? Cast<APlayerController>(Player->GetOwner()) : nullptr;
	if (Player && (!PC || !PC->IsLocalPlayerController())) return;
	Sub->OnPoiChanged.Broadcast(PC, Level, Id);

	// The original Blueprint events describe the first local player's combined discoveries.
	const auto* Instance = List.GetWorld()->GetGameInstance();
	auto* Viewer = Instance ? Instance->GetFirstLocalPlayerController() : nullptr;
	if (PC && Viewer != PC) return;
	const auto* Global = Sub->GetMinimapGlobal();
	const auto* Local = Viewer ? Viewer->GetComponentByClass<UMinimapPlayerComponent>() : nullptr;
	const FPoiStateList* Other = Player ? (Global ? &Global->PoiStateList : nullptr) :
		(Local ? &Local->PersonalPoiStateList : nullptr);
	if (Other && Other->IsPoiFound(Level, Id)) return;
	if (bAdded) Sub->OnHotPointFoundEvent.Broadcast(Level, Id);
	else Sub->OnHotPointRemoveEvent.Broadcast(Level, Id);
}
}

#pragma region FPoiStateEntry
FPoiStateEntry::FPoiStateEntry(const FString& InLevelName, const FGuid InPoiIndex)
: LevelName(InLevelName), PoiIndex(InPoiIndex)
{
}

FPoiStateEntry::FPoiStateEntry(const UMinimapMapData* MapData, const FGuid InPoiIndex)
	: FPoiStateEntry(MapData ? MapData->LevelName : FString(), InPoiIndex)
{
}

FString FPoiStateEntry::ResolveLevelName() const
{
	return LevelName.IsEmpty() ? FPoiStateList::GetLevelName(LevelIndex) : LevelName;
}
#pragma endregion FPoiStateEntry
#pragma region FPoiStateList
FPoiStateList::FPoiStateList(const TMap<FString, FMinimapIndices>& InitMapping)
{
	for (const auto Itr : InitMapping)
	{
		for (const auto ItrIndex : Itr.Value.Indices)
		{
			AddPoi(Itr.Key, ItrIndex);
		}
	}
}

TMap<FString, FMinimapIndices> FPoiStateList::AppendOther(const FPoiStateList& Other) const
{
	auto Copy = PoiStateMap;
	for (const auto Itr : Other.PoiStateMap)
	{
		if (const auto Found = Copy.Find(Itr.Key))
		{
			Found->Indices.Append(Itr.Value.Indices);
		}
		else
		{
			Copy.Add(Itr.Key, Itr.Value);
		}
	}
	
	return Copy;
}

UWorld* FPoiStateList::GetWorld() const
{
	if (WorldContextObject)
	{
		return WorldContextObject->GetWorld();
	}

	return nullptr;
}

UMinimapSubsystem* FPoiStateList::GetSubsystem() const
{
	if (const auto World = GetWorld())
	{
		return  World->GetSubsystem<UMinimapSubsystem>();
	}

	return nullptr;
}

FString FPoiStateList::GetLevelName(int32 LevelIndex)
{
	const auto DefaultMinimapSettings = GetDefault<UMinimapSettings>();
	TArray<FString> Array;
	DefaultMinimapSettings->MapsInfos.GenerateKeyArray(Array);
	if (Array.IsValidIndex(LevelIndex))
	{
		return Array[LevelIndex];
	}

	return FString();
}

void FPoiStateList::AddPoi(const FString& LevelName, const FGuid& PoiIndex)
{
	if (LevelName.IsEmpty() || !PoiIndex.IsValid() || !PoiStateMap.FindOrAdd(LevelName).Add(PoiIndex)) return;
	const int32 Index = PoiStateEntries.Emplace(LevelName, PoiIndex);
	MarkItemDirty(PoiStateEntries[Index]);
	NotifyPoiChange(*this, LevelName, PoiIndex, true);
}

void FPoiStateList::ReplaceWith(const TMap<FString, FMinimapIndices>& Mapping)
{
	const auto Previous = PoiStateMap;
	for (const auto& Pair : Previous)
	{
		const auto* Keep = Mapping.Find(Pair.Key);
		for (const FGuid& Id : Pair.Value.Indices)
			if (!Keep || !Keep->Find(Id)) RemovePoi(Pair.Key, Id);
	}
	for (const auto& Pair : Mapping)
		for (const FGuid& Id : Pair.Value.Indices) AddPoi(Pair.Key, Id);
}

void FPoiStateList::RemovePoi(const FString& LevelName, const FGuid& PoiIndex)
{
	if (LevelName.IsEmpty())
	{
		return;
	}

	const auto Found = PoiStateMap.Find(LevelName);
	if (!Found)
	{
		return;
	}
	
	if (Found->Remove(PoiIndex))
	{
		PoiStateEntries.Remove(FPoiStateEntry(LevelName, PoiIndex));
		MarkArrayDirty();

		// Remove empty level category.
		TryRemoveEmpty(LevelName);
		// 修复：事件可修改 PoiStateMap，完成删除后再广播。
		NotifyPoiChange(*this, LevelName, PoiIndex, false);
	}
}

void FPoiStateList::TryRemoveEmpty(const FString& LevelName)
{
	const auto Found = PoiStateMap.Find(LevelName);
	if (!Found)
	{
		return;
	}
	
	if (Found->Indices.IsEmpty())
	{
		PoiStateMap.Remove(LevelName);
	}
}

bool FPoiStateList::IsPoiFound(const FString& LevelName, const FGuid& PoiIndex) const
{
	if (const auto Found = PoiStateMap.Find(LevelName))
	{
		return Found->Indices.Contains(PoiIndex);
	}

	return false;
}

void FPoiStateList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		const auto Entry = PoiStateEntries[Index];
		const FString Level = Entry.ResolveLevelName();
		auto* Found = PoiStateMap.Find(Level);
		if (Found && Found->Remove(Entry.PoiIndex))
		{
			TryRemoveEmpty(Level);
			NotifyPoiChange(*this, Level, Entry.PoiIndex, false);
		}
	}
}

void FPoiStateList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		const auto Entry = PoiStateEntries[Index];
		const FString Level = Entry.ResolveLevelName();
		if (!Level.IsEmpty() && Entry.PoiIndex.IsValid() && PoiStateMap.FindOrAdd(Level).Add(Entry.PoiIndex))
			NotifyPoiChange(*this, Level, Entry.PoiIndex, true);
	}
}
#pragma endregion FPoiStateList
#pragma region UMinimapFastArrayLibrary
TMap<FString, FMinimapIndices> UMinimapFastArrayLibrary::GetPoiMapping(const FPoiStateList& List)
{
	return List.PoiStateMap;
}

void UMinimapFastArrayLibrary::LoadPoi(FPoiStateList& List, const TMap<FString, FMinimapIndices>& InitMapping)
{
	List.ReplaceWith(InitMapping);
}

void UMinimapFastArrayLibrary::AddPoi(FPoiStateList& List, const FString& LevelName, const FGuid& PoiIndex)
{
	List.AddPoi(LevelName, PoiIndex);
}

bool UMinimapFastArrayLibrary::IsPoiFound(const FPoiStateList& List, const FString& LevelName, const FGuid& PoiIndex)
{
	return List.IsPoiFound(LevelName, PoiIndex);
}
#pragma endregion UMinimapFastArrayLibrary
