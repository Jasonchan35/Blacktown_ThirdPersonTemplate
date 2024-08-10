// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyMainGameMode.h"
#include "MyCharacter.h"
#include "UObject/ConstructorHelpers.h"

#include "MyPlayerController.h"

AMyMainGameMode::AMyMainGameMode()
{
	MY_CDO_FINDER(DefaultPawnClass,			TEXT("/Game/ThirdPerson/Blueprints/BP_MyCharacter"));
	MY_CDO_FINDER(PlayerControllerClass,	TEXT("/Game/ThirdPerson/Blueprints/BP_MyPlayerController"));
}
