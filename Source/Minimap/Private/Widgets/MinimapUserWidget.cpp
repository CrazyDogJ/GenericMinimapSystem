// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MinimapUserWidget.h"

#include "Components/MinimapComponent_Player.h"
#include "Widgets/MapPinUserWidget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"

void UMinimapUserWidget::NativeConstruct()
{
	if (GetMinimapImageWidget() && GetCurrentMapData())
	{
		const auto DynMat = GetMinimapImageWidget()->GetDynamicMaterial();
		const auto Texture = GetCurrentMapData()->MapTexture;
		Texture->UpdateResource();
		Texture->SetForceMipLevelsToBeResident(10.0f);
		DynMat->SetTextureParameterValue("Map", GetCurrentMapData()->MapTexture);
	}
	
	if (const auto MinimapPlayer = GetLocalPlayerMinimapComponent())
	{
		MinimapPlayer->OnLocalMinimapChanged.AddDynamic(this, &ThisClass::OnLocalMapDataChanged);
		MinimapPlayer->SetMinimapRadius(DefaultRadius);
		// Initializing pins.
		for (const auto Itr : MinimapPlayer->GetShownMapPins())
		{
			OnMarkerShowOnMinimap(Itr);
		}
		InterpRadius = DefaultRadius;
		// Binding events.
		MinimapPlayer->OnMapPinShowOnMinimap.AddDynamic(this, &ThisClass::OnMarkerShowOnMinimap);
		MinimapPlayer->OnMapPinHideOnMinimap.AddDynamic(this, &ThisClass::OnMarkerHideOnMinimap);
	}
	
	Super::NativeConstruct();
}

void UMinimapUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	UpdateInterpRadius(InDeltaTime);
	UpdateViewAngle();
	UpdateLocalPlayerAngle();
	UpdateNorthWidgets();
	UpdateMarkers();
	UpdateMinimapImageParameters();
	
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UMinimapUserWidget::NativeDestruct()
{
	if (const auto MinimapPlayer = GetLocalPlayerMinimapComponent())
	{
		MinimapPlayer->OnLocalMinimapChanged.RemoveAll(this);
		MinimapPlayer->OnMapPinShowOnMinimap.RemoveAll(this);
		MinimapPlayer->OnMapPinHideOnMinimap.RemoveAll(this);
	}
	
	Super::NativeDestruct();
}

TSubclassOf<UMapPinUserWidget> UMinimapUserWidget::GetCustomClass(const FGuid& Guid)
{
	if (const auto Subsystem = GetMinimapSubsystem())
	{
		bool bSuccess;
		const auto MapStruct = Subsystem->GetShownMinimapPin(Guid, bSuccess);
		if (bSuccess)
		{
			if (MapStruct.CustomMinimapWidgetClass)
				return MapStruct.CustomMinimapWidgetClass;
		}
	}
	
	return Super::GetCustomClass(Guid);
}

void UMinimapUserWidget::OnLocalMapDataChanged(const UMinimapMapData* ChangedMinimapData)
{
	if (const auto LocalComp = GetLocalPlayerMinimapComponent())
	{
		LocalComp->SetMinimapRadius(GetMinimapDisplayRadius());
	}
}

void UMinimapUserWidget::OnMarkerHideOnMinimap(FGuid Guid)
{
	RemoveMapPin(Guid);
}

void UMinimapUserWidget::OnMarkerShowOnMinimap(FGuid Guid)
{
	if (IsLocalPlayerMarker(Guid))
	{
		return;
	}

	AddMapPin(Guid);
}

void UMinimapUserWidget::UpdateInterpRadius(const float DeltaTime)
{
	const auto CurrentMapData = GetCurrentMapData();
	const auto LocalComp = GetLocalPlayerMinimapComponent();
	if (CurrentMapData && LocalComp)
	{
		InterpRadius = FMath::FInterpTo(InterpRadius, LocalComp->MinimapRadius, DeltaTime, InterpSpeed);
		ZoomMultiplier = GetCurrentMapData()->MapSize / InterpRadius;
	}
}

void UMinimapUserWidget::UpdateViewAngle()
{
	if (GetViewAreaWidget())
	{
		GetViewAreaWidget()->SetRenderTransformAngle(bLockNorth ? GetViewAngle() : 0.0f);
	}
}

void UMinimapUserWidget::UpdateLocalPlayerAngle()
{
	if (GetLocalPlayerWidget())
	{
		const float PlayerYaw = GetLocalPlayerTransform().Rotator().Yaw;
		const float InvertViewYaw = GetViewAngle() * -1;
		GetLocalPlayerWidget()->SetRenderTransformAngle(bLockNorth ? PlayerYaw : PlayerYaw + InvertViewYaw);
	}
}

void UMinimapUserWidget::UpdateNorthWidgets()
{
	if (GetNorthPivotWidget())
	{
		GetNorthPivotWidget()->SetRenderTransformAngle(bLockNorth ? NorthYawOffset : (NorthYawOffset + GetViewAngle()) * -1);
	}
	
	if (GetNorthWidget())
	{
		GetNorthWidget()->SetRenderTransformAngle(NorthYawOffset * -1 + (bLockNorth ? 0 : GetViewAngle()));
	}
}

void UMinimapUserWidget::UpdateMarker(UMapPinUserWidget* MapPin, const FVector2D WorldPosition2D,
	const float Angle, const bool bRotate)
{
	if (MapPin)
	{
		const auto WidgetPosition = GetWidgetPosition(WorldPosition2D);
		MapPin->SetRenderTranslation(WidgetPosition);
		MapPin->SetRenderTransformAngle(bRotate ? Angle : bLockNorth ? 0.0f : GetViewAngle());
	}
}

void UMinimapUserWidget::UpdateMarkers()
{
	if (GetMarkersOverlay())
	{
		GetMarkersOverlay()->SetRenderTransformAngle(bLockNorth ? 0.0f : GetViewAngle() * -1.0);
	}
	
	const auto Subsystem = GetMinimapSubsystem();
	for (const auto Itr : Markers)
	{
		bool Success;
		const auto Found = Subsystem->GetShownMinimapPin(Itr.Key, Success);
		if (Success)
		{
			UpdateMarker(Itr.Value, FVector2D(Found.Location), Found.Yaw, Found.bHasRotation);
		}
	}
}

void UMinimapUserWidget::UpdateMinimapImageParameters()
{
	if (GetMinimapImageWidget())
	{
		GetMinimapImageWidget()->SetRenderTransformAngle(bLockNorth ? 0.0f : GetViewAngle() * -1.0);
		
		if (const auto MatDyn = GetMinimapImageWidget()->GetDynamicMaterial(); MatDyn && GetCurrentMapData())
		{
			const auto DeltaPosition = GetLocalPlayerTransform().GetLocation() - GetCurrentMapData()->CaptureActorLocation;
			const float X = DeltaPosition.X / -GetCurrentMapData()->MapSize + 0.5;
			const float Y = DeltaPosition.Y / GetCurrentMapData()->MapSize - 0.5;
			MatDyn->SetVectorParameterValue("PlayerLocation", FVector(Y, X, 0.0f));
			MatDyn->SetScalarParameterValue("Zoom", ZoomMultiplier);
		}
	}
}

float UMinimapUserWidget::GetMinimapDisplayRadius_Implementation()
{
	// Default radius
	if (const auto CurrentMapData = GetCurrentMapData())
	{
		if (CurrentMapData->MapSize < DefaultRadius)
		{
			return CurrentMapData->MapSize;
		}
	}
	
	return DefaultRadius;
}

float UMinimapUserWidget::GetViewAngle_Implementation()
{
	return GetOwningPlayer()->GetControlRotation().Yaw;
}

FTransform UMinimapUserWidget::GetLocalPlayerTransform_Implementation()
{
	if (const auto LocalPlayerActor = GetLocalPlayerActor())
	{
		return LocalPlayerActor->GetTransform();
	}

	return FTransform::Identity;
}

float UMinimapUserWidget::GetDesiredRadius_Implementation()
{
	if (GetMinimapImageWidget())
	{
		return GetMinimapImageWidget()->GetDesiredSize().X / 2;
	}

	return GetDesiredSize().X / 2;
}

UMinimapMapData* UMinimapUserWidget::GetCurrentMapData() const
{
	if (const auto MinimapComp = GetLocalPlayerMinimapComponent())
	{
		if (MinimapComp->GetCurrentLocalMinimapData())
		{
			return MinimapComp->GetCurrentLocalMinimapData();
		}
	}

	if (const auto MinimapSubsystem = GetMinimapSubsystem())
	{
		return MinimapSubsystem->GetCurrentMinimapMapData();
	}

	return nullptr;
}

FVector2D UMinimapUserWidget::GetWidgetPosition(const FVector2D InWorldPosition2D)
{
	const auto LocalPlayer2D = FVector2D(GetLocalPlayerTransform().GetLocation());
	const FVector2D TempPosition = (LocalPlayer2D - InWorldPosition2D) / InterpRadius * GetDesiredRadius() * 2;
	FVector2D Direction;
	float Length;
	TempPosition.ToDirectionAndLength(Direction, Length);
	Length = FMath::Clamp(Length, 0.0f, GetDesiredRadius());
	return FVector2D((Direction * Length).Y * -1, (Direction * Length).X);
}

bool UMinimapUserWidget::IsLocalPlayerMarker(const FGuid Guid) const
{
	if (const auto Comp = GetLocalPlayerMinimapComponent())
	{
		return Comp->MinimapGuid == Guid;
	}

	return false;
}

bool UMinimapUserWidget::IsCurrentMapDataValid() const
{
	return IsValid(GetCurrentMapData());
}
