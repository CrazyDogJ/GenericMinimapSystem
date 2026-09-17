// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapPinData.h"

#include "Net/UnrealNetwork.h"

int FMinimapPinDataList::RemovePinData(const FMinimapPinDataEntry& InEntry)
{
	if (InEntry.PinDataPtr)
	{
		OnPinDataRemove.ExecuteIfBound(InEntry.PinDataPtr);
		const auto Result = MinimapPinDatas.Remove(InEntry);
		MarkArrayDirty();
		return Result;
	}
	
	return 0;
}

int FMinimapPinDataList::AddPinData(const FMinimapPinDataEntry& InEntry)
{
	if (InEntry.PinDataPtr)
	{
		FMinimapPinDataEntry NewEntry = InEntry;
		NewEntry.PinDataPtr->UpdatePinData();
		const auto Result = MinimapPinDatas.Add(NewEntry);
		OnPinDataAdd.ExecuteIfBound(NewEntry.PinDataPtr);
		MarkItemDirty(NewEntry);
		return Result;
	}

	return -1;
}

void FMinimapPinDataList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (const auto Index : RemovedIndices)
	{
		OnPinDataRemove.ExecuteIfBound(MinimapPinDatas[Index].PinDataPtr);
	}
}

void FMinimapPinDataList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const auto Index : AddedIndices)
	{
		OnPinDataAdd.ExecuteIfBound(MinimapPinDatas[Index].PinDataPtr);
	}
}

void FMinimapPinDataList::UpdatePinDatas()
{
	for (const auto Entry : MinimapPinDatas)
	{
		if (Entry.PinDataPtr)
		{
			Entry.PinDataPtr->UpdatePinData();
		}
	}
}

TObjectPtr<AActor> UMinimapPinData::TryGetOwningActor() const
{
	if (PinDataActorPtr.IsValid())
	{
		return PinDataActorPtr.Get();
	}
	
	return nullptr;
}

void UMinimapPinData::UpdatePinData(TObjectPtr<AActor> InActor)
{
	if (InActor)
	{
		Location = InActor->GetActorLocation();
		Yaw = InActor->GetActorRotation().Yaw;
	}
	else if (const auto Actor = TryGetOwningActor())
	{
		Location = Actor->GetActorLocation();
		Yaw = Actor->GetActorRotation().Yaw;
	}
}

void UMinimapPinData::OnRep_Channel()
{
}

void UMinimapPinData::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		TArray<class FLifetimeProperty> BP_Props;
		BPClass->GetLifetimeBlueprintReplicationList(BP_Props);
		for (auto& Itr : BP_Props)
		{
			Itr.bIsPushBased = true;
		}
		
		OutLifetimeProps.Append(BP_Props);
	}
	
	DOREPLIFETIME(ThisClass, PinDataActorPtr);
	DOREPLIFETIME(ThisClass, Location);
	DOREPLIFETIME(ThisClass, Yaw);
}
