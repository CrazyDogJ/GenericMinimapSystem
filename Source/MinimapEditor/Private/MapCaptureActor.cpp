// Fill out your copyright notice in the Description page of Project Settings.


#include "MapCaptureActor.h"

#include "Editor.h"
#include "Actors/MapHotPointActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "MinimapSettings.h"
#include "TextureResource.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Logging/MessageLog.h"
#include "UObject/SavePackage.h"

// Sets default values
AMapCaptureActor::AMapCaptureActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	SetRootComponent(SceneComponent);
	Capture2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture2D"));
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	Capture2D->ProjectionType = ECameraProjectionMode::Orthographic;
	Capture2D->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, -90, 0)));
	Capture2D->bCaptureEveryFrame = false;
	Capture2D->bCaptureOnMovement = false;
	bIsEditorOnlyActor = true;
	SceneComponent->bIsEditorOnly = true;
	Capture2D->bIsEditorOnly = true;
	Capture2D->TextureTarget = nullptr;
}

void AMapCaptureActor::CaptureMap()
{
	TilePositions.Empty();
	
	// Setup something.
	const int32 TilesPerAxis = TileAxisCount;
	const float TileWorldSize = EndPoint.X / TilesPerAxis;

	// Loop capture.
	Capture2D->OrthoWidth = EndPoint.X / TilesPerAxis;
	for (int32 y = 0; y < TilesPerAxis; y++)
	{
		for (int32 x = 0; x < TilesPerAxis; x++)
		{
			FVector WorldPos = GetActorLocation()
				+ GetActorRightVector() * (x + 0.5f) * TileWorldSize
				+ GetActorForwardVector() * (y + 0.5f) * TileWorldSize;

			TilePositions.Add(FIntPoint(x, y), WorldPos);
		}
	}

	StartCapture();
}

void AMapCaptureActor::CaptureHotPoints() const
{
	UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>();
	if (auto Value = Settings->MapsInfos.Find(MapName))
	{
		if (UMinimapMapData* MapData = Value->LoadSynchronous())
		{
			WriteHotPoints(MapData);
			// ReSharper disable once CppExpressionWithoutSideEffects
			MapData->MarkPackageDirty();
		}
	}
	else
	{
		Settings->LoadConfig(UMinimapSettings::StaticClass(), *Settings->GetDefaultConfigFilename());
		FString AssetPath = Settings->MapTexturePath + "DA_" + MapName;
		FString AssetName = "DA_" + MapName;
		UPackage* Package = CreatePackage(*AssetPath);
		if (UMinimapMapData* NewMapInfo = NewObject<UMinimapMapData>(Package, *AssetName, RF_Public | RF_Standalone))
		{
			WriteHotPoints(NewMapInfo);
		}
		// save mapper class
		FString const PackageName = Package->GetName();
		FString const PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
		
		auto SoftRef = TSoftObjectPtr<UMinimapMapData>(FSoftObjectPath(AssetPath + "." + AssetName));
		if (!bLocalMap)
		{
			Settings->MapsInfos.Add(UGameplayStatics::GetCurrentLevelName(GetWorld()), SoftRef);
			Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());
		}
	}
}

void AMapCaptureActor::CaptureSingleMapTexture()
{
	TilePositions.Empty();
	
	// Setup something.
	const int32 TilesPerAxis = TileAxisCount;
	const float TileWorldSize = EndPoint.X / TilesPerAxis;

	// Loop capture.
	Capture2D->OrthoWidth = EndPoint.X / TilesPerAxis;
	for (int32 y = 0; y < TilesPerAxis; y++)
	{
		for (int32 x = 0; x < TilesPerAxis; x++)
		{
			FVector WorldPos = GetActorLocation()
				+ GetActorRightVector() * (x + 0.5f) * TileWorldSize
				+ GetActorForwardVector() * (y + 0.5f) * TileWorldSize;

			TilePositions.Add(FIntPoint(x, y), WorldPos);
		}
	}

	if (const auto Found = TilePositions.Find(SingleCapture2D))
	{
		const auto TilePos = *Found;
		Capture2D->SetWorldLocation(TilePos);
		Capture2D->CaptureScene();
			
		FTileCaptureResult Tile;
		Tile.TileX = SingleCapture2D.X;
		Tile.TileY = TilesPerAxis - 1 - SingleCapture2D.Y;
		Tile.Size = TextureScale;

		FString TexName = "T_" + MapName
			+ "_" + FString::Printf(TEXT("%d"), LodCount) + "_" + FString::Printf(TEXT("%dx%d"), Tile.TileX, Tile.TileY);
		UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>();
		FString TotalFileName = FPaths::Combine(Settings->MapTexturePath, TexName);

		CaptureMiniMapToPath(TotalFileName, Capture2D->TextureTarget);
	}
}

void AMapCaptureActor::CaptureMapSequence()
{
	for (const auto& Sequence : Sequences)
	{
		LodCount = Sequence.LodIndex;
		Power = Sequence.ResolutionPower;
		TextureScale = TextureScale = FMath::Pow(static_cast<float>(2), static_cast<float>(Power));
		TileAxisCount = Sequence.TileAxisCount;
		SpecificFolder = Sequence.SpecificFolder;
		
		CaptureMap();
	}
}

void AMapCaptureActor::WriteMapInfo(UMinimapMapData* DataAsset, UTexture2D* Tex)
{
	DataAsset->LevelName = MapName;
	DataAsset->MapSize = EndPoint.X;
	DataAsset->MapTexture = Tex;
	DataAsset->TextureSize = TextureScale;
	DataAsset->CaptureActorLocation = GetActorLocation();
}

void AMapCaptureActor::WriteHotPoints(UMinimapMapData* DataAsset) const
{
	if (!DataAsset)
	{
		return;
	}

	DataAsset->Modify();
	DataAsset->HotPointInfos.Empty();
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMapHotPointActor::StaticClass(), OutActors);
	for (auto Actor : OutActors)
	{
		if (const auto Point = Cast<AMapHotPointActor>(Actor))
		{
			DataAsset->HotPointInfos.Add(Point->PoiInfo.Id, Point->PoiInfo);
			Point->Modify();
			Point->HotPointLevelName = MapName;
			// ReSharper disable once CppExpressionWithoutSideEffects
			Point->MarkPackageDirty();
		}
	}

	DataAsset->BuildHotPointsQuadTree();
	// ReSharper disable once CppExpressionWithoutSideEffects
	DataAsset->MarkPackageDirty();
}

void AMapCaptureActor::StartCapture()
{
	AllTiles.Empty();
	CurrentTileIndex = 0;
	CaptureNextTile();
}

void AMapCaptureActor::CaptureNextTile()
{
	const int32 TilesPerAxis = TileAxisCount;
	const auto Array = TilePositions.Array();
	if (Array.IsValidIndex(CurrentTileIndex))
	{
		const auto TilePos = Array[CurrentTileIndex];
		Capture2D->SetWorldLocation(TilePos.Value);
		Capture2D->CaptureScene();
			
		FTileCaptureResult Tile;
		Tile.TileX = TilePos.Key.X;
		Tile.TileY = TilesPerAxis - 1 - TilePos.Key.Y;
		Tile.Size = TextureScale;

		FString TexName = "T_" + MapName
			+ "_" + FString::Printf(TEXT("%d"), LodCount) + "_" + FString::Printf(TEXT("%dx%d"), Tile.TileX, Tile.TileY);
		const UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>();
		FString TotalFileName = FPaths::Combine(Settings->MapTexturePath + SpecificFolder, TexName);

		CaptureMiniMapToPath(TotalFileName, Capture2D->TextureTarget);
	}
	
	CurrentTileIndex++;
	if (CurrentTileIndex != Array.Num())
	{
		CaptureNextTile();
	}
}

void AMapCaptureActor::OnConstruction(const FTransform& Transform)
{
	// Map name.
	if (!bLocalMap)
	{
		MapName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	}
	// Texture size
	TextureScale = FMath::Pow(static_cast<float>(2), static_cast<float>(Power));
	// Square shape end point.
	EndPoint = FVector(EndPoint.X, EndPoint.X * UKismetMathLibrary::SignOfFloat(EndPoint.Y), 0);
	// Update camera location.
	Capture2D->SetWorldLocation(FVector(GetActorLocation().X + 0.5 * EndPoint.X, GetActorLocation().Y + 0.5 * EndPoint.Y, GetActorLocation().Z));
	// Set up render target
	if (!Capture2D->TextureTarget)
	{
		UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
		RenderTarget->InitAutoFormat(TextureScale, TextureScale);
		RenderTarget->ClearColor = FLinearColor::Black;
		RenderTarget->UpdateResourceImmediate(true);
		Capture2D->TextureTarget = RenderTarget;
	}
	else
	{
		Capture2D->TextureTarget->InitAutoFormat(TextureScale, TextureScale);
	}
}

void AMapCaptureActor::OverwriteExistingTextureFromRenderTarget(UTexture2D* TargetTexture, UTextureRenderTarget2D* SourceRT)
{
	if (!TargetTexture || !SourceRT)
	{
		return;
	}
 
	FTextureRenderTargetResource* RTResource = SourceRT->GameThread_GetRenderTargetResource();
	if (!RTResource) return;
 
	// 1. 从 RenderTarget 中读取像素数据
	FIntPoint Size = FIntPoint(SourceRT->SizeX, SourceRT->SizeY);
	TArray<FColor> OutPixels;
    
	// ReadPixels 是一个同步操作，会阻塞直到 GPU 完成渲染
	RTResource->ReadPixels(OutPixels);
 
	if (OutPixels.Num() == 0) return;
 
	// 2. 准备更新纹理的 Source 数据
	// 关键：这里直接操作 TargetTexture 的 Source，而不是创建新对象
	TargetTexture->Source.Init(Size.X, Size.Y, 1, 1, TSF_BGRA8, (uint8*)OutPixels.GetData());
 
	// 3. 配置纹理属性（保持与小地图需求一致）
	TargetTexture->SRGB = SourceRT->SRGB;
	TargetTexture->CompressionSettings = TC_Default;
	TargetTexture->MipGenSettings = TMGS_NoMipmaps; // 小地图通常不需要Mipmaps
	TargetTexture->LODGroup = TEXTUREGROUP_UI;
 
	// 4. 通知渲染系统和编辑器资产系统
	TargetTexture->UpdateResource();
	TargetTexture->PostEditChange();
    
	// 标记资产为“脏”，以便提醒用户保存
	// ReSharper disable once CppExpressionWithoutSideEffects
	TargetTexture->MarkPackageDirty();
}

void AMapCaptureActor::CaptureMiniMapToPath(const FString& PackagePath, UTextureRenderTarget2D* SourceRT)
{
	if (!SourceRT || PackagePath.IsEmpty()) return;
	
	if (UTexture2D* ExistingTex = LoadObject<UTexture2D>(nullptr, *PackagePath))
	{
		OverwriteExistingTextureFromRenderTarget(ExistingTex, SourceRT);
		UE_LOG(LogTemp, Log, TEXT("MiniMap: Updated existing texture at %s"), *PackagePath);
	}
	else
	{
		UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(SourceRT, PackagePath, TC_Default, TMGS_NoMipmaps);
		UE_LOG(LogTemp, Warning, TEXT("MiniMap: Existing texture not found. Created new one at default path."));
	}
}