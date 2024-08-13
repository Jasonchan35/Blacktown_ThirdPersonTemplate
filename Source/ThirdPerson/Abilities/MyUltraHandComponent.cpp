#include "MyUltraHandComponent.h"
#include "../MyCharacter.h"
#include "../MyPlayerController.h"
#include "MyFusedComponent.h"

UMyUltraHandComponent::UMyUltraHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
//	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	SearchTargetRadius  = 800;
	MaxFusableDistance  = 100;

	HoldTargetDistanceMin = 50;
	HoldTargetDistanceMax = 700;

	HoldTargetHeightMax   =  850;

	HoldTargetDampingFactor = 8;
	FuseTargetDampingFactor = 3;

	MY_CDO_FINDER(MI_SelectionOverlay, TEXT("/Game/ThirdPerson/Materials/M_SelectionOverlay"));
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
//		case EMyUltraHandMode::SearchTarget:	SetAbilityActive(false); break;
		case EMyUltraHandMode::HoldTarget:		SetSearchTargetMode(); break;
	}
}

void UMyUltraHandComponent::SetTargetActor(AActor* Actor)
{
	auto* Cur = TargetActor.Get();
	if (Cur == Actor)
		return;

	if (Cur)
		MyFuseHelper::SetActorState(Cur, true, nullptr);

	TargetActor = Actor;
	Fusable.Reset();

	if (Actor)
		MyFuseHelper::SetActorState(Actor, true, MI_SelectionOverlay);
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
		case EMyUltraHandMode::HoldTarget: MoveTargetForward(Value.Y); break;
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

	MyFuseHelper::SetActorState(Target, false, MI_SelectionOverlay);

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
	if (!MoveTargetLocation(DeltaTime))
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
	auto* Target = TargetActor.Get();

	if (!Fusable || !Target)
		return false;

	auto* PrimComp = Target->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return false;

	MyFuseHelper::FuseActors(	Fusable.SourceActor.Get(),
								Fusable.SourcePoint,
								Fusable.DestActor.Get(),
								Fusable.ImpactPoint);

	Fusable.Reset();

	SetSearchTargetMode();

	return true;
}

bool UMyUltraHandComponent::MoveTargetLocation(float DeltaTime)
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
	FVector MoveVector = NewLoc - TargetLoc;

	auto& AsyncData = MoveTargetAsyncData;

	AsyncData.Count			= 0;
	AsyncData.MinDistance	= MoveVector.Length();
	AsyncData.Direction		= MoveVector.GetSafeNormal();
	AsyncData.Quat			= ControlRot.Quaternion() * HoldTargetQuat;

	FCollisionQueryParams	QueryParams;
	QueryParams.AddIgnoredActor(Target);

	auto* Group = MyFuseHelper::FindGroup(Target);
	if (Group)
	{
		for (auto& Member : Group->GetMembers())
		{
			if (Member && Member != Target)
				QueryParams.AddIgnoredActor(Member);
		}
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	auto* World = GetWorld();

	if (!MoveTargetAsyncDelegate.IsBound())
		MoveTargetAsyncDelegate.BindUObject(this, &ThisClass::MoveTargetAsyncResult);

	auto AsyncSweep = [&](AActor* Actor, const FTransform& Tran) -> void
	{
		auto* Prim = Actor->FindComponentByClass<UPrimitiveComponent>();
		if (!Prim)
			return;

		World->AsyncSweepByObjectType(	EAsyncTraceType::Single,
										Tran.GetLocation() + AsyncData.Direction * 10,
										Tran.GetLocation() + AsyncData.Direction * (AsyncData.MinDistance + 10),
										Tran.GetRotation(),
										ObjectQueryParams,
										Prim->GetCollisionShape(),
										QueryParams,
										&MoveTargetAsyncDelegate);

		AsyncData.Count++;
	};

	auto& TargetTran = Target->GetActorTransform();
	auto NewTran = TargetTran;
	NewTran.SetLocation(NewLoc);

	auto InvTargetTran = TargetTran.Inverse();
	auto ReletiveTran = InvTargetTran * NewTran;

	AsyncSweep(Target, NewTran);

	if (Group)
	{
		for (auto& Member : Group->GetMembers())
		{
			if (Member && Member != Target)
				AsyncSweep(Member, Member->GetActorTransform() * ReletiveTran);
		}
	}

	return true;
}

void UMyUltraHandComponent::MoveTargetAsyncResult(const FTraceHandle& TraceHandle, FTraceDatum& Data)
{
	auto& AsyncData = MoveTargetAsyncData;

	for (auto& Hit : Data.OutHits)
	{
		AsyncData.MinDistance = FMath::Min(Hit.Distance, AsyncData.MinDistance);
	}

	AsyncData.Count--;
	if (AsyncData.Count != 0)
		return;

	if (Mode != EMyUltraHandMode::HoldTarget)
		return;

	auto* Target = TargetActor.Get();
	if (!Target)
		return;

	if (FMath::IsNearlyZero(AsyncData.MinDistance))
		return;

	auto NewTran = Target->GetActorTransform();
	NewTran.SetLocation(NewTran.GetLocation() + AsyncData.Direction * AsyncData.MinDistance);
	NewTran.SetRotation(AsyncData.Quat);

	MyFuseHelper::SetActorTransform(Target, NewTran);
}

void UMyUltraHandComponent::MoveTargetForward(float V)
{
	auto& X = HoldTargetVector.X;
	X = FMath::Clamp(X + V * 10, HoldTargetDistanceMin, HoldTargetDistanceMax);
}

bool UMyUltraHandComponent::DoSearchFusable()
{
	Fusable = SearchFusableAsyncData;
	SearchFusableAsyncData.Reset();

	SearchFusableAsyncData.Distance = MaxFusableDistance;

	auto* Target = TargetActor.Get();
	if (!Target)
		return false;

	FCollisionQueryParams	QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(Target);

	if (auto* Group = MyFuseHelper::FindGroup(Target))
	{
		for(auto& Member : Group->GetMembers())
		{
			if (Member && Member != Target)
				QueryParams.AddIgnoredActor(Member);
		}
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

//---- check overlapped actor within radius
	{
		SearchFusableTempOverlaps.Reset();
		bool overlapped = GetWorld()->OverlapMultiByObjectType(
											SearchFusableTempOverlaps,
											Target->GetActorLocation(),
											FQuat::Identity,
											ObjectQueryParams,
											FCollisionShape::MakeSphere(SearchTargetRadius),
											QueryParams);
		if (!overlapped)
			return true;
	}

//----- Find out actual impact point by Async Sweep from TargetActor to each overlapped actor
	{
		if (!SearchFusableAsyncDelegate.IsBound())
			SearchFusableAsyncDelegate.BindUObject(this, &ThisClass::SearchFusableAsyncResult);

		auto SweepEnd = Target->GetActorLocation();

		for (int i = 0; i < SearchFusableTempOverlaps.Num(); i++)
		{
			auto& Overlap = SearchFusableTempOverlaps[i];
			auto* SourceActor = Overlap.GetActor();

			if (!SourceActor)
				continue;

			auto* Prim = SourceActor->FindComponentByClass<UPrimitiveComponent>();
			if (!Prim)
				continue;

			auto SweepStart = SourceActor->GetActorLocation();
			auto SweepRot	= SourceActor->GetActorQuat();
			auto SweepShape	= Prim->GetCollisionShape();

			QueryParams.ClearIgnoredActors();
			QueryParams.AddIgnoredActor(SourceActor);

			// DrawDebugLine(GetWorld(), SweepStart, SweepEnd, FColor::Cyan);

			// Sweep nearby objects against Target Actor or Group
			GetWorld()->AsyncSweepByObjectType(
							EAsyncTraceType::Single,
							SweepStart, SweepEnd, SweepRot,
							ObjectQueryParams,
							SweepShape,
							QueryParams,
							&SearchFusableAsyncDelegate,
							i);
		}
	}
	return true;
}

void UMyUltraHandComponent::SearchFusableAsyncResult(const FTraceHandle& TraceHandle, FTraceDatum& Data)
{
	int Index = Data.UserData;
	if (Index < 0 || Index >= SearchFusableTempOverlaps.Num())
		return;

	auto& Overlap = SearchFusableTempOverlaps[Index];
	if (!Overlap.GetActor())
		return;

	auto* Target = TargetActor.Get();
	if (!Target)
		return;

	auto* Group = MyFuseHelper::FindGroup(Target);

	auto& AsyncData = SearchFusableAsyncData;

	for (auto& Hit : Data.OutHits)
	{
		if (Hit.Distance >= AsyncData.Distance)
			continue;
		
		auto* Actor = Hit.GetActor();

		if (Actor != Target && !MyFuseHelper::MatchGroup(Actor, Group))
			continue;

		AsyncData.SourceActor	= Overlap.GetActor();
		AsyncData.DestActor		= Actor;
		AsyncData.Location		= Hit.Location;
		AsyncData.Distance		= Hit.Distance;

		AsyncData.SourcePoint	= Hit.ImpactPoint - Hit.Location + Hit.TraceStart;
		AsyncData.ImpactPoint	= Hit.ImpactPoint;
	}
}

void UMyUltraHandComponent::SetFuseTargetMode()
{
	if (!Fusable)
		return;

	Mode = EMyUltraHandMode::FuseTarget;
}
