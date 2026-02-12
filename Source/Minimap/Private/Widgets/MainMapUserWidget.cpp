// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MainMapUserWidget.h"

#include "Components/MinimapComponent_Player.h"
#include "MinimapSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Widgets/MapPinUserWidget.h"
#include "Widgets/MinimapUserWidget.h"

void UMainMapUserWidget::OnValueChanged(float Value)
{
	Scale = Value;
	UpdateText();
}

void UMainMapUserWidget::OnStaticReg(const FStaticMapPin& StaticMapPin)
{
	if (!Markers.Find(StaticMapPin.IdentifyGuid))
	{
		AddMapPin(StaticMapPin.IdentifyGuid);
	}
}

void UMainMapUserWidget::OnStaticUnreg(const FStaticMapPin& StaticMapPin)
{
	// If not exist, remove.
	const auto Found = GetMinimapSubsystem()->GetRegisteredComponents().FindByPredicate([StaticMapPin](const UMinimapComponent* Component)
	{
		return Component->MinimapGuid == StaticMapPin.IdentifyGuid;
	});
	
	if (!Found)
	{
		RemoveMapPin(StaticMapPin.IdentifyGuid);
	}
}

void UMainMapUserWidget::OnCompReg(UMinimapComponent* Component)
{
	if (!Markers.Find(Component->MinimapGuid))
	{
		AddMapPin(Component->MinimapGuid);
	}
}

void UMainMapUserWidget::OnCompUnreg(UMinimapComponent* Component)
{
	// If not has static, do not remove map pin.
	auto NewMapPin = FStaticMapPin();
	NewMapPin.IdentifyGuid = Component->MinimapGuid;
	const auto Index = GetMinimapSubsystem()->GetRegisteredStaticMapPins().Find(NewMapPin);
	if (Index == INDEX_NONE)
	{
		RemoveMapPin(Component->MinimapGuid);
	}
}

void UMainMapUserWidget::UpdateText() const
{
	if (const auto TextBlock = GetScaleDisplayText())
	{
		TextBlock->SetText(GetDisplayScaleText());
	}
}

void UMainMapUserWidget::UpdateDragging()
{
	float X;
	float Y;
	UWidgetLayoutLibrary::GetMousePositionScaledByDPI(GetOwningPlayer(), X, Y);
	const FVector2D MousePosition(X, Y);

	const auto Root = GetMapRootWidget();
	if (bMouseDragging && Root)
	{
		Offset += (MousePosition - CachedMousePosition) / FMath::Square(Scale);
		// Clamp offset.
		const auto LocalSize = Root->GetCachedGeometry().GetLocalSize();
		Offset = FVector2D::Clamp(Offset, FVector2D(-LocalSize / 2), FVector2D(LocalSize / 2));
	}
	
	CachedMousePosition = MousePosition;
}

void UMainMapUserWidget::UpdateTransform() const
{
	const auto SquareScale = FMath::Square(Scale);
	if (const auto Root = GetMapRootWidget())
	{
		Root->SetRenderScale(FVector2D(SquareScale));
		Root->SetRenderTranslation(Offset);
		const auto LocalSize = Root->GetCachedGeometry().GetLocalSize();
		Root->SetRenderTransformPivot((LocalSize / 2 - Offset) / LocalSize);
	}

	if (const auto MarkersOverlay = GetMarkersOverlay())
	{
		MarkersOverlay->SetRenderScale(FVector2D(1) / FVector2D(SquareScale));
	}
}

void UMainMapUserWidget::UpdateMarkers()
{
	if (const auto Subsystem = GetMinimapSubsystem())
	{
		for (const auto Marker : Markers)
		{
			bool bSuccess;
			const auto Found = Subsystem->GetShownMinimapPin(Marker.Key, bSuccess);
			if (bSuccess)
			{
				Marker.Value->SetRenderTranslation(WorldToWidget(FVector2D(Found.Location)));
				Marker.Value->SetRenderTransformAngle(Found.bHasRotation ? Found.Yaw : 0.0f);
				Marker.Value->SetVisibility(IsMapPinVisible(Marker.Key) ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
			}
		}
	}
}

void UMainMapUserWidget::NativeConstruct()
{
	if (const auto Slider = GetSliderWidget())
	{
		Slider->SetMinValue(MinScale);
		Slider->SetMaxValue(MaxScale);
		Slider->SetValue(Scale);
	}

	UpdateText();
	
	if (const auto MapImage = GetImageWidget())
	{
		MaterialInstance = MapImage->GetDynamicMaterial();
		// TODO : Do function that switch to local map later
		if (const auto Data = GetCurrentGlobalMapData())
		{
			MaterialInstance->SetTextureParameterValue("Map", Data->MapTexture);
		}
	}

	if (const auto Subsystem = GetMinimapSubsystem())
	{
		for (const auto Comp : Subsystem->GetRegisteredComponents())
		{
			OnCompReg(Comp);
		}

		for (const auto Static : Subsystem->GetRegisteredStaticMapPins())
		{
			OnStaticReg(Static);
		}
	}

	ManageEvents(true);
	SetFocus();

	Super::NativeConstruct();
}

void UMainMapUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	UpdateDragging();
	UpdateTransform();
	UpdateMarkers();

	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UMainMapUserWidget::NativeDestruct()
{
	ManageEvents(false);

	Super::NativeDestruct();
}

TSubclassOf<UMapPinUserWidget> UMainMapUserWidget::GetCustomClass(const FGuid& Guid)
{
	if (const auto Subsystem = GetMinimapSubsystem())
	{
		bool bSuccess;
		const auto MapStruct = Subsystem->GetShownMinimapPin(Guid, bSuccess);
		if (bSuccess)
		{
			if (MapStruct.CustomMainmapWidgetClass)
				return MapStruct.CustomMainmapWidgetClass;
		}
	}
	
	return Super::GetCustomClass(Guid);
}

FReply UMainMapUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(DragKey))
	{
		bMouseDragging = true;
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const auto MapImageWidget = GetImageWidget();
	if (InMouseEvent.IsMouseButtonDown(MarkKey) && MapImageWidget)
	{
		const auto ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		const auto Local = MapImageWidget->GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
		if (const auto LocalPlayerComp = GetLocalPlayerMinimapComponent())
		{
			LocalPlayerComp->AddTempPin_MainMap(
				FVector2D(GetCaptureCenter()) - WidgetToWorld(Local),
				GetCurrentGlobalMapData(), ECC_Visibility);
		}
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMainMapUserWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == DragKey)
	{
		bMouseDragging = false;
	}
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UMainMapUserWidget::ManageEvents(bool bManage)
{
	if (const auto Slider = GetSliderWidget())
	{
		if (bManage)
		{
			Slider->OnValueChanged.AddDynamic(this, &ThisClass::OnValueChanged);
		}
		else
		{
			Slider->OnValueChanged.RemoveAll(this);
		}
	}
	
	if (const auto Subsystem = GetMinimapSubsystem())
	{
		if (bManage)
		{
			Subsystem->OnStaticRegistered.AddDynamic(this, &ThisClass::OnStaticReg);
			Subsystem->OnStaticUnregistered.AddDynamic(this, &ThisClass::OnStaticUnreg);
			Subsystem->OnComponentRegistered.AddDynamic(this, &ThisClass::OnCompReg);
			Subsystem->OnComponentUnregistered.AddDynamic(this, &ThisClass::OnCompUnreg);
		}
		else
		{
			Subsystem->OnStaticRegistered.RemoveAll(this);
			Subsystem->OnStaticUnregistered.RemoveAll(this);
			Subsystem->OnComponentRegistered.RemoveAll(this);
			Subsystem->OnComponentUnregistered.RemoveAll(this);
		}
	}
}

bool UMainMapUserWidget::IsMapPinVisible_Implementation(FGuid Guid)
{
	return true;
}

FText UMainMapUserWidget::GetDisplayScaleText_Implementation() const
{
	const auto ScaleString = FString::Printf(TEXT("%f"), Scale);
	return FText::FromString(ScaleString);
}

UMinimapMapData* UMainMapUserWidget::GetCurrentGlobalMapData() const
{
	if (GetMinimapSubsystem())
	{
		return GetMinimapSubsystem()->GetCurrentMinimapMapData();
	}
	
	return nullptr;
}

FVector UMainMapUserWidget::GetCaptureCenter() const
{
	if (const auto Data = GetCurrentGlobalMapData())
	{
		const float HalfSize = Data->MapSize / 2;
		return Data->CaptureActorLocation + FVector(HalfSize, HalfSize, 0.0f);
	}

	// Invalid map data right now.
	return FVector::ZeroVector;
}

FVector2D UMainMapUserWidget::WorldToWidget(const FVector2D InWorldPosition2D) const
{
	const auto CurrentData = GetCurrentGlobalMapData();
	const auto MapImage = GetImageWidget();
	if (CurrentData && MapImage)
	{
		FVector2D Dir;
		float Length;
		(InWorldPosition2D - FVector2D(GetCaptureCenter())).ToDirectionAndLength(Dir, Length);
		const FVector2D FinalVector = Dir * (Length / CurrentData->MapSize * MapImage->GetDesiredSize().X * FMath::Square(Scale));
		return FVector2D(FinalVector.Y, -FinalVector.X);
	}

	return FVector2D();
}

FVector2D UMainMapUserWidget::WidgetToWorld(FVector2D InLocalVector2D) const
{
	const auto CurrentData = GetCurrentGlobalMapData();
	const auto MapImage = GetImageWidget();
	if (CurrentData && MapImage)
	{
		const auto FinalVector2D = (InLocalVector2D / MapImage->GetDesiredSize() - FVector2D(0.5f)) * CurrentData->MapSize;
		return FVector2D(FinalVector2D.Y, -FinalVector2D.X);
	}
	
	return FVector2D();
}
