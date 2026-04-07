// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SUniformGridPanel.h"

class MINIMAP_API SMapGridPanel : public SUniformGridPanel
{
	FIntPoint Offsets[9] = {
		{-1,-1}, {0,-1}, {1,-1},  // 上排
		{-1, 0}, {0, 0}, {1, 0},  // 中排（中心）
		{-1, 1}, {0, 1}, {1, 1}   // 下排
	};
	
public:
	SMapGridPanel();
	virtual ~SMapGridPanel();
	
	SLATE_BEGIN_ARGS(SMapGridPanel)
	{}

	SLATE_END_ARGS()
	
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, const int32 InAxisCount, const TArray<UTexture2D*>& InMapTextures);

	void SetAxisCount(const int32 InAxisCount);

	void SetTextures(const TArray<UTexture2D*>& InMapTextures) { MapTextures = InMapTextures; }
	
	void SetCoordinate(const FIntPoint InCoordinate)
	{
		CenterCoordinate = InCoordinate;
	}

	UTexture2D* GetTextureAtCoordinate(FIntPoint InCoordinate) const;

	void Update() const;
	
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

protected:
	TArray<UTexture2D*> MapTextures;
	TMap<FIntPoint, FScopedWidgetSlotArguments> Slots;
	FIntPoint CenterCoordinate = FIntPoint(0, 0);
	TArray<TSharedPtr<SImage>> TileImages;
	int32 AxisCount = 1;
};
