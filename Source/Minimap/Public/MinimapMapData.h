// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MinimapStructs.h"
#include "Engine/DataAsset.h"
#include "MinimapMapData.generated.h"

struct FHotPointInfo;

/** Minimap quad tree node. */
USTRUCT(BlueprintType)
struct FSerializableQuadtreeNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FBox2D Bounds;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FGuid> Indices;

	UPROPERTY()
	int32 Children[4]; // 子节点索引 -1=无效

	FSerializableQuadtreeNode(): Bounds()
	{
		Children[0] = -1;
		Children[1] = -1;
		Children[2] = -1;
		Children[3] = -1;
	}
};

UCLASS(BlueprintType)
class MINIMAP_API UMinimapMapData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap")
	FString LevelName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	UTexture2D* MapTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	float MapSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	float TextureSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	FVector CaptureActorLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	TMap<FGuid, FHotPointInfo> HotPointInfos;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap", AdvancedDisplay)
	TArray<FSerializableQuadtreeNode> QuadtreeNodes;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap", AdvancedDisplay)
	TMap<FGameplayTag, FMinimapIndices> CategoryMap;

public:
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	TArray<FGuid> QueryBox(FBox2D Box);
	
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	TArray<FGuid> QueryRange(const FVector& Center, float Radius);

	void QueryRecursive(const TArray<FSerializableQuadtreeNode>& Nodes, int32 NodeIndex, const FBox2D& QueryBounds, TArray<FGuid>& OutResults);
	
#if WITH_EDITOR
	FMinimapIndices* FindOrCreateCategory(const FGameplayTag& CategoryTag);
	void BuildRecursive(TArray<FSerializableQuadtreeNode>& Nodes, const FBox2D& Bounds, const TArray<FGuid>& Indices, int32 Depth);
	void BuildHotPointsQuadTree();
#endif
	
	FBox2D GetWorldBounds() const;
};
