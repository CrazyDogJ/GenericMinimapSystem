// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "StructUtils/InstancedStruct.h"
#include "MinimapFastArray.generated.h"

class UMapPinUserWidget;
class UMinimapComponent;
class UMinimapSubsystem;
class UMinimapMapData;

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

/** Map pin state entry for FMapPinStateList fast array. */
USTRUCT(BlueprintType)
struct FMapPinStateEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	/** Guid to identify */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGuid Id;
	
	/** Subscribed world component */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TWeakObjectPtr<const AActor> ActorWeakPtr;
	
	/** Back up static location when actor is disappeared */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector_NetQuantize100 StaticLocation;
	
	/** Net quantized location to replicate */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector_NetQuantize100 Location;
	
	/** Should rotate */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHasYaw = false;
	
	/** Yaw angle */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Yaw = 0.0f;
	
	/** Display brush */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSlateBrush Brush;
	
	/** Category tag for specific displaying. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTag CategoryTag;
	
	/** Should add to screen */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddToOverlay = false;
	
	/** Always show on minimap (not removed when out of bound) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAlwaysOnMinimap = false;
	
	/** Localizable name for this pin */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText PinName;
	
	/** Localizable description for this pin */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText PinDescription;
	
	/** The custom minimap widget class for this map pin */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UMapPinUserWidget> CustomMinimapWidgetClass;
	
	/** The custom main map widget class for this map pin */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UMapPinUserWidget> CustomMainmapWidgetClass;
	
	/** Additional map pin data to store in here to replicate */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FInstancedStruct CustomDatas;
	
	bool operator==(const FMapPinStateEntry& Other) const
	{
		return Id == Other.Id;
	}
	
	FMapPinStateEntry operator=(const FMapPinStateEntry& Other)
	{
		Id = Other.Id;
		ActorWeakPtr = Other.ActorWeakPtr;
		StaticLocation = Other.StaticLocation;
		Location = Other.Location;
		bHasYaw = Other.bHasYaw;
		Yaw = Other.Yaw;
		Brush = Other.Brush;
		CategoryTag = Other.CategoryTag;
		bAddToOverlay = Other.bAddToOverlay;
		bAlwaysOnMinimap = Other.bAlwaysOnMinimap;
		PinName = Other.PinName;
		PinDescription = Other.PinDescription;
		CustomMainmapWidgetClass = Other.CustomMainmapWidgetClass;
		CustomMinimapWidgetClass = Other.CustomMinimapWidgetClass;
		CustomDatas = Other.CustomDatas;
		
		return *this;
	}
	
	// Can call in client.
	FMapPinStateEntry GetCurrentState() const;
	
	// Return whether transform needs update.
	void UpdateTransform();
};

/** 
 * Used to replicate map pin state on minimap global. 
 * This is a back-up struct data for actors that out of replicating range,
 * and it's not relevant anymore.
 */
USTRUCT(BlueprintType)
struct FMapPinStateList : public FFastArraySerializer
{
	GENERATED_BODY()
	
	FMapPinStateList() {}
	FMapPinStateList(const UObject* InOwnerObject) : OwnerObject(InOwnerObject) {}
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, NotReplicated)
	const UObject* OwnerObject = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FMapPinStateEntry> StateEntries;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, NotReplicated)
	TMap<FGuid, int> QueryMapping;
	
	// If id is already exist, return the exist index. Call in authority.
	int AddMapPinState(const FMapPinStateEntry& InNewEntry);
	
	// Return true if we remove the map pin successfully. Call in authority.
	bool RemoveMapPinState(FGuid Id);
	
	bool SetMapPinState(const FMapPinStateEntry& InNewEntry);
	
	void BroadcastMapPinChange(FGuid Id, bool AddOrRemove) const;
	
	bool GetCurrentState(int Index, FMapPinStateEntry& OutEntry) const;
	bool GetCurrentState(FGuid Id, FMapPinStateEntry& OutEntry) const;
	
	void UpdateTransform(int Index);
	void UpdateTransform(FGuid Id);
	void UpdateAllTransforms();
	
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	
	// Replication
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FMapPinStateEntry, FMapPinStateList>(StateEntries, DeltaParams, *this);
	}
};

template<> struct TStructOpsTypeTraits<FMapPinStateList> : public TStructOpsTypeTraitsBase2<FMapPinStateList>
{
	enum { WithNetDeltaSerializer = true };
};

/** Poi map pin info (Hot point info is deprecated)*/
USTRUCT(BlueprintType)
struct FPoiInfo : public FMapPinStateEntry
{
	GENERATED_BODY()
	
	// Used for localization text.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HotPointId;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsTeleportPoint = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = bIsTeleportPoint))
	FTransform TeleportTransform;
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
	
	UFUNCTION(BlueprintPure, Category = "Minimap|Map Pins")
	static bool GetMapPinCurrentState(UPARAM(ref)FMapPinStateList& List, FGuid Id, FMapPinStateEntry& OutEntry);
	
	UFUNCTION(BlueprintCallable, Category = "Minimap|Map Pins")
	static int AddMapPin(UPARAM(ref)FMapPinStateList& List, const FMapPinStateEntry& InEntry);
	
	UFUNCTION(BlueprintCallable, Category = "Minimap|Map Pins")
	static bool RemoveMapPin(UPARAM(ref)FMapPinStateList& List, FGuid Id);
};
