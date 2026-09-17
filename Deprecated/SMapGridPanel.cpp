// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SMapGridPanel.h"

#include "SlateOptMacros.h"
#include "Kismet/KismetMathLibrary.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SMapGridPanel::SMapGridPanel()
{
}

SMapGridPanel::~SMapGridPanel() = default;

void SMapGridPanel::Construct(const FArguments& InArgs, const int32 InAxisCount, const TArray<UTexture2D*>& InMapTextures)
{
	AxisCount = InAxisCount;
	MapTextures = InMapTextures;
	
	for (int i = 0; i < 9; ++i)
	{
		const auto NewImage = SNew(SImage);
		TileImages.Add(NewImage.ToSharedPtr());
	}
	
	for (int i = 0; i < AxisCount * AxisCount; ++i)
	{
		double X;
		const float Y = UKismetMathLibrary::FMod(i, AxisCount, X);
		Slots.Add(FIntPoint(X, Y), AddSlot(X, Y));
	}

	Update();
}

void SMapGridPanel::SetAxisCount(const int32 InAxisCount)
{
	ClearChildren();
	Slots.Empty();
	AxisCount = InAxisCount;
	for (int i = 0; i < AxisCount * AxisCount; ++i)
	{
		double X;
		const float Y = UKismetMathLibrary::FMod(i, AxisCount, X);
		Slots.Add(FIntPoint(X, Y), AddSlot(X, Y));
	}
}

UTexture2D* SMapGridPanel::GetTextureAtCoordinate(FIntPoint InCoordinate) const
{
	const auto Column = InCoordinate.X * AxisCount;
	const auto Index = Column + InCoordinate.Y;
	if (MapTextures.IsValidIndex(Index))
	{
		return MapTextures[Index];
	}
	
	return nullptr;
}

void SMapGridPanel::Update() const
{
	for (int i = 0; i < Offsets->Num(); ++i)
	{
		const auto Offset = Offsets[i];
		const auto Image = TileImages[i];
		const auto Itr = CenterCoordinate + Offset;
		if (const auto FoundSlot = Slots.Find(Itr))
		{
			const auto SlotWidget = FoundSlot->GetSlot()->GetWidget().ToSharedPtr();
			const TSharedPtr<SImage> SlotImage = StaticCastSharedPtr<SImage>(SlotWidget);
			if (SlotImage.IsValid())
			{
				auto EmptyBrush = FSlateBrush();
				SlotImage->SetImage(&EmptyBrush);
			}
			
			auto Brush = FSlateBrush();
			const auto Texture = GetTextureAtCoordinate(Itr);
			Brush.SetResourceObject(Texture);
			Image->SetImage(&Brush);
			FoundSlot->GetSlot()->AttachWidget(Image.ToSharedRef());
		}
	}
}

int32 SMapGridPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                             FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
                             bool bParentEnabled) const
{
	Update();
	
	return SUniformGridPanel::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                                  bParentEnabled);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
