#include "MyUltraHandComponent.h"
#include "MyFuseComponent.h"
#include "../MyCharacter.h"

UMyUltraHandComponent::UMyUltraHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	GoalExtend = FVector(300, 0, 50);
	DampingFactor = 8;

	FusiblePoint.Reset();
	FusiblePointSearchRadius = 100;
	FusiblePointAsyncSweep.Reset();
}

bool UMyUltraHandComponent::OnLookPitchInputAdded(float Pitch)
{
	if (!TargetActor.Get())
		return false;

	GoalExtend.Z += Pitch * -6;
	return true;
}

void UMyUltraHandComponent::SetTargetActor(AActor* Actor)
{
	auto* Cur = TargetActor.Get();
	if (Cur == Actor)
		return;

	if (Cur)
	{
		if (auto* PrimComp = Cur->FindComponentByClass<UPrimitiveComponent>())
		{
			PrimComp->SetEnableGravity(true);
			PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
			PrimComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	}

	TargetActor = Actor;
	FusiblePoint.Reset();

	if (Actor)
	{
		auto Yaw  = GetControlRotation().Yaw;
		auto InvQuat = FRotator(0, Yaw, 0).Quaternion().Inverse();
		TargetRelativeQuat = Actor->GetActorQuat() * InvQuat;
	}
}

void UMyUltraHandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateTargetActorLocation(DeltaTime);

	if (!UpdateFusiblePoint())
		FusiblePoint.Reset();

	if (FusiblePoint)
		DrawDebugLine(GetWorld(), FusiblePoint.SourcePoint, FusiblePoint.ImpactPoint, FColor::Red);
}

void UMyUltraHandComponent::UpdateTargetActorLocation(float DeltaTime)
{
	auto* Target = TargetActor.Get();
	if (!Target)
		return;

	auto* Ch = GetOwner<AMyCharacter>();
	if (!Ch)
		return;

	auto ControlRot = GetControlRotation();

	auto Forward = FVector::ForwardVector;

	auto LineStart = Ch->GetActorLocation();
	auto LineEnd   = LineStart + FRotator(0, ControlRot.Yaw, 0).RotateVector(GoalExtend);

	auto GoalLoc = LineEnd;

	auto Distance = FVector::Distance(GoalLoc, Target->GetActorLocation());
	const float DropTargetDistance = 300;
	if (Distance > DropTargetDistance)
	{
		SetTargetActor(nullptr);
		return;
	}

	DrawDebugLine(GetWorld(), LineStart, LineEnd, FColor::Green, false, 0, 0, 0);

	auto* PrimComp = Target->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return;

	auto TargetLoc = Target->GetActorLocation();
	auto Delta = GoalLoc - TargetLoc;

	FVector NewLoc  = FMath::VInterpTo(TargetLoc, GoalLoc, DeltaTime, DampingFactor);
	FQuat   NewQuat = ControlRot.Quaternion() * TargetRelativeQuat;

	Target->SetActorLocationAndRotation(NewLoc, NewQuat, true);
	PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PrimComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PrimComp->SetEnableGravity(false);
}

FRotator UMyUltraHandComponent::GetControlRotation()
{
	auto* Ch = GetOwner<AMyCharacter>();
	if (Ch)
	{
		if (auto* PC = Ch->GetController())
			return PC->GetControlRotation();
	}

	return FRotator::ZeroRotator;
}

bool UMyUltraHandComponent::UpdateFusiblePoint()
{
	auto* Target = TargetActor.Get();
	if (!Target)
		return false;

	auto* TargetPrimComp = Target->FindComponentByClass<UPrimitiveComponent>();
	if (!TargetPrimComp)
		return false;

	FCollisionQueryParams	QueryParams;
	QueryParams.AddIgnoredActor(Target);
	QueryParams.AddIgnoredActor(GetOwner());

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

//---- check overlapped actor within radius
	{
		float SphereRadius = FusiblePointSearchRadius + MyActorUtil::GetBoundingSphereRadius(Target, true);

		FusiblePointTempOverlaps.Reset();
		bool overlapped = GetWorld()->OverlapMultiByObjectType(
											FusiblePointTempOverlaps,
											Target->GetActorLocation(),
											FQuat::Identity,
											ObjectQueryParams,
											FCollisionShape::MakeSphere(SphereRadius),
											QueryParams);
		if (!overlapped)
			return false;
	}

//----- Find out actual impact point by Async Sweep from TargetActor to each overlapped actor
	{
		FusiblePointAsyncSweep.Reset();
		FusiblePointAsyncSweep.SourceActor = TargetActor;

		if (!FusiblePointAsyncSweepDelegate.IsBound())
			FusiblePointAsyncSweepDelegate.BindUObject(this, &ThisClass::FusiblePointAsyncSweepResult);

		auto SweepStart	= Target->GetActorLocation();
		auto SweepRot	= Target->GetActorQuat();
		auto SweepShape	= TargetPrimComp->GetCollisionShape();

		int Index = 0;
		for (auto& Overlap : FusiblePointTempOverlaps)
		{
			if (!Overlap.GetActor())
				continue;

			int  Remain		= FusiblePointTempOverlaps.Num() - Index - 1;
			auto SweepEnd   = Overlap.GetActor()->GetActorLocation();

			GetWorld()->AsyncSweepByObjectType(
							EAsyncTraceType::Single,
							SweepStart, SweepEnd, SweepRot,
							ObjectQueryParams,
							SweepShape,
							QueryParams,
							&FusiblePointAsyncSweepDelegate,
							Remain);
			Index++;
		}
	}
	return true;
}

void UMyUltraHandComponent::FusiblePointAsyncSweepResult(const FTraceHandle& TraceHandle, FTraceDatum& Data)
{
	auto* Target = TargetActor.Get();
	if (!Target)
		return;

	auto& Out = FusiblePointAsyncSweep;

	if (Out.SourceActor != Target)
		return; // TargetActor changed during Async call, so drop it

	for (auto& Hit : Data.OutHits)
	{
		int Remain = Data.UserData;

		auto Dis			= Hit.Distance;
		auto MinDisSq		= Out.Distance;

		if (Out.DestActor.Get() && (Dis * Dis) > MinDisSq)
			continue;

		Out.DestActor		= Hit.GetActor();

		Out.ActorLocation	= Hit.Location;
		Out.Distance		= Dis;

		Out.SourcePoint		= Hit.ImpactPoint - Hit.Location + Hit.TraceStart;
		Out.ImpactPoint		= Hit.ImpactPoint;

		if (Remain == 0) // Last one to update result
			FusiblePoint = Out;
	}
}

void UMyUltraHandComponent::FuseFusibleObject()
{
	auto* DestActor = FusiblePoint.DestActor.Get();
	if (!DestActor)
		return;

	auto* SourceActor = FusiblePoint.SourceActor.Get();
	if (!SourceActor)
		return;

	auto* SourcePrim = SourceActor->FindComponentByClass<UPrimitiveComponent>();
	auto*   DestPrim =   DestActor->FindComponentByClass<UPrimitiveComponent>();

	if (!SourcePrim || !DestPrim)
		return;

	auto* FuseComp = MyActorUtil::GetOrNewComponent<UMyFuseComponent>(SourceActor);
	if (!FuseComp)
		return;

	SourcePrim->SetSimulatePhysics(true);
	  DestPrim->SetSimulatePhysics(true);

	FuseComp->SetWorldLocation(FusiblePoint.SourcePoint);

	FuseComp->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0);
	FuseComp->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0);
	FuseComp->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0);

	FuseComp->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	FuseComp->SetAngularTwistLimit( EAngularConstraintMotion::ACM_Locked, 0);
	FuseComp->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
	FuseComp->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);

	FuseComp->SetConstrainedComponents(SourcePrim, NAME_None, DestPrim, NAME_None);
}
