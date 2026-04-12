// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MapGridPanel.h"

UMapGridPanel::UMapGridPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsVariable = false;
	SetVisibilityInternal(ESlateVisibility::SelfHitTestInvisible);
}

void UMapGridPanel::SetCoordinate(FIntPoint InCoordinate)
{
	if (!MyMapGridPanel.IsValid())
	{
		return;
	}

	MyMapGridPanel->SetCoordinate(InCoordinate);
}

void UMapGridPanel::SetAxisCount(int32 InAxisCount)
{
	AxisCount = InAxisCount;
	
	if (!MyMapGridPanel.IsValid())
	{
		return;
	}

	MyMapGridPanel->SetAxisCount(InAxisCount);
}

void UMapGridPanel::SetTextures(TArray<UTexture2D*> InTextures)
{
	Textures = InTextures;
	
	if (!MyMapGridPanel.IsValid())
	{
		return;
	}

	MyMapGridPanel->SetTextures(InTextures);
}

void UMapGridPanel::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	
	if (!MyMapGridPanel.IsValid())
	{
		return;
	}

	MyMapGridPanel->SetAxisCount(AxisCount);
	MyMapGridPanel->SetTextures(Textures);
}

void UMapGridPanel::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyMapGridPanel.Reset();
}

TSharedRef<SWidget> UMapGridPanel::RebuildWidget()
{
	MyMapGridPanel = SNew(SMapGridPanel, AxisCount, Textures);

	return MyMapGridPanel.ToSharedRef();
}
