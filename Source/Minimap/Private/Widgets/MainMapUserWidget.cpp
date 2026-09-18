// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MainMapUserWidget.h"

#include "MinimapMapData.h"
#include "MinimapSettings.h"
#include "MinimapSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/MinimapGlobal.h"
#include "Components/Overlay.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameStateBase.h"
#include "Widgets/MapPinUserWidget.h"
#include "Widgets/MinimapUserWidget.h"
#include "Widgets/MinimapWidgetInterface.h"

UMainMapUserWidget::UMainMapUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MarkerClassType = TYPE_MAINMAP;
}

void UMainMapUserWidget::OnValueChanged(float Value)
{
	Scale = Value;
	UpdateText();
}

void UMainMapUserWidget::OnMapPinAddEvent(const FGuid& MapPinId)
{
	AddMapPin(MapPinId);
}

void UMainMapUserWidget::OnMapPinRemoveEvent(const FGuid& MapPinId)
{
	RemoveMapPin(MapPinId);
}

void UMainMapUserWidget::OnHotPointFound(const FString& LevelName, const FGuid& Guid)
{
	AddMapPin(Guid);
}

void UMainMapUserWidget::OnHotPointRemove(const FString& LevelName, const FGuid& Guid)
{
	RemoveMapPin(Guid);
}

void UMainMapUserWidget::InitializeSlider() const
{
	if (const auto Slider = GetSliderWidget())
	{
		Slider->SetMinValue(MinScale);
		Slider->SetMaxValue(MaxScale);
		Slider->SetValue(Scale);
	}
}

void UMainMapUserWidget::InitializeMapPins()
{
	if (const auto Interface = TryGetDataInterface())
	{
		TSet<FGuid> TotalPins;
		TSet<FGuid> HotPoints;
		Interface->GetRegisteredMapPins(TotalPins);
		Interface->GetFoundHotPoints(HotPoints);
		TotalPins.Append(HotPoints);
		
		for (const auto Itr : TotalPins)
		{
			AddMapPin(Itr);
		}
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

void UMainMapUserWidget::UpdateMarkers() const
{
	if (const auto Interface = TryGetDataInterface())
	{
		for (const auto Marker : Markers)
		{
			FVector OutLocation;
			bool HasLocation = Interface->GetLocation(Marker.Key, OutLocation);
			float Yaw;
			bool HasYaw = Interface->GetYaw(Marker.Key, Yaw);
			if (HasLocation)
			{
				Marker.Value->SetRenderTranslation(WorldToWidget(FVector2D(OutLocation), Scale));
			}
			Marker.Value->SetRenderTransformAngle(HasYaw ? Yaw : 0.0f);
			if (ShouldHide(Marker.Key))
			{
				Marker.Value->SetVisibility(ESlateVisibility::Hidden);
			}
			else
			{
				Marker.Value->SetVisibility(IsMapPinVisible(Marker.Key) ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
			}
		}
	}
}

void UMainMapUserWidget::ReleaseAllMarkers()
{
	// Clear widgets on destruct.
	for (auto Itr : Markers)
	{
		Itr.Value->RemoveFromParent();
		WidgetPool.Release(Itr.Value);
	}
	Markers.Empty();
}

void UMainMapUserWidget::NativeConstruct()
{
	InitializeSlider();
	UpdateText();
	InitializeMapPins();
	ManageEvents(true);
	SetFocus();

	Super::NativeConstruct();
}

int32 UMainMapUserWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	UpdateTransform();
	UpdateMarkers();
	
	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                          bParentEnabled);
}

void UMainMapUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	UpdateDragging();
	
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UMainMapUserWidget::NativeDestruct()
{
	ManageEvents(false);
	ReleaseAllMarkers();

	Super::NativeDestruct();
}

FReply UMainMapUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(DragKey))
	{
		bMouseDragging = true;
		return FReply::Handled();
	}

	const auto MapImageWidget = GetImageWidget();
	if (InMouseEvent.IsMouseButtonDown(MarkKey) && MapImageWidget)
	{
		const auto ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		const auto Local = MapImageWidget->GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
		AddTempPin(FVector2D(GetCaptureCenter()) - WidgetToWorld(Local),
				GetCurrentGlobalMapData(), ECC_Visibility);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UMainMapUserWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == DragKey)
	{
		bMouseDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	
	return FReply::Unhandled();
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
	
	if (const auto Interface = TryGetDataInterface())
	{
		if (bManage)
		{
			if (Interface->GetMapPinAddEvent())
			{
				Interface->GetMapPinAddEvent()->AddUObject(this, &ThisClass::OnMapPinAddEvent);
			}
			if (Interface->GetMapPinRemoveEvent())
			{
				Interface->GetMapPinRemoveEvent()->AddUObject(this, &ThisClass::OnMapPinRemoveEvent);
			}
		}
		else
		{
			if (Interface->GetMapPinAddEvent())
			{
				Interface->GetMapPinAddEvent()->RemoveAll(this);
			}
			if (Interface->GetMapPinRemoveEvent())
			{
				Interface->GetMapPinRemoveEvent()->RemoveAll(this);
			}
		}
	}
}

void UMainMapUserWidget::AddTempPin(const FVector2D Location, const UMinimapMapData* MapData,
	const ECollisionChannel TraceChannel) const
{
	//Set map highest point, used to be the z location of the map capture actor.
	float MapHighestPoint = 100000.f;
	if (MapData)
	{
		MapHighestPoint = MapData->CaptureActorLocation.Z;
	}

	//Get setting
	float HitResultTraceDistance = 100000.f;
	if (const UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
	{
		HitResultTraceDistance = Settings->ControllerHitResultDistance;
	}
	
	// Line trace by channel, channel is visibility
	FHitResult HitResult;
	const FVector Start = FVector(Location.X, Location.Y, MapHighestPoint);
	const FVector End = FVector(Location.X, Location.Y, MapHighestPoint - HitResultTraceDistance);
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, TraceChannel))
	{
		if (const auto World = GetWorld())
		{
			if (const auto GS = World->GetGameState())
			{
				if (const auto Global = GS->GetComponentByClass<UMinimapGlobal>())
				{
					Global->AddTempPin(GetOwningPlayer(), HitResult.Location, TempPinBrush);
				}
			}
		}
	}
}

bool UMainMapUserWidget::IsMapPinVisible_Implementation(FGuid Guid) const
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

FVector2D UMainMapUserWidget::WorldToWidget(const FVector2D InWorldPosition2D, const float& InScale) const
{
	const auto CurrentData = GetCurrentGlobalMapData();
	const auto MapImage = GetImageWidget();
	if (CurrentData && MapImage)
	{
		FVector2D Dir;
		float Length;
		(InWorldPosition2D - FVector2D(GetCaptureCenter())).ToDirectionAndLength(Dir, Length);
		const FVector2D FinalVector = Dir * (Length / CurrentData->MapSize * MapImage->GetDesiredSize().X * FMath::Square(InScale));
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

void UMainMapUserWidget::LocalPawnCenter()
{
	if (LocalPawn)
	{
		const auto WidgetOffset = WorldToWidget(FVector2D(LocalPawn->GetActorLocation()), 1.0f);
		Offset = -WidgetOffset;
	}
}
