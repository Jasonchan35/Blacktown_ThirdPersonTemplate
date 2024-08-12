#include "MyUltraHandComponent.h"
#include "MyFuseComponent.h"
#include "../MyCharacter.h"
#include "../MyPlayerController.h"

UMyUltraHandComponent::UMyUltraHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	DampingFactor = 8;

	Fusable.Reset();
	SearchFusableRadius = 100;
	SearchFusableAsyncSweep.Reset();

	MY_CDO_FINDER(MI_SelectionOverlay,	TEXT("/Game/ThirdPerson/Materials/M_SelectionOverlay"));
}

void UMyUltraHandComponent::IA_Confirm_Started()
{
	switch (Mode)
	{
		case EMyUltraHandMode::SearchTarget:	SetHoldTargetMode(); break;
		case EMyUltraHandMode::HoldTarget:		SetFuseTargetMode(); break;
	}
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

		MyActorUtil::SetOverlayMaterial(Cur, nullptr);
	}

	TargetActor = Actor;
	Fusable.Reset();

	if (Actor)
		MyActorUtil::SetOverlayMaterial(Actor, MI_SelectionOverlay);
}

void UMyUltraHandComponent::SetAbilityActive(bool Active)
{
	if (Active)
	{
		SetSearchTargetMode();
	}
	else
	{
		Mode = EMyUltraHandMode::None;

		SetTargetActor(nullptr);
		Fusable.Reset();
		if (auto* Ch = GetMyCharacter())
			Ch->SetCurrentAbility(EMyAbility::None);
	}
}

void UMyUltraHandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	MY_LOG_ON_SCREEN(L"UltraHand Mode='{}'", Mode);

	switch (Mode)
	{
		case EMyUltraHandMode::SearchTarget:	TickSearchTarget(DeltaTime);	break;
		case EMyUltraHandMode::HoldTarget:		TickHoldTarget(DeltaTime);		break;
		case EMyUltraHandMode::FuseTarget:		TickFuseTarget(DeltaTime);		break;
	}
}

void UMyUltraHandComponent::SetSearchTargetMode()
{
	Mode = EMyUltraHandMode::SearchTarget;
	SetTargetActor(nullptr);
}

void UMyUltraHandComponent::TickSearchTarget(float DeltaTime)
{
	if (!DoTickSearchTarget(DeltaTime))
		SetAbilityActive(false);
}

bool UMyUltraHandComponent::DoTickSearchTarget(float DeltaTime)
{
	auto* Ch = GetMyCharacter();
	if (!Ch)
		return false;

	auto* PC = Ch->GetController<AMyPlayerController>();
	if (!PC)
		return false;

	auto CameraLoc = PC->GetCameraLocation();
	auto CrossHairPos = PC->GetCrossHairPos();

	auto LineStart = CameraLoc;
	auto LineEnd   = LineStart + PC->GetControlRotation().Vector() * 1000.0;

	if (!SearchTargetAsyncTraceDelegate.IsBound())
		SearchTargetAsyncTraceDelegate.BindUObject(this, &ThisClass::SearchTargetAsyncTraceResult);

	FCollisionQueryParams	QueryParams;
	QueryParams.AddIgnoredActor(Ch);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

//	DrawDebugLine(GetWorld(), LineStart, LineEnd, FColor::Red, false, 5.0f, 0, 0);

	GetWorld()->AsyncLineTraceByObjectType(	EAsyncTraceType::Single,
											LineStart,
											LineEnd,
											ObjectQueryParams,
											QueryParams,
											&SearchTargetAsyncTraceDelegate);

	return true;
}

void UMyUltraHandComponent::SearchTargetAsyncTraceResult(const FTraceHandle& TraceHandle, FTraceDatum& Data)
{
	auto* Actor = Data.OutHits.Num() > 0 ? Data.OutHits[0].GetActor() : nullptr;
	SetTargetActor(Actor);
}

void UMyUltraHandComponent::SetHoldTargetMode()
{
	Mode = EMyUltraHandMode::HoldTarget;
	Fusable.Reset();

	auto* Target = TargetActor.Get();
	auto* Ch = GetMyCharacter();
	auto* PC = GetMyPlayerController();

	if (!Target || !Ch || !PC)
	{
		SetSearchTargetMode();
		return;
	}

	auto Rot = PC->GetControlRotation();

	auto Yaw  = Rot.Yaw;
	auto InvQuat = FRotator(0, Yaw, 0).Quaternion().Inverse();
	HoldTargetRelativeQuat		= Target->GetActorQuat() * InvQuat;
	HoldTargetRelativeLocation	= Target->GetActorLocation() - Ch->GetActorLocation() + FVector(0, 0, 50);
}

void UMyUltraHandComponent::TickHoldTarget(float DeltaTime)
{
	if (!DoTickHoldTarget(DeltaTime))
		SetSearchTargetMode();
}

bool UMyUltraHandComponent::DoTickHoldTarget(float DeltaTime)
{
	if (!UpdateTargetActorLocation(DeltaTime))
		return false;

	if (!DoSearchFusable())
		return false;
		
	if (Fusable)
		DrawDebugLine(GetWorld(), Fusable.SourcePoint, Fusable.ImpactPoint, FColor::Red);

	return true;
}

void UMyUltraHandComponent::TickFuseTarget(float DeltaTime)
{
	if (!DoTickFuseTarget(DeltaTime))
		SetSearchTargetMode();
}

bool UMyUltraHandComponent::DoTickFuseTarget(float DeltaTime)
{
	FuseTargetTimeRemain -= DeltaTime;
	if (FuseTargetTimeRemain <= 0)
		return false;

	auto* Target = TargetActor.Get();

	if (!Fusable || !Target)
		return false;

	auto* PrimComp = Target->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return false;

	PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PrimComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PrimComp->SetEnableGravity(true);

	auto TargetLoc = Target->GetActorLocation();
	auto GoalLoc   = Fusable.GoalLocation;

	FVector NewLoc  = FMath::VInterpTo(TargetLoc, GoalLoc, DeltaTime, DampingFactor);
	FQuat   NewQuat = Target->GetActorQuat();

	FHitResult Hit;
	auto bHit = Target->SetActorLocationAndRotation(NewLoc, NewQuat, true, &Hit, ETeleportType::ResetPhysics);

	if (!bHit || Hit.GetActor() != Fusable.DestActor)
		return true; // Not hit or hit something else

	FuseFusableObject();

	return true;
}

bool UMyUltraHandComponent::UpdateTargetActorLocation(float DeltaTime)
{
	auto* Target = TargetActor.Get();
	if (!Target) // Lost Target
		return false;

	auto* Ch = GetMyCharacter();
	auto* PC = GetMyPlayerController();

	if (!PC || !Ch)
		return false;

	auto ControlRot = PC->GetControlRotation();

	auto Forward = FVector::ForwardVector;

	auto LineStart = Ch->GetActorLocation();
	auto LineEnd   = LineStart + FRotator(0, ControlRot.Yaw, 0).RotateVector(HoldTargetRelativeLocation);

	auto GoalLoc = LineEnd;

	auto Distance = FVector::Distance(GoalLoc, Target->GetActorLocation());
	const float DropTargetDistance = 300;
	if (Distance > DropTargetDistance)
		return false;

	DrawDebugLine(GetWorld(), LineStart, LineEnd, FColor::Green, false, 0, 0, 0);

	auto TargetLoc = Target->GetActorLocation();
	auto Delta = GoalLoc - TargetLoc;

	FVector NewLoc  = FMath::VInterpTo(TargetLoc, GoalLoc, DeltaTime, DampingFactor);
	FQuat   NewQuat = ControlRot.Quaternion() * HoldTargetRelativeQuat;

	Target->SetActorLocationAndRotation(NewLoc, NewQuat, true, nullptr, ETeleportType::ResetPhysics);
	if (auto* PrimComp = Target->FindComponentByClass<UPrimitiveComponent>())
	{
		PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		PrimComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		PrimComp->SetEnableGravity(false);
	}

	return true;
}

bool UMyUltraHandComponent::DoSearchFusable()
{
	Fusable = SearchFusableAsyncSweep;
	SearchFusableAsyncSweep.Reset();

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

//---- check overlapped actor within radius
	{
		float SphereRadius = SearchFusableRadius + MyActorUtil::GetBoundingSphereRadius(Target, true);

		SearchFusableTempOverlaps.Reset();
		bool overlapped = GetWorld()->OverlapMultiByObjectType(
											SearchFusableTempOverlaps,
											Target->GetActorLocation(),
											FQuat::Identity,
											ObjectQueryParams,
											FCollisionShape::MakeSphere(SphereRadius),
											QueryParams);
		if (!overlapped)
			return true;
	}

//----- Find out actual impact point by Async Sweep from TargetActor to each overlapped actor
	{
		SearchFusableAsyncSweep.SourceActor = TargetActor;

		if (!SearchFusableAsyncSweepDelegate.IsBound())
			SearchFusableAsyncSweepDelegate.BindUObject(this, &ThisClass::SearchFusableAsyncSweepResult);

		auto SweepStart	= Target->GetActorLocation();
		auto SweepRot	= Target->GetActorQuat();
		auto SweepShape	= TargetPrimComp->GetCollisionShape();

		for (auto& Overlap : SearchFusableTempOverlaps)
		{
			if (!Overlap.GetActor())
				continue;

			auto SweepEnd   = Overlap.GetActor()->GetActorLocation();

			GetWorld()->AsyncSweepByObjectType(
							EAsyncTraceType::Single,
							SweepStart, SweepEnd, SweepRot,
							ObjectQueryParams,
							SweepShape,
							QueryParams,
							&SearchFusableAsyncSweepDelegate);
		}
	}
	return true;
}

void UMyUltraHandComponent::SearchFusableAsyncSweepResult(const FTraceHandle& TraceHandle, FTraceDatum& Data)
{
	auto* Target = TargetActor.Get();
	if (!Target)
		return;

	auto& Out = SearchFusableAsyncSweep;

	if (Out.SourceActor != Target)
		return; // TargetActor changed during Async call, so drop it

	for (auto& Hit : Data.OutHits)
	{
		auto Dis			= Hit.Distance;
		auto MinDisSq		= Out.Distance;

		if (Out.DestActor.Get() && (Dis * Dis) > MinDisSq)
			continue;

		Out.DestActor		= Hit.GetActor();

		Out.GoalLocation	= Hit.Location;
		Out.Distance		= Dis;

		Out.SourcePoint		= Hit.ImpactPoint - Hit.Location + Hit.TraceStart;
		Out.ImpactPoint		= Hit.ImpactPoint;
	}
}

void UMyUltraHandComponent::SetFuseTargetMode()
{
	Mode = EMyUltraHandMode::FuseTarget;
	FuseTargetTimeRemain = 3; // try to fuse object within some time
}

void UMyUltraHandComponent::FuseFusableObject()
{
	auto* DestActor = Fusable.DestActor.Get();
	if (!DestActor)
		return;

	auto* SourceActor = Fusable.SourceActor.Get();
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

	FuseComp->SetWorldLocation(Fusable.SourcePoint);

	FuseComp->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0);
	FuseComp->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0);
	FuseComp->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0);

	FuseComp->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	FuseComp->SetAngularTwistLimit( EAngularConstraintMotion::ACM_Locked, 0);
	FuseComp->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
	FuseComp->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);

	FuseComp->SetConstrainedComponents(SourcePrim, NAME_None, DestPrim, NAME_None);
}
