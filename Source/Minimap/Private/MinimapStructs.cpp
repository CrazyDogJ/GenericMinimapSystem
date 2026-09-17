// Fill out your copyright notice in the Description page of Project Settings.

#include "MinimapStructs.h"

#include "MinimapMapData.h"
#include "MinimapSettings.h"
#include "MinimapSubsystem.h"

#pragma region FPoiStateEntry
FPoiStateEntry::FPoiStateEntry(const FString& InLevelName, const FGuid InPoiIndex)
{
	const auto DefaultMinimapSettings = GetDefault<UMinimapSettings>();
	TArray<FString> Array;
	DefaultMinimapSettings->MapsInfos.GenerateKeyArray(Array);
	LevelIndex = Array.Find(InLevelName);
	PoiIndex = InPoiIndex;
}

FPoiStateEntry::FPoiStateEntry(const UMinimapMapData* MapData, const FGuid InPoiIndex)
{
	if (MapData)
	{
		FPoiStateEntry(MapData->LevelName, InPoiIndex);
	}
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
	if (LevelName.IsEmpty())
	{
		return;
	}

	if (!PoiStateMap.Find(LevelName))
	{
		PoiStateMap.Add(LevelName);
	}
	
	if (const auto Found = PoiStateMap.Find(LevelName))
	{
		if (Found->Add(PoiIndex))
		{
			auto NewEntry = FPoiStateEntry(LevelName, PoiIndex);
			PoiStateEntries.Add(NewEntry);
			MarkItemDirty(NewEntry);

			if (GetSubsystem())
			{
				GetSubsystem()->OnHotPointFoundEvent.Broadcast(LevelName, PoiIndex);
			}
		}
	}
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
	
	if (GetSubsystem())
	{
		GetSubsystem()->OnHotPointRemoveEvent.Broadcast(LevelName, PoiIndex);
	}
	
	if (Found->Remove(PoiIndex))
	{
		PoiStateEntries.Remove(FPoiStateEntry(LevelName, PoiIndex));
		MarkArrayDirty();

		// Remove empty level category.
		TryRemoveEmpty(LevelName);
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
	for (const auto Index : RemovedIndices)
	{
		const auto Entry = PoiStateEntries[Index];
		const auto LevelName = GetLevelName(Entry.LevelIndex);
		
		if (!PoiStateMap.Find(LevelName))
		{
			continue;
		}
		
		if (const auto Found = PoiStateMap.Find(LevelName))
		{
			if (GetSubsystem())
			{
				GetSubsystem()->OnHotPointRemoveEvent.Broadcast(LevelName, Entry.PoiIndex);
			}
			
			Found->Remove(Entry.PoiIndex);
			
			// Remove empty level category.
			TryRemoveEmpty(LevelName);
		}
	}
}

void FPoiStateList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const auto Index : AddedIndices)
	{
		const auto Entry = PoiStateEntries[Index];
		const auto LevelName = GetLevelName(Entry.LevelIndex);
		
		if (!PoiStateMap.Find(LevelName))
		{
			PoiStateMap.Add(LevelName);
		}
		
		if (const auto Found = PoiStateMap.Find(LevelName))
		{
			if (Found->Add(Entry.PoiIndex))
			{
				if (GetSubsystem())
				{
					GetSubsystem()->OnHotPointFoundEvent.Broadcast(LevelName, Entry.PoiIndex);
				}
			}
		}
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
	List = FPoiStateList(InitMapping);
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