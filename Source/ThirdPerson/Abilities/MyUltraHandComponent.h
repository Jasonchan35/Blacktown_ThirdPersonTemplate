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
	float		FusiblePointSearchRadius;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AActor>	TargetActor;

public:
	UMyUltraHandComponent();

	void	SetTargetActor(AActor* Actor);
	AActor*	GetTargetActor() { return TargetActor.Get(); }

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
	void UpdateTargetActorLocation(float DeltaTime);

	struct FFusiblePoint
	{
		FFusiblePoint() { Reset(); }
		void Reset() { DestActor = nullptr; }

		explicit operator bool() const { return DestActor.Get() != nullptr; }

		TWeakObjectPtr<AActor>	SourceActor;
		TWeakObjectPtr<AActor>	DestActor;

		FVector		ActorLocation;
		float		Distance;
		FVector		SourcePoint;
		FVector		ImpactPoint;
	};

	FFusiblePoint	FusiblePoint;

	bool UpdateFusiblePoint();

	TArray<FOverlapResult>	FusiblePointTempOverlaps;
	FFusiblePoint			FusiblePointAsyncSweep;
	FTraceDelegate			FusiblePointAsyncSweepDelegate;
	void FusiblePointAsyncSweepResult(const FTraceHandle& TraceHandle, FTraceDatum& Data);
};