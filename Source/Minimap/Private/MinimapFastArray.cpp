// Fill out your copyright notice in the Description page of Project Settings.

#include "MinimapFastArray.h"

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

bool FPoiStateList::IsPoiFound(const FString& LevelName, const FGuid& PoiIndex) const
{
	if (const auto Found = PoiStateMap.Find(LevelName))
	{
		return Found->Indices.Contains(PoiIndex);
	}

	return false;
}

void FPoiStateList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const auto Entry : PoiStateEntries)
	{
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
