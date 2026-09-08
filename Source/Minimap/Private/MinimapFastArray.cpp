// Fill out your copyright notice in the Description page of Project Settings.

#include "MinimapFastArray.h"

#include "MinimapMapData.h"
#include "MinimapSettings.h"
#include "Components/MinimapComponent.h"

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

TMap<FString, FMinimapIndices> UMinimapFastArrayLibrary::GetPoiMapping(const FPoiStateList& List)
{
	return List.PoiStateMap;
}

FMapPinStateEntry FMapPinStateEntry::GetCurrentState() const
{
	FMapPinStateEntry Copy = *this;
	Copy.UpdateTransform();

	return Copy;
}

void FMapPinStateEntry::UpdateTransform()
{
	if (ActorWeakPtr.IsValid())
	{
		Location = ActorWeakPtr->GetActorLocation();
		if (bHasYaw)
		{
			Yaw = ActorWeakPtr->GetActorRotation().Yaw;
		}
	}
}

int FMapPinStateList::AddMapPinState(const FMapPinStateEntry& InNewEntry)
{
	if (InNewEntry.Id.IsValid() && !QueryMapping.Contains(InNewEntry.Id))
	{
		FMapPinStateEntry NewEntry = InNewEntry;
		NewEntry.UpdateTransform();
		const auto Index = StateEntries.Add(NewEntry);
		QueryMapping.Add(NewEntry.Id, Index);
		BroadcastMapPinChange(NewEntry.Id, true);
		MarkItemDirty(NewEntry);
		return Index;
	}
	
	return *QueryMapping.Find(InNewEntry.Id);
}

bool FMapPinStateList::RemoveMapPinState(const FGuid Id)
{
	FMapPinStateEntry FindingEntry;
	FindingEntry.Id = Id;
	const auto RemovedANum = StateEntries.Remove(FindingEntry);
	BroadcastMapPinChange(Id, false);
	const auto RemovedMNum = QueryMapping.Remove(Id);
	MarkArrayDirty();
	return RemovedANum > 0 && RemovedMNum > 0;
}

bool FMapPinStateList::SetMapPinState(const FMapPinStateEntry& InNewEntry)
{
	if (const auto Found = QueryMapping.Find(InNewEntry.Id))
	{
		if (StateEntries.IsValidIndex(*Found))
		{
			StateEntries[*Found] = InNewEntry;
			return true;
		}
	}
	
	return false;
}

void FMapPinStateList::BroadcastMapPinChange(const FGuid Id, const bool AddOrRemove) const
{
	auto Sub = Cast<UMinimapSubsystem>(OwnerObject);
	if (!Sub)
	{
		if (OwnerObject && OwnerObject->GetWorld())
		{
			Sub = OwnerObject->GetWorld()->GetSubsystem<UMinimapSubsystem>();
		}
	}
	
	if (Sub)
	{
		if (AddOrRemove)
		{
			Sub->OnMapPinAddEvent.Broadcast(Id);
		}
		else
		{
			Sub->OnMapPinRemoveEvent.Broadcast(Id);
		}
	}
}

bool FMapPinStateList::GetCurrentState(int Index, FMapPinStateEntry& OutEntry) const
{
	if (StateEntries.IsValidIndex(Index))
	{
		OutEntry = StateEntries[Index].GetCurrentState();
		return true;
	}
	
	return false;
}

bool FMapPinStateList::GetCurrentState(FGuid Id, FMapPinStateEntry& OutEntry) const
{
	if (const auto FoundIndex = QueryMapping.Find(Id))
	{
		return GetCurrentState(*FoundIndex, OutEntry);
	}
	
	return false;
}

void FMapPinStateList::UpdateTransform(int Index)
{
	if (StateEntries.IsValidIndex(Index))
	{
		StateEntries[Index].UpdateTransform();
		MarkItemDirty(StateEntries[Index]);
	}
}

void FMapPinStateList::UpdateTransform(FGuid Id)
{
	if (const auto FoundIndex = QueryMapping.Find(Id))
	{
		UpdateTransform(*FoundIndex);
	}
}

void FMapPinStateList::UpdateAllTransforms()
{
	for (auto& Entry : StateEntries)
	{
		Entry.UpdateTransform();
		MarkItemDirty(Entry);
	}
}

void FMapPinStateList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (const auto Index : RemovedIndices)
	{
		BroadcastMapPinChange(StateEntries[Index].Id, false);
		QueryMapping.Remove(StateEntries[Index].Id);
	}
}

void FMapPinStateList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const auto Index : AddedIndices)
	{
		QueryMapping.Add(StateEntries[Index].Id, Index);
		BroadcastMapPinChange(StateEntries[Index].Id, true);
	}
}

void FMapPinStateList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	// Not trigger anything, id should be persistent and can not be modified!!!
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

bool UMinimapFastArrayLibrary::GetMapPinCurrentState(FMapPinStateList& List, FGuid Id, FMapPinStateEntry& OutEntry)
{
	return List.GetCurrentState(Id, OutEntry);
}

int UMinimapFastArrayLibrary::AddMapPin(FMapPinStateList& List, const FMapPinStateEntry& InEntry)
{
	return List.AddMapPinState(InEntry);
}

bool UMinimapFastArrayLibrary::RemoveMapPin(FMapPinStateList& List, const FGuid Id)
{
	return List.RemoveMapPinState(Id);
}
