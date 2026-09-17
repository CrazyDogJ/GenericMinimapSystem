// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "StructUtils/InstancedStruct.h"
#include "MinimapStructs.generated.h"

class UMinimapSubsystem;
class UMapPinUserWidget;
class UMinimapMapData;

USTRUCT(BlueprintType)
struct FMinimapPinData
{
	GENERATED_BODY()
	
	// Used to calculate transform.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TWeakObjectPtr<USceneComponent> AttachedComponent;
	
	// Net replicated location.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector_NetQuantize Location = FVector_NetQuantize();
	
	// Has rotation on map pin.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHasYaw = false;
	
	// Yaw rotation.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Yaw = 0.0f;
	
	// Text name.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText DisplayName = FText();
	
	// Text Description.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText DisplayDescription = FText();
	
	// Display slate brush.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSlateBrush Brush = FSlateBrush();
	
	// Category tag for hide category pins feature.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTag CategoryTag = FGameplayTag();
	
	// Should add to screen overlay.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddToOverlay = false;
	
	// Should this pin always show on minimap no matter it is over radius.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAlwaysOnMinimap = false;
	
	// The custom minimap widget class for this map pin.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, NotReplicated)
	TSubclassOf<UMapPinUserWidget> CustomMinimapWidgetClass;
	
	// The custom main map widget class for this map pin.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, NotReplicated)
	TSubclassOf<UMapPinUserWidget> CustomMainmapWidgetClass;
	
	// Additional datas to store to display.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FInstancedStruct CustomDatas;
};

/** Poi map pin info (Hot point info is deprecated)*/
USTRUCT(BlueprintType)
struct FPoiInfo : public FMinimapPinData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGuid HotPointId;
	
	// Used for localization text.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HotPointStringId;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsTeleportPoint = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = bIsTeleportPoint))
	FTransform TeleportTransform;
};

/** 
 * Minimap indices container struct. 
 * Used to make poi category for displaying. 
 * Or used to make poi found state fast array.
 */
USTRUCT(BlueprintType)
struct FMinimapIndices
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	TSet<FGuid> Indices;

	bool Find(const FGuid& InIndex) const
	{
		return Indices.Contains(InIndex);
	}
	
	bool Add(const FGuid& InIndex)
	{
		if (InIndex.IsValid())
		{
			if (Find(InIndex))
			{
				return false;
			}
			
			Indices.Add(InIndex);
			return true;
		}

		return false;
	}
	
	bool Remove(const FGuid& InIndex)
	{
		if (InIndex.IsValid())
		{
			return Indices.Remove(InIndex) > 0;
		}

		return false;
	}
};

/** 
 * Poi found save data entry. Used to record player's finding history. 
 * LevelIndex represent the map index in project settings.
 * PoiIndex represent found poi in map data asset.
 */
USTRUCT(BlueprintType)
struct FPoiStateEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	FPoiStateEntry() {}
	
	FPoiStateEntry(const int InLevelIndex, const FGuid InPoiIndex)
		: LevelIndex(InLevelIndex), PoiIndex(InPoiIndex)
	{}

	FPoiStateEntry(const FString& InLevelName, const FGuid InPoiIndex);

	FPoiStateEntry(const UMinimapMapData* MapData, const FGuid InPoiIndex);

	bool IsValid() const
	{
		return LevelIndex >= 0 && PoiIndex.IsValid();
	}
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	int LevelIndex = -1;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FGuid PoiIndex;
	
	bool operator==(const FPoiStateEntry& Other) const
	{
		return LevelIndex == Other.LevelIndex && PoiIndex == Other.PoiIndex;
	}
};

/** Poi found save data list. Used to record player's finding history. */
USTRUCT(BlueprintType)
struct FPoiStateList : public FFastArraySerializer
{
	GENERATED_BODY()

public:
	FPoiStateList() {}
	explicit FPoiStateList(const TMap<FString, FMinimapIndices>& InitMapping);
	
	UPROPERTY(NotReplicated)
	UObject* WorldContextObject = nullptr;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FPoiStateEntry> PoiStateEntries;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, NotReplicated)
	TMap<FString, FMinimapIndices> PoiStateMap;

	/** Used to append found record to other list struct. Like personal and global. */
	TMap<FString, FMinimapIndices> AppendOther(const FPoiStateList& Other) const;
	
	UWorld* GetWorld() const;

	UMinimapSubsystem* GetSubsystem() const;
	
	static FString GetLevelName(int32 LevelIndex);
	void AddPoi(const FString& LevelName, const FGuid& PoiIndex);
	void RemovePoi(const FString& LevelName, const FGuid& PoiIndex);
	void TryRemoveEmpty(const FString& LevelName);
	bool IsPoiFound(const FString& LevelName, const FGuid& PoiIndex) const;

	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
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
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Minimap|POI")
	static void LoadPoi(UPARAM(ref)FPoiStateList& List, const TMap<FString, FMinimapIndices>& InitMapping);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Minimap|POI")
	static void AddPoi(UPARAM(ref)FPoiStateList& List, const FString& LevelName, const FGuid& PoiIndex);

	UFUNCTION(BlueprintPure, Category = "Minimap|POI")
	static TMap<FString, FMinimapIndices> GetPoiMapping(const FPoiStateList& List);
	
	UFUNCTION(BlueprintPure, Category = "Minimap|POI")
	static bool IsPoiFound(const FPoiStateList& List, const FString& LevelName, const FGuid& PoiIndex);
};
