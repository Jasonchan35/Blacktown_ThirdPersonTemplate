#pragma once

#include "MyCharacter.h"
#include "MyPlayerController.generated.h"

struct FInputActionValue;

UCLASS()
class AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> InputMappingContext;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Move;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Look;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Jump;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Skill;

public:
	AMyPlayerController();

	FORCEINLINE AMyCharacter* GetCharacter() const { return CastChecked<AMyCharacter>(Super::GetCharacter()); }

protected:
	virtual void SetupInputComponent() override;

	void IA_Move_Triggered(const FInputActionValue& Value);
	void IA_Look_Triggered(const FInputActionValue& Value);

	void IA_Jump_Started(const FInputActionValue& Value);
	void IA_Jump_Completed(const FInputActionValue& Value);

	void IA_Skill_Started(const FInputActionValue& Value);
	void IA_Skill_Completed(const FInputActionValue& Value);
};