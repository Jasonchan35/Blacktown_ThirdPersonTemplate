// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyMainGameMode.h"
#include "MyCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMyMainGameMode::AMyMainGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_MyCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
