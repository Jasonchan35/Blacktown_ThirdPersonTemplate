#pragma once

#include "Components/ActorComponent.h"
#include "MyUltraHandComponent.generated.h"

UCLASS()
class UMyUltraHandComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator	GoalRotator;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector		GoalExtend;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float		DampingFactor;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AActor>	TargetActor;

public:
	UMyUltraHandComponent();

	void SetTarget(AActor* Actor);

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
};