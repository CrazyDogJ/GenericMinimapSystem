// Fill out your copyright notice in the Description page of Project Settings.


#include "MapCaptureActor.h"

#include "EditorAssetLibrary.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "MinimapSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetMathLibrary.h"

int32 AMapCaptureActor::HasConfig(UMinimapSettings* Settings, const FString& LevelName)
{
	if (Settings)
	{
		int32 id=0;
		for (auto setting : Settings->MapsData)
		{
			if (setting.LevelName == LevelName)
			{
				return id;
			}
			id++;
		}
	}
	
	return -1;
}

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
}

void AMapCaptureActor::CaptureMap()
{
#if WITH_EDITOR
	Capture2D->OrthoWidth = EndPoint.X;
	RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), TextureScale, TextureScale);
	Capture2D->TextureTarget = RenderTarget;
	Capture2D->CaptureScene();
	FString texName = FString(TEXT("T_")) + GetWorld()->GetMapName();
	UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>();
	FString TotalFileName = FPaths::Combine(Settings->MapTexturePath, texName);
	
	FAssetRegistryModule* const AssetRegistryModule = FModuleManager::Get().GetModulePtr<FAssetRegistryModule>("AssetRegistry");
	if (!AssetRegistryModule)
	{
		return;
	}
	const IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
	TArray<FAssetData> outData;
	AssetRegistry.GetAssetsByPath(FName(*Settings->MapTexturePath), outData);
	for (auto data : outData)
	{
		if (data.AssetName.ToString() == texName)
		{
			UEditorAssetLibrary::DeleteAsset(TotalFileName);
		}
	}
	
	UTexture2D* tex = UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(RenderTarget, TotalFileName);

	FMinimapStruct structToAdd;
	structToAdd.LevelName = GetWorld()->GetMapName();
	structToAdd.MapSize = EndPoint.X;
	structToAdd.TextureSize = TextureScale;
	structToAdd.CaptureActorLocation = GetActorLocation();
	structToAdd.MapTexture = tex;

	Settings->LoadConfig(UMinimapSettings::StaticClass(), *Settings->GetDefaultConfigFilename());
	int32 foundIndex = HasConfig(Settings, GetWorld()->GetMapName());
	if (foundIndex >= 0 && foundIndex < Settings->MapsData.Num())
	{
		Settings->MapsData[foundIndex] = structToAdd;
	}
	else
	{
		Settings->MapsData.Add(structToAdd);
	}

	Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());
#endif  
}

void AMapCaptureActor::OnConstruction(const FTransform& Transform)
{
	EndPoint = FVector(EndPoint.X, EndPoint.X * UKismetMathLibrary::SignOfFloat(EndPoint.Y), 0);
	Capture2D->SetRelativeLocation(FVector((EndPoint / 2).X, (EndPoint / 2).Y, GetActorLocation().Z));
}