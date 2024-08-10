// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyMainGameMode.h"
#include "MyCharacter.h"
#include "UObject/ConstructorHelpers.h"

#include "MyPlayerController.h"

AMyMainGameMode::AMyMainGameMode()
{
	MY_CLASS_FINDER(DefaultPawnClass, TEXT("/Game/ThirdPerson/Blueprints/BP_MyCharacter"));
	PlayerControllerClass = AMyPlayerController::StaticClass();
}
