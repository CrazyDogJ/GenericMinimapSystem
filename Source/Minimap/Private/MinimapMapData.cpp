// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapMapData.h"

#include "MinimapSettings.h"
#include "MinimapStructs.h"


TArray<FGuid> UMinimapMapData::QueryBox(FBox2D Box)
{
	TArray<FGuid> Results;
	if (HotPointInfos.Num() == 0) return Results;

	QueryRecursive(QuadtreeNodes, 0, Box, Results);
	return Results;
}

TArray<FGuid> UMinimapMapData::QueryRange(const FVector& Center, float Radius)
{
	TArray<FGuid> Results;
	if (HotPointInfos.Num() == 0) return Results;

	const FVector2D Center2D(Center.X, Center.Y);
	const FBox2D QueryBounds(
		FVector2D(Center2D.X - Radius,
		Center2D.Y - Radius),
		FVector2D(Center2D.X + Radius,
		Center2D.Y + Radius)
	);

	QueryRecursive(QuadtreeNodes, 0, QueryBounds, Results);
	return Results;
}

void UMinimapMapData::QueryRecursive(const TArray<FSerializableQuadtreeNode>& Nodes, int32 NodeIndex,
	const FBox2D& QueryBounds, TArray<FGuid>& OutResults)
{
	if (!Nodes.IsValidIndex(NodeIndex)) return;

	const FSerializableQuadtreeNode& Node = Nodes[NodeIndex];
	if (!Node.Bounds.Intersect(QueryBounds)) return;

	// 添加当前节点Index
	for (const FGuid& Index : Node.Indices)
	{
		if (const auto Found = HotPointInfos.Find(Index))
		{
			const FVector2D Loc = FVector2D(Found->Location);
			if (QueryBounds.IsInside(Loc))
			{
				OutResults.Add(Index);
			}
		}
	}

	// 递归子节点
	for (int32 ChildIndex : Node.Children)
	{
		if (ChildIndex != -1)
		{
			QueryRecursive(Nodes, ChildIndex, QueryBounds, OutResults);
		}
	}
}

#if WITH_EDITOR

FMinimapIndices* UMinimapMapData::FindOrCreateCategory(const FGameplayTag& CategoryTag)
{
	if (!CategoryTag.IsValid())
	{
		return nullptr;
	}
	
	if (const auto Found = CategoryMap.Find(CategoryTag))
	{
		return Found;
	}
	
	return &CategoryMap.Add(CategoryTag);
}

int32 UMinimapMapData::BuildRecursive(TArray<FSerializableQuadtreeNode>& Nodes, const FBox2D& Bounds,
	const TArray<FGuid>& Indices, int32 Depth)
{
	// Create new node
	int32 CurrentNodeIndex = Nodes.AddDefaulted();
	FSerializableQuadtreeNode& Node = Nodes[CurrentNodeIndex];
	Node.Bounds = Bounds;

	const auto Settings = GetDefault<UMinimapSettings>();
	
	// 1. 不分裂条件：Index数量 < 上限 或 已达最大深度
	if (Indices.Num() <= Settings->MaxPOIPerNode || Depth >= Settings->MaxDepth)
	{
		Node.Indices = Indices;
		return CurrentNodeIndex;
	}

	// 2. 需要分裂：计算四叉树中心点
	const float CenterX = (Bounds.Min.X + Bounds.Max.X) * 0.5f;
	const float CenterY = (Bounds.Min.Y + Bounds.Max.Y) * 0.5f;

	// 3. 定义四个子区域（左上、右上、左下、右下）
	FBox2D ChildBounds[4] = {
		FBox2D(FVector2D(Bounds.Min.X, Bounds.Min.Y), FVector2D(CenterX, CenterY)),
		FBox2D(FVector2D(CenterX, Bounds.Min.Y), FVector2D(Bounds.Max.X, CenterY)),
		FBox2D(FVector2D(Bounds.Min.X, CenterY), FVector2D(CenterX, Bounds.Max.Y)),
		FBox2D(FVector2D(CenterX, CenterY), FVector2D(Bounds.Max.X, Bounds.Max.Y)) 
	};

	// 4. 把Index分配到四个子区域
	TArray<FGuid> ChildIndices[4];
	for (const FGuid& Guid : Indices)
	{
		if (const auto* Found = HotPointInfos.Find(Guid))
		{
			const FVector2D Loc = FVector2D(Found->Location);

			for (int32 i = 0; i < 4; ++i)
			{
				if (ChildBounds[i].IsInside(Loc))
				{
					ChildIndices[i].Add(Guid);
					break;
				}
			}
		}
	}

	// 5. 递归构建四个子节点，并记录索引
	for (int32 i = 0; i < 4; ++i)
	{
		if (ChildIndices[i].Num() > 0)
		{
			int32 ChildNodeIndex = BuildRecursive(Nodes, ChildBounds[i], ChildIndices[i], Depth + 1);
			Node.Children[i] = ChildNodeIndex; // 记录子节点索引
		}
	}
	
	return CurrentNodeIndex;
}

void UMinimapMapData::BuildHotPointsQuadTree()
{
	CategoryMap.Empty();
	QuadtreeNodes.Empty();
	TArray<FGuid> Indices;
	for (const auto Itr : HotPointInfos)
	{
		// Update points.
		Indices.Add(Itr.Key);

		// Update category.
		if (Itr.Value.CategoryTag.IsValid())
		{
			const auto Category = FindOrCreateCategory(Itr.Value.CategoryTag);
			Category->Indices.Add(Itr.Value.IdentifyGuid);
		}
	}
	
	BuildRecursive(QuadtreeNodes, GetWorldBounds(), Indices, 0);
}
#endif

FBox2D UMinimapMapData::GetWorldBounds() const
{
	return FBox2D(FVector2D(CaptureActorLocation.X, CaptureActorLocation.Y),
		FVector2D(CaptureActorLocation.X + MapSize, CaptureActorLocation.Y + MapSize));
}
