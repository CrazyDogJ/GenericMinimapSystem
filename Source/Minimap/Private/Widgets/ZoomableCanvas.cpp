// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/ZoomableCanvas.h"

#include "Components/Image.h"
#include "Widgets/SZoomableCanvas.h"

#define LOCTEXT_NAMESPACE "UMG"

void UZoomableCanvas::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyZoomableCanvas.IsValid())
	{
		MyZoomableCanvas->SetDebugScreenString(bDebug);
		MyZoomableCanvas->SetZoomStep(ZoomStep);
		MyZoomableCanvas->SetMaxScale(MaxScale);
		MyZoomableCanvas->SetMinScale(MinScale);
		MyZoomableCanvas->SetEnableInput(bEnableInput);
		MyZoomableCanvas->SetViewOffset(ViewOffset);
		MyZoomableCanvas->SetViewScale(ViewScale);
		MyZoomableCanvas->DebugMultiplier = DebugMultiplier;
	}
}

void UZoomableCanvas::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	
	MyZoomableCanvas.Reset();
}

void UZoomableCanvas::SetViewScale(float Scale)
{
	ViewScale = Scale;
	if (MyZoomableCanvas.IsValid())
	{
		return MyZoomableCanvas->SetViewScale(Scale);
	}
}

void UZoomableCanvas::SetViewOffset(FVector2D Offset)
{
	ViewOffset = Offset;
	if (MyZoomableCanvas.IsValid())
	{
		return MyZoomableCanvas->SetViewOffset(Offset);
	}
}

void UZoomableCanvas::SetEnableInput(bool bNewEnableInput)
{
	bEnableInput = bNewEnableInput;
	if (MyZoomableCanvas.IsValid())
	{
		return MyZoomableCanvas->SetEnableInput(bEnableInput);
	}
}

void UZoomableCanvas::SetDebugEnabled(bool Enable)
{
	bDebug = Enable;
	if (MyZoomableCanvas.IsValid())
	{
		return MyZoomableCanvas->SetDebugScreenString(bDebug);
	}
}

void UZoomableCanvas::SetZoomStep(float InZoomStep)
{
	ZoomStep = InZoomStep;
	if (MyZoomableCanvas.IsValid())
	{
		return MyZoomableCanvas->SetZoomStep(InZoomStep);
	}
}

float UZoomableCanvas::GetCurrentScale() const
{
	if (MyZoomableCanvas.IsValid())
	{
		return MyZoomableCanvas->GetScale();
	}
	
	return -1.0f;
}

void UZoomableCanvas::SetMinScale(float Scale)
{
	MinScale = Scale;
	if (MyZoomableCanvas.IsValid())
	{
		MyZoomableCanvas->SetMinScale(MinScale);
	}
}

void UZoomableCanvas::SetMaxScale(float Scale)
{
	MaxScale = Scale;
	if (MyZoomableCanvas.IsValid())
	{
		MyZoomableCanvas->SetMaxScale(MaxScale);
	}
}

void UZoomableCanvas::SetDebugMultiplier(float Multiplier)
{
	DebugMultiplier = Multiplier;
	if (MyZoomableCanvas.IsValid())
	{
		MyZoomableCanvas->DebugMultiplier = DebugMultiplier;
	}
}

void UZoomableCanvas::AddWidgetToCanvas(UWidget* InWidget, const FVector2D InPosition, const FVector2D InSize) const
{
	if (!InWidget)
	{
		return;
	}
	
	if (!MyZoomableCanvas.IsValid())
	{
		return;
	}

	const auto TakeWidget = InWidget->TakeWidget();
	MyZoomableCanvas->AddTile(TakeWidget, InPosition, InSize);
}

const FText UZoomableCanvas::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

TSharedRef<SWidget> UZoomableCanvas::RebuildWidget()
{
	MyZoomableCanvas = SNew(SZoomableCanvas);

	// SetClipping(EWidgetClipping::ClipToBounds);
	// 
	// if (MyZoomableCanvas.IsValid())
	// {
	// 	MyZoomableCanvas->SetClipping(EWidgetClipping::ClipToBounds);
	// }
	
	return MyZoomableCanvas.ToSharedRef();
}

#if WITH_ACCESSIBILITY
TSharedPtr<SWidget> UZoomableCanvas::GetAccessibleWidget() const
{
	return MyZoomableCanvas;
}
#endif

#undef LOCTEXT_NAMESPACE
