// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/MinimapPlayerComponent.h"

#include "EnhancedInputSubsystems.h"
#include "MinimapBlueprintFunctionLibrary.h"
#include "MinimapSubsystem.h"
#include "MinimapZoneGraphAStar.h"
#include "Actors/MapPinActor.h"
#include "Blueprint/UserWidget.h"
#include "Components/MinimapGlobal.h"
#include "GameFramework/GameStateBase.h"
#include "Widgets/MainMapUserWidget.h"
#include "Widgets/MinimapUserWidget.h"

UMinimapPlayerComponent::UMinimapPlayerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMinimapPlayerComponent::OnMinimapGlobalReadyEvent(UMinimapGlobal* MinimapGlobal)
{
	if (const auto MinimapWidget = GetOrCreateMinimapWidget(MinimapGlobal))
	{
		MinimapWidget->AddToViewport();
	}
}

void UMinimapPlayerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	// const auto InputSubsystem = OwningPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	// if (const auto IMC = InputMappingContext.LoadSynchronous())
	// {
	// 	InputSubsystem->AddMappingContext(IMC, 0);
	// }

	if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		if (const auto Global = Sub->GetMinimapGlobal())
		{
			if (const auto MinimapWidget = GetOrCreateMinimapWidget(Global))
			{
				MinimapWidget->AddToViewport();
			}
		}
		else
		{
			Sub->OnMinimapGlobalReadyEvent.AddDynamic(this, &ThisClass::OnMinimapGlobalReadyEvent);
		}
	}
}

void UMinimapPlayerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	if (OwningPlayerController && OwningPlayerController->IsLocalPlayerController())
	{
		UpdateNavPath(DeltaTime);
	}
}

void UMinimapPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	// const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	// if (const auto InputSubsystem = OwningPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	// {
	// 	if (const auto IMC = InputMappingContext.LoadSynchronous())
	// 	{
	// 		InputSubsystem->RemoveMappingContext(IMC);
	// 	}
	// }
}

UMinimapUserWidget* UMinimapPlayerComponent::GetOrCreateMinimapWidget(UObject* DataSourceObject)
{
	if (MinimapUserWidget)
	{
		return MinimapUserWidget;
	}
	
	if (!DataSourceObject)
	{
		if (GetWorld() && GetWorld()->GetGameState())
		{
			DataSourceObject = GetWorld()->GetGameState()->GetComponentByClass<UMinimapGlobal>();
		}
	}
	
	const auto MinimapClass = UMinimapBlueprintFunctionLibrary::GetMinimapWidgetClass();
	const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	if (OwningPlayerController && OwningPlayerController->IsLocalPlayerController() && MinimapClass)
	{
		MinimapUserWidget = CreateWidget<UMinimapUserWidget, APlayerController*>(OwningPlayerController, MinimapClass);
		MinimapUserWidget->MinimapDataSourceObject = DataSourceObject;
	}

	return MinimapUserWidget;
}

UMainMapUserWidget* UMinimapPlayerComponent::GetOrCreateMainMapWidget(UObject* DataSourceObject)
{
	if (MainMapUserWidget)
	{
		return MainMapUserWidget;
	}

	if (!DataSourceObject)
	{
		if (GetWorld() && GetWorld()->GetGameState())
		{
			DataSourceObject = GetWorld()->GetGameState()->GetComponentByClass<UMinimapGlobal>();
		}
	}
	
	const auto MainMapClass = UMinimapBlueprintFunctionLibrary::GetMainmapWidgetClass();
	const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	if (OwningPlayerController && OwningPlayerController->IsLocalPlayerController() && MainMapClass)
	{
		MainMapUserWidget = CreateWidget<UMainMapUserWidget, APlayerController*>(OwningPlayerController, MainMapClass);
		MainMapUserWidget->MinimapDataSourceObject = DataSourceObject;
	}

	return MainMapUserWidget;
}

bool UMinimapPlayerComponent::ShouldShowNavPath() const
{
	return bShouldUpdateNavQuery && bPathPointsValid;
}

void UMinimapPlayerComponent::UpdateNavPath(const float& DeltaTime)
{
	const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>();
	if (!Sub)
	{
		return;
	}

	if (Sub->LocalTempPinActor)
	{
		NavQueryEndPosition = Sub->LocalTempPinActor->GetActorLocation();
		bShouldUpdateNavQuery = true;
	}
	else
	{
		bShouldUpdateNavQuery = false;
	}
	
	// Should update nav query.
	if (bShouldUpdateNavQuery)
	{
		// Update nav query start position using local player actor location.
		if (bAutoUpdateStartLocation)
		{
			NavQueryStartPosition = GetOwner()->GetActorLocation();
		}
		// Update nav query period. Using task to do async task update.
		NavQueryTime += DeltaTime;
		if (NavQueryTime >= NavQueryPeriod)
		{
			NavQueryTime = 0.0f;
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
			{
				FMinimapZoneGraphLanePath OutPath;
				bPathPointsValid = UMinimapZoneGraphAStarLibrary::GetZoneGraphPathBP(this, NavQueryStartPosition, NavQueryEndPosition, NavQueryExtend, OutPath);
				UMinimapZoneGraphAStarLibrary::GetPathPoints(this, OutPath, NavQueryOutPathPoints);
			});
		}
	}
}
