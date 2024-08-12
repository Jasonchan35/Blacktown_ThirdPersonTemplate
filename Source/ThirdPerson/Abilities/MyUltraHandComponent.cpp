#include "MyUltraHandComponent.h"
#include "../MyCharacter.h"
#include "../MyPlayerController.h"
#include "MyFusedComponent.h"

UMyUltraHandComponent::UMyUltraHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	
	SearchTargetRadius  = 800;
	SearchFusableRadius = 100;

	HoldTargetDistanceMin = 50;
	HoldTargetDistanceMax = 700;

	HoldTargetHeightMax   =  850;

	HoldTargetDampingFactor = 8;
	FuseTargetDampingFactor = 3;

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

void UMyUltraHandComponent::IA_Cancel_Started()
{
	switch (Mode)
	{
		case EMyUltraHandMode::SearchTarget:	SetAbilityActive(false); break;
		case EMyUltraHandMode::HoldTarget:		SetSearchTargetMode(); break;
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
			SetActorState(Cur, true, nullptr);

		MyActorUtil::SetOverlayMaterial(Cur, nullptr);
	}

	TargetActor = Actor;
	Fusable.Reset();

	if (Actor)
		SetActorState(Actor, true, MI_SelectionOverlay);
}

void UMyUltraHandComponent::SetActorState(AActor* InActor, bool bGravity, UMaterialInterface* OverlayMaterial)
{
	if (!InActor)
		return;

	auto Func = [&](AActor* Actor) {
		if (!Actor)
			return;
		if (auto* Prim = Actor->FindComponentByClass<UPrimitiveComponent>())
		{
			Prim->SetEnableGravity(bGravity);
			Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

			MyActorUtil::SetOverlayMaterial(Actor, OverlayMaterial);
		}
	};

	Func(InActor);

	auto* Group = MyFuseHelper::FindGroup(InActor);
	if (!Group)
		return;

	for (auto* Member : Group->GetMembers())
	{
		if (Member)
			Func(Member->GetOwner());
	}
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

#if 1 // Debug
	MY_LOG_ON_SCREEN(L"UltraHand Mode='{}'", Mode);

	if (TargetActor.Get())
	{
		if (auto* Ch = GetMyCharacter())
		{
			DrawDebugLine(GetWorld(),
				Ch->GetActorLocation(),
				TargetActor->GetActorLocation(),
				FColor::Green);
		}
	}
#endif

	switch (Mode)
	{
		case EMyUltraHandMode::SearchTarget:	TickSearchTarget(DeltaTime);	break;
		case EMyUltraHandMode::HoldTarget:		TickHoldTarget(DeltaTime);		break;
		case EMyUltraHandMode::FuseTarget:		TickFuseTarget(DeltaTime);		break;
	}
}

void UMyUltraHandComponent::IA_DPad_Triggered(const FVector2D& Value)
{
	switch (Mode)
	{
		case EMyUltraHandMode::HoldTarget: AddHoldTargetRelativeLocation(Value); break;
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
	auto LineEnd   = LineStart + PC->GetControlRotation().Vector() * SearchTargetRadius;

	if (!SearchTargetAsyncTraceDelegate.IsBound())
		SearchTargetAsyncTraceDelegate.BindUObject(this, &ThisClass::SearchTargetAsyncTraceResult);

	QueryParams.ClearIgnoredActors();
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
	SetTargetActor(Cast<AActor>(Actor));
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

	SetActorState(Target, false, MI_SelectionOverlay);

	auto Rot = PC->GetControlRotation();

	auto Yaw  = Rot.Yaw;
	auto InvQuat = FRotator(0, Yaw, 0).Quaternion().Inverse();
	HoldTargetQuat	= InvQuat * Target->GetActorQuat();

	auto TargetLoc	= Target->GetActorLocation();
	auto ChLoc		= Ch->GetActorLocation();

	HoldTargetVector = InvQuat * (TargetLoc - ChLoc);
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

// Move TargetActor to Hit Fusable Actor

	auto Direction = (Fusable.ImpactPoint - Fusable.SourcePoint).GetSafeNormal();

	auto TargetLoc = Target->GetActorLocation();
	auto GoalLoc   = Fusable.GoalLocation + Direction * 50; // Move a little bit more to hit DestActor

	FVector NewLoc  = FMath::VInterpTo(TargetLoc, GoalLoc, DeltaTime, FuseTargetDampingFactor);
	FQuat   NewQuat = Target->GetActorQuat();

	FHitResult Hit;
	auto bHit = Target->SetActorLocationAndRotation(NewLoc, NewQuat, true, &Hit, ETeleportType::ResetPhysics);
	PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PrimComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	if (!bHit || Hit.GetActor() != Fusable.DestActor)
		return true; // Not hit yet or hit something else

//----
	MyFuseHelper::FuseActors(	Fusable.SourceActor.Get(),
								Fusable.DestActor.Get(),
								Fusable.ImpactPoint);

	Fusable.Reset();

	SetAbilityActive(false);

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

	auto ChLoc = Ch->GetActorLocation();
	auto TargetLoc = Target->GetActorLocation();

	auto DeltaZ = TargetLoc.Z - ChLoc.Z;
	auto DistXY = FVector::DistXY(TargetLoc, ChLoc);

	if (DistXY < HoldTargetDistanceMin - 10) return false;
	if (DistXY > HoldTargetDistanceMax + 10) return false;
	if (DeltaZ < -HoldTargetHeightMax  - 10) return false;
	if (DeltaZ >  HoldTargetHeightMax  + 10) return false;

	auto ControlRot = PC->GetControlRotation();
	ControlRot.Pitch = FMath::ClampAngle(ControlRot.Pitch, -80, 80);

	auto Vector = HoldTargetVector;

	Vector.Z = FMath::Tan(FMath::DegreesToRadians(ControlRot.Pitch)) * HoldTargetVector.X;

	Vector.X = FMath::Clamp(Vector.X, HoldTargetDistanceMin, HoldTargetDistanceMax);
	Vector.Z = FMath::Clamp(Vector.Z, -HoldTargetHeightMax, HoldTargetHeightMax);
	Vector.Z += 60; // add some extra Z to life object above the ground

	auto GoalLoc   = ChLoc + FRotator(0, ControlRot.Yaw, 0).RotateVector(Vector);

	DrawDebugLine(GetWorld(), ChLoc, GoalLoc, FColor::Green, false, 0, 0, 0);

	FVector NewLoc  = FMath::VInterpTo(TargetLoc, GoalLoc, DeltaTime, HoldTargetDampingFactor);
	FQuat   NewQuat = ControlRot.Quaternion() * HoldTargetQuat;

	Target->SetActorLocationAndRotation(NewLoc, NewQuat, true, nullptr, ETeleportType::ResetPhysics);
	if (auto* PrimComp = Target->FindComponentByClass<UPrimitiveComponent>())
	{
		PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		PrimComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	return true;
}

void UMyUltraHandComponent::AddHoldTargetRelativeLocation(const FVector2D& Value)
{
	auto& X = HoldTargetVector.X;
	X = FMath::Clamp(X + Value.Y * 10, HoldTargetDistanceMin, HoldTargetDistanceMax);
}

bool UMyUltraHandComponent::DoSearchFusable()
{
	Fusable = SearchFusableAsyncSweep;
	SearchFusableAsyncSweep.Reset();

	SearchFusableAsyncSweep.Distance = SearchFusableRadius;

	auto* Target = TargetActor.Get();
	if (!Target)
		return false;

	auto* TargetPrimComp = Target->FindComponentByClass<UPrimitiveComponent>();
	if (!TargetPrimComp)
		return false;

	QueryParams.ClearIgnoredActors();

	if (auto* Group = MyFuseHelper::FindGroup(Target))
	{
		for(auto& Member : Group->GetMembers())
		{
			if (Member)
				QueryParams.AddIgnoredActor(Member->GetOwner());
		}
	}
	else
	{
		QueryParams.AddIgnoredActor(Target);
	}
	QueryParams.AddIgnoredActor(GetOwner());



	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

//---- check overlapped actor within radius
	{
		float SphereRadius = 50 + SearchFusableRadius + MyActorUtil::GetBoundingSphereRadius(Target, true);

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
		auto* Actor = Cast<AActor>(Hit.GetActor());

		if (Hit.Distance > Out.Distance)
			continue;

		Out.DestActor		= Actor;

		Out.GoalLocation	= Hit.Location;
		Out.Distance		= Hit.Distance;

		Out.SourcePoint		= Hit.ImpactPoint - Hit.Location + Hit.TraceStart;
		Out.ImpactPoint		= Hit.ImpactPoint;
	}
}

void UMyUltraHandComponent::SetFuseTargetMode()
{
	if (!Fusable)
		return;

	Mode = EMyUltraHandMode::FuseTarget;
	FuseTargetTimeRemain = 1; // try to fuse object within some time
}