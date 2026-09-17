// Copyright Epic Games, Inc. All Rights Reserved.

#include "Minimap.h"

#define LOCTEXT_NAMESPACE "FMinimapModule"

DEFINE_LOG_CATEGORY(LogMinimap);

void FMinimapModule::StartupModule()
{
}

void FMinimapModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMinimapModule, Minimap)