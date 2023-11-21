// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapStructs.h"

bool FMinimapStruct::IsValid() const
{
	return !LevelName.IsEmpty();
}
