#include "MyUltraHandComponent.h"

UMyUltraHandComponent::UMyUltraHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	GoalRotator = FRotator(0, 0, 0);
	GoalExtend  = FVector(200, 0, 50);
	DampingFactor = 4;
}

void UMyUltraHandComponent::SetTarget(AActor* Actor)
{
	auto* Cur = TargetActor.Get();
	if (Cur == Actor)
		return;

	if (Cur)
		MyActorUtil::SetEnableGravity(Cur, true);

	TargetActor = Actor;

	if (Actor)
		MyActorUtil::SetEnableGravity(Actor, false);
}

void UMyUltraHandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	auto* Target = TargetActor.Get();
	if (!Target)
		return;

	auto* Owner = GetOwner();
	if (!Owner)
		return;

	auto LineStart = Owner->GetActorLocation();
	auto LineEnd   = LineStart + GoalRotator.RotateVector(GoalExtend);

	DrawDebugLine(GetWorld(), LineStart, LineEnd, FColor::Green, false, 0, 0, 0);

	auto GoalLoc = LineEnd;

	auto* PrimComp = Target->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return;

	auto TargetLoc = Target->GetActorLocation();
	auto Delta = GoalLoc - TargetLoc;

	FVector NewLoc = FMath::VInterpTo(TargetLoc, GoalLoc, DeltaTime, DampingFactor);

	Target->SetActorLocation(NewLoc, true, nullptr, ETeleportType::ResetPhysics);
}

