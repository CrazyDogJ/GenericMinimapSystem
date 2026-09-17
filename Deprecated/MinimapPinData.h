// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"
#include "MinimapPinData.generated.h"

class UMinimapPinData;

USTRUCT(BlueprintType)
struct FMinimapPinDataEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	FMinimapPinDataEntry() {}
	FMinimapPinDataEntry(FGuid A, uint8 B, UMinimapPinData* C)
		: Id(A), Channel(B), PinDataPtr(C) {}
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGuid Id;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	uint8 Channel = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UMinimapPinData* PinDataPtr = nullptr;
	
	bool operator==(const FMinimapPinDataEntry& Other) const
	{
		return Id == Other.Id;
	}
	
	bool IsVisible(const TArray<uint8>& InChannels) const
	{
		return InChannels.Contains(Channel);
	}
};

USTRUCT(BlueprintType)
struct FMinimapPinDataList : public FFastArraySerializer
{
	GENERATED_BODY()
	
	DECLARE_DELEGATE_OneParam(FPinDataChange, TWeakObjectPtr<UMinimapPinData> PinDataPtr);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FMinimapPinDataEntry> MinimapPinDatas;
	
	int RemovePinData(const FMinimapPinDataEntry& InEntry);
	int AddPinData(const FMinimapPinDataEntry& InEntry);
	
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	
	FPinDataChange OnPinDataAdd;
	FPinDataChange OnPinDataRemove;
	
	void UpdatePinDatas();
	
	// Replication
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FMinimapPinDataEntry, FMinimapPinDataList>(MinimapPinDatas, DeltaParams, *this);
	}
};

template<> struct TStructOpsTypeTraits<FMinimapPinDataList> : public TStructOpsTypeTraitsBase2<FMinimapPinDataList>
{
	enum { WithNetDeltaSerializer = true };
};

UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class MINIMAP_API UMinimapPinData : public UObject
{
	GENERATED_BODY()
	
public:
	// Pin data actor weak pointer.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated)
	TWeakObjectPtr<AActor> PinDataActorPtr;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated)
	FVector_NetQuantize Location;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated)
	float Yaw = 0.0f;
	
	TObjectPtr<AActor> TryGetOwningActor() const;
	void UpdatePinData(TObjectPtr<AActor> InActor = nullptr);
	
	UFUNCTION()
	void OnRep_Channel();
	
protected:
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
	virtual bool ImplementsGetWorld() const override { return true; }
#endif
	
};
