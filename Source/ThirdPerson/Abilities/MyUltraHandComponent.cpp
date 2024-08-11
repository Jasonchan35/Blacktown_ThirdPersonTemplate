#include "MyUltraHandComponent.h"
#include "MyFuseComponent.h"
#include "../MyCharacter.h"

UMyUltraHandComponent::UMyUltraHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	TargetExtend = FVector2D(300, 50);
	DampingFactor = 3;
	FusiblePointSearchRadius = 100;

	FusiblePoint.Reset();
	FusiblePointAsyncSweep.Reset();
}

void UMyUltraHandComponent::SetTargetActor(AActor* Actor)
{
	auto* Cur = TargetActor.Get();
	if (Cur == Actor)
		return;

	if (Cur)
		MyActorUtil::SetEnableGravity(Cur, true);

	TargetActor = Actor;
	FusiblePoint.Reset();

	if (Actor)
		MyActorUtil::SetEnableGravity(Actor, false);
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

	auto* Controller = Ch->GetController();
	auto Rot = Controller ? Controller->GetControlRotation() : Ch->GetActorRotation();
	
	auto LineStart = Ch->GetActorLocation();
	auto LineEnd   = LineStart + FRotator(0, Rot.Yaw, 0).RotateVector(FVector(TargetExtend.X, 0, TargetExtend.Y));

	DrawDebugLine(GetWorld(), LineStart, LineEnd, FColor::Green, false, 0, 0, 0);

	auto GoalLoc = LineEnd;

	auto* PrimComp = Target->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return;

	auto TargetLoc = Target->GetActorLocation();
	auto Delta = GoalLoc - TargetLoc;

	FVector NewLoc = FMath::VInterpTo(TargetLoc, GoalLoc, DeltaTime, DampingFactor);

	Target->SetActorLocation(NewLoc, true, nullptr, ETeleportType::ResetPhysics);
	PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PrimComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
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

	FuseComp->SetConstrainedComponents(SourcePrim, NAME_None, DestPrim, NAME_None);

	FuseComp->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);

	//FuseComp->SetAngularVelocityDriveTwistAndSwing(true, true);
	//FuseComp->SetAngularVelocityTarget(FVector::Zero());

	//FuseComp->SetAngularTwistLimit( ACM_Limited, 1.0f);
	//FuseComp->SetAngularSwing1Limit(ACM_Limited, 1.0f);
	//FuseComp->SetAngularSwing2Limit(ACM_Limited, 1.0f);

	//const float Stiffness	 = 1000;
	//const float Damping		 = 5;
	//const float InForceLimit = 1000;
	//FuseComp->SetAngularDriveParams(Stiffness, Damping, InForceLimit);

	// FuseComp->SetLinearBreakable(true, 1000);
	// FuseComp->SetAngularBreakable(true, 1000);
	// FuseComp->SetAngularPlasticity(true, 50);
}