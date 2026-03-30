// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapSettings.h"
#include "MinimapSubsystem.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MinimapFastArray.generated.h"

class UMinimapSubsystem;
class UMinimapMapData;

USTRUCT(BlueprintType)
struct FPoiStateEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	FPoiStateEntry() {}
	
	FPoiStateEntry(const int InLevelIndex, const FGuid InPoiIndex)
		: LevelIndex(InLevelIndex), PoiIndex(InPoiIndex)
	{}

	FPoiStateEntry(const FString& InLevelName, const FGuid InPoiIndex)
	{
		const auto DefaultMinimapSettings = GetDefault<UMinimapSettings>();
		TArray<FString> Array;
		DefaultMinimapSettings->MapsInfos.GenerateKeyArray(Array);
		LevelIndex = Array.Find(InLevelName);
		PoiIndex = InPoiIndex;
	}

	FPoiStateEntry(const UMinimapMapData* MapData, const FGuid InPoiIndex)
	{
		if (MapData)
		{
			FPoiStateEntry(MapData->LevelName, InPoiIndex);
		}
	}

	bool IsValid() const
	{
		return LevelIndex >= 0 && PoiIndex.IsValid();
	}
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	int LevelIndex = -1;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FGuid PoiIndex;
};

USTRUCT(BlueprintType)
struct FPoiStateList : public FFastArraySerializer
{
	GENERATED_BODY()

public:
	FPoiStateList() {}
	explicit FPoiStateList(const TMap<FString, FMinimapIndices>& InitMapping);
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FPoiStateEntry> PoiStateEntries;

	UPROPERTY(NotReplicated)
	UObject* WorldContextObject = nullptr;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, NotReplicated)
	TMap<FString, FMinimapIndices> PoiStateMap;

	TMap<FString, FMinimapIndices> AppendOther(const FPoiStateList& Other) const;
	
	UWorld* GetWorld() const
	{
		if (WorldContextObject)
		{
			return WorldContextObject->GetWorld();
		}

		return nullptr;
	}

	UMinimapSubsystem* GetSubsystem() const
	{
		if (const auto World = GetWorld())
		{
			return  World->GetSubsystem<UMinimapSubsystem>();
		}

		return nullptr;
	}
	
	static FString GetLevelName(int32 LevelIndex);
	void AddPoi(const FString& LevelName, const FGuid& PoiIndex);
	bool IsPoiFound(const FString& LevelName, const FGuid& PoiIndex) const;

	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	
	// Replication
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FPoiStateEntry, FPoiStateList>(PoiStateEntries, DeltaParams, *this);
	}
};

template<> struct TStructOpsTypeTraits<FPoiStateList> : public TStructOpsTypeTraitsBase2<FPoiStateList>
{
	enum { WithNetDeltaSerializer = true };
};

UCLASS()
class UMinimapFastArrayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static void LoadPoi(UPARAM(ref)FPoiStateList& List, const TMap<FString, FMinimapIndices>& InitMapping);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static void AddPoi(UPARAM(ref)FPoiStateList& List, const FString& LevelName, const FGuid& PoiIndex);

	UFUNCTION(BlueprintPure)
	static TMap<FString, FMinimapIndices> GetPoiMapping(const FPoiStateList& List);
	
	UFUNCTION(BlueprintPure)
	static bool IsPoiFound(const FPoiStateList& List, const FString& LevelName, const FGuid& PoiIndex);
};
