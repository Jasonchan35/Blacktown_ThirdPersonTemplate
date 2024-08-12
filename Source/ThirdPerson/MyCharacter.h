// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

class AMyPlayerController;
class UMyAbilityComponent;
class UMyUltraHandComponent;

UENUM()
enum class EMyAbility
{
	None,
	UltraHand,
};

UCLASS(config=Game)
class AMyCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(Category = Skill, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMyUltraHandComponent>	UltraHandComponent;

public:
	AMyCharacter();
	void		SetCurrentAbility(EMyAbility Ability);
	EMyAbility	GetCurrentAbility() const { return CurrentAbility; }

	AMyPlayerController* GetPlayerController();

	void IA_Confirm_Started();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	TObjectPtr<UMyAbilityComponent>	CurrentAbilityComponent;
	EMyAbility	CurrentAbility = EMyAbility::None;
};
