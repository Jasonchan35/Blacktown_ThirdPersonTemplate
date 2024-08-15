#include "MyUltraHandComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraActor.h"

#include "../MyCharacter.h"
#include "../MyPlayerController.h"
#include "MyFusedComponent.h"

FName UMyUltraHandComponent::NAME_Color(		TEXT("Color"));
FName UMyUltraHandComponent::NAME_BeamEnd(		TEXT("BeamEnd"));
FName UMyUltraHandComponent::NAME_BeamWidthMin(	TEXT("BeamWidthMin"));
FName UMyUltraHandComponent::NAME_BeamWidthMax(	TEXT("BeamWidthMax"));

UMyUltraHandComponent::UMyUltraHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
//	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	SearchTargetRadius  = 800;
	MaxFusableDistance  = 100;

	GrabTargetDistanceMin = 50;
	GrabTargetDistanceMax = 700;

	GrabTargetHeightMax   =  850;

	GrabTargetDampingFactor = 8;
	FuseTargetDampingFactor = 3;

	MY_CDO_FINDER(MI_GrabbableOverlay,	TEXT("/Game/ThirdPerson/Materials/MI_GrabbableOverlay"));
	MY_CDO_FINDER(MI_FusableOverlay,	TEXT("/Game/ThirdPerson/Materials/MI_FusableOverlay"));
	MY_CDO_FINDER(FXS_GrabTarget,		TEXT("/Game/ThirdPerson/VFX/FXS_GrabTarget"));

}

void UMyUltraHandComponent::IA_Confirm_Started()
{
	switch (Mode)
	{
		case EMyUltraHandMode::SearchTarget:	SetGrabTargetMode(); break;
		case EMyUltraHandMode::GrabTarget:		AcceptFusableTarget(); break;
	}
}

void UMyUltraHandComponent::IA_Cancel_Started()
{
	switch (Mode)
	{
//		case EMyUltraHandMode::SearchTarget:	SetAbilityActive(false); break;
		case EMyUltraHandMode::GrabTarget:		SetSearchTargetMode(); break;
	}
}

void UMyUltraHandComponent::IA_Break_Started()
{
	if (Mode != EMyUltraHandMode::GrabTarget)
		return;

	MyFuseHelper::BreakFusedActor(TargetActor.Get());
}

void UMyUltraHandComponent::SetTargetActor(AActor* Actor)
{
	auto* Cur = TargetActor.Get();
	if (Cur == Actor)
		return;

	if (Cur)
	{
		ResetSearchFusableTempOverlaps();
		MyFuseHelper::SetActorState(Cur, true, nullptr);
		EnableFx(GrabTargetFxActor, false);
		EnableFx(FusableFxActor, false);
	}

	TargetActor = Actor;
	Fusable.Reset();

	if (Actor)
		MyFuseHelper::SetActorState(Actor, true, MI_GrabbableOverlay);
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

void UMyUltraHandComponent::BeginPlay()
{
	Super::BeginPlay();

	GrabTargetFxActor = GetWorld()->SpawnActor<ANiagaraActor>();
	if (GrabTargetFxActor)
	{
		MyActorUtil::SetActorLabel(GrabTargetFxActor, "GrabTargetFx");
		GrabTargetFxActor->SetActorLocation(FVector(0, 0, 30));
		GrabTargetFxActor->AttachToActor(GetOwner(), FAttachmentTransformRules::KeepRelativeTransform);
		if (auto* Nia = GrabTargetFxActor->GetNiagaraComponent())
		{
			Nia->SetColorParameter(NAME_Color, FLinearColor(0.0f, 0.2f, 0.4f));
			Nia->SetFloatParameter(NAME_BeamWidthMin, 0.1f);
			Nia->SetFloatParameter(NAME_BeamWidthMax, 2);
			Nia->SetVisibility(false);
			Nia->SetAsset(FXS_GrabTarget);
		}
	}

	FusableFxActor = GetWorld()->SpawnActor<ANiagaraActor>();
	if (FusableFxActor)
	{
		MyActorUtil::SetActorLabel(GrabTargetFxActor, "FusableFx");
		FusableFxActor->AttachToActor(GetOwner(), FAttachmentTransformRules::KeepRelativeTransform);
		if (auto* Nia = FusableFxActor->GetNiagaraComponent())
		{
			Nia->SetColorParameter(NAME_Color, FLinearColor(0.0f, 0.8f, 0.0f));
			Nia->SetVisibility(false);
			Nia->SetAsset(FXS_GrabTarget);
		}
	}
}

void UMyUltraHandComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (GrabTargetFxActor)
		GrabTargetFxActor->Destroy();

	if (FusableFxActor)
		FusableFxActor->Destroy();
}

void UMyUltraHandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	switch (Mode)
	{
		case EMyUltraHandMode::SearchTarget:	TickSearchTarget(DeltaTime);	break;
		case EMyUltraHandMode::GrabTarget:		TickGrabTarget(DeltaTime);		break;
	}
}

void UMyUltraHandComponent::IA_DPad_Triggered(const FVector2D& Value)
{
	switch (Mode)
	{
		case EMyUltraHandMode::GrabTarget: MoveTargetForward(Value.Y); break;
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

void UMyUltraHandComponent::SetGrabTargetMode()
{
	Mode = EMyUltraHandMode::GrabTarget;
	Fusable.Reset();

	auto* Target = TargetActor.Get();
	auto* Ch = GetMyCharacter();
	auto* PC = GetMyPlayerController();

	if (!Target || !Ch || !PC)
	{
		SetSearchTargetMode();
		return;
	}

	MyFuseHelper::SetActorState(Target, false, MI_GrabbableOverlay);

	auto Rot = PC->GetControlRotation();

	auto Yaw  = Rot.Yaw;
	auto InvQuat = FRotator(0, Yaw, 0).Quaternion().Inverse();
	GrabTargetQuat	= InvQuat * Target->GetActorQuat();

	auto TargetLoc	= Target->GetActorLocation();
	auto ChLoc		= Ch->GetActorLocation();

	GrabTargetVector = InvQuat * (TargetLoc - ChLoc);
}

void UMyUltraHandComponent::TickGrabTarget(float DeltaTime)
{
	if (!DoTickGrabTarget(DeltaTime))
		SetSearchTargetMode();
}

bool UMyUltraHandComponent::DoTickGrabTarget(float DeltaTime)
{
	if (!MoveTargetLocation(DeltaTime))
		return false;

	if (!DoSearchFusable())
		return false;
		
	//if (Fusable.IsValid())
	//	DrawDebugLine(GetWorld(), Fusable.SourcePoint, Fusable.ImpactPoint, FColor::Red);
	EnableFx(FusableFxActor, Fusable.IsValid(), Fusable.SourcePoint, Fusable.ImpactPoint);

	return true;
}

bool UMyUltraHandComponent::AcceptFusableTarget()
{
	auto* Target = TargetActor.Get();

	if (!Fusable.IsValid() || !Target)
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

void UMyUltraHandComponent::EnableFx(ANiagaraActor* NiagaraActor, bool Enable, const FVector& Start, const FVector& End)
{
	if (!NiagaraActor)
		return;

	auto* Nia = NiagaraActor->GetNiagaraComponent();
	if (!Nia)
		return;

	if (!Enable)
	{
		Nia->Deactivate();
		Nia->SetVisibility(false);
	}
	else
	{
		NiagaraActor->SetActorLocation(Start);
		Nia->SetVectorParameter(NAME_BeamEnd, End);

		if (!Nia->IsVisible())
		{
			Nia->ReinitializeSystem();
			Nia->Activate();
			Nia->SetVisibility(true);
		}
	}
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

	auto StartLoc = Ch->GetActorLocation() + FVector(0, 0, 40);
	auto TargetLoc  = Target->GetActorLocation();
	auto TargetQuat = Target->GetActorQuat();

	auto DeltaZ = TargetLoc.Z - StartLoc.Z;
	auto DistXY = FVector::DistXY(TargetLoc, StartLoc);

	if (DistXY < GrabTargetDistanceMin - 10) return false;
	if (DistXY > GrabTargetDistanceMax + 10) return false;
	if (DeltaZ < -GrabTargetHeightMax  - 10) return false;
	if (DeltaZ >  GrabTargetHeightMax  + 10) return false;

	auto ControlRot = PC->GetControlRotation();
	ControlRot.Pitch = FMath::ClampAngle(ControlRot.Pitch, -80, 80);

	auto Vector = GrabTargetVector;

	Vector.Z = FMath::Tan(FMath::DegreesToRadians(ControlRot.Pitch)) * GrabTargetVector.X;

	Vector.X = FMath::Clamp(Vector.X, GrabTargetDistanceMin, GrabTargetDistanceMax);
	Vector.Z = FMath::Clamp(Vector.Z, -GrabTargetHeightMax, GrabTargetHeightMax);
	Vector.Z += 80; // add some extra Z to life object above the ground

	auto GoalQuat = FRotator(0, ControlRot.Yaw, 0).Quaternion() * GrabTargetQuat;

	auto GoalLoc  = StartLoc + FRotator(0, ControlRot.Yaw, 0).RotateVector(Vector);
	GoalLoc = FMath::VInterpTo(TargetLoc, GoalLoc, DeltaTime, GrabTargetDampingFactor);

	FVector MoveVector = GoalLoc - TargetLoc;

	// DrawDebugLine(GetWorld(), StartLoc, GoalLoc, FColor::Green, false, 0, 0, 0);
	EnableFx(GrabTargetFxActor, true, StartLoc, GoalLoc);

	auto& AsyncData = MoveTargetAsyncData;
	AsyncData.Reset();
	AsyncData.MoveDistance		= MoveVector.Length();
	AsyncData.HitMinDistance	= AsyncData.MoveDistance;
	AsyncData.Direction			= MoveVector.GetSafeNormal();
	AsyncData.RelativeQuat		= TargetQuat.Inverse() * GoalQuat;

	if (FMath::IsNearlyZero(AsyncData.MoveDistance))
		return true;

	FCollisionQueryParams	QueryParams;
	MyFuseHelper::VisitMembers(Target, [&](AActor* Member)
	{
		QueryParams.AddIgnoredActor(Member);
	});

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	auto* World = GetWorld();

	if (!MoveTargetAsyncDelegate.IsBound())
		MoveTargetAsyncDelegate.BindUObject(this, &ThisClass::MoveTargetAsyncResult);

	auto TraceStart = TargetLoc + AsyncData.Direction * 20;
	auto TraceEnd   = TargetLoc + AsyncData.Direction * (AsyncData.MoveDistance + 80);
	AsyncData.SweepCount += MyFuseHelper::AsyncSweepByObjectType(	Target, EAsyncTraceType::Single,
																	TraceStart, TraceEnd, GoalQuat, 
																	ObjectQueryParams, QueryParams, 
																	&MoveTargetAsyncDelegate);
	return true;
}

void UMyUltraHandComponent::MoveTargetAsyncResult(const FTraceHandle& TraceHandle, FTraceDatum& Data)
{
	auto& AsyncData = MoveTargetAsyncData;

	for (auto& Hit : Data.OutHits)
	{
		if (AsyncData.HitMinDistance > Hit.Distance)
		{
			AsyncData.HitCount++;
			AsyncData.HitMinDistance  = Hit.Distance;
			AsyncData.HitImpactNormal = Hit.ImpactNormal;
		}
	}

	AsyncData.SweepCount--;
	if (AsyncData.SweepCount == 0)
		MoveTargetAsyncCompleted();
}

void UMyUltraHandComponent::MoveTargetAsyncCompleted()
{
	if (Mode != EMyUltraHandMode::GrabTarget)
		return;

	auto* Target = TargetActor.Get();
	if (!Target)
		return;

	auto& AsyncData = MoveTargetAsyncData;

	auto RemainDistance = AsyncData.MoveDistance - AsyncData.HitMinDistance;

	auto NewTran = Target->GetActorTransform();
	auto NewLoc = NewTran.GetLocation() + AsyncData.Direction * AsyncData.HitMinDistance;
	NewTran.SetLocation(NewLoc);
	NewTran.SetRotation(NewTran.GetRotation() * AsyncData.RelativeQuat);

#if 1 // Sliding Along the hit plane
	if (AsyncData.HitCount > 0 && RemainDistance > 10)
	{
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

		FCollisionQueryParams	QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		MyFuseHelper::VisitMembers(Target, [&](AActor* Member)
		{
			QueryParams.AddIgnoredActor(Member);
		});

		auto N   = AsyncData.HitImpactNormal;
		auto Dir = AsyncData.Direction;
		auto NewDir = Dir - Dir.Dot(N) * N;
	
		auto NewStart = NewLoc + NewDir * 10;
		auto NewEnd   = NewLoc + NewDir * (RemainDistance + 50);
		auto NewRot   = NewTran.GetRotation();
		
		FHitResult HitResult;
		int NewHitCount = MyFuseHelper::SweepSingleByObjectType(
												Target,
												HitResult, 
												NewStart, NewEnd, NewRot,
												ObjectQueryParams,
												QueryParams);

		auto NewDistance = FMath::Min(RemainDistance, HitResult.Distance);
		auto FinalLoc = NewStart + NewDir * NewDistance;
		NewTran.SetLocation(FinalLoc);
	}
#endif

	MyFuseHelper::SetActorTransform(Target, NewTran);
}

void UMyUltraHandComponent::MoveTargetForward(float V)
{
	auto& X = GrabTargetVector.X;
	X = FMath::Clamp(X + V * 10, GrabTargetDistanceMin, GrabTargetDistanceMax);
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
	MyFuseHelper::VisitMembers(Target, [&](AActor* Member)
	{
		QueryParams.AddIgnoredActor(Member);
	});

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

//---- check overlapped actor within radius
	{
		ResetSearchFusableTempOverlaps();
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

			MyActorUtil::SetOverlayMaterial(SourceActor, MI_FusableOverlay);

			auto* Prim = SourceActor->FindComponentByClass<UPrimitiveComponent>();
			if (!Prim)
				continue;

			auto* BodyInstance = Prim->GetBodyInstance();
			if (!BodyInstance)
				continue;

			auto SweepStart = SourceActor->GetActorLocation();
			auto SweepRot	= SourceActor->GetActorQuat();

			QueryParams.ClearIgnoredActors();
			MyFuseHelper::VisitMembers(SourceActor, [&](AActor* Member)
			{
				QueryParams.AddIgnoredActor(Member);
			});

			// DrawDebugLine(GetWorld(), SweepStart, SweepEnd, FColor::Cyan);

			// Sweep nearby objects against Target Actor or Group
			MyPhysics::AsyncSweepByObjectType(
							GetWorld(),
							EAsyncTraceType::Single,
							SweepStart, SweepEnd, SweepRot,
							ObjectQueryParams,
							BodyInstance,
							QueryParams,
							&SearchFusableAsyncDelegate,
							i);
		}
	}
	return true;
}

void UMyUltraHandComponent::ResetSearchFusableTempOverlaps()
{
	for (auto& OverlapResult : SearchFusableTempOverlaps)
		MyActorUtil::SetOverlayMaterial(OverlapResult.GetActor(), nullptr);
	SearchFusableTempOverlaps.Reset();
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

	auto* TargetGroup = MyFuseHelper::FindGroup(Target);

	auto& AsyncData = SearchFusableAsyncData;

	for (auto& Hit : Data.OutHits)
	{
		if (Hit.Distance >= AsyncData.Distance)
			continue;
		
		if (!Hit.GetActor())
			continue;

		// Hit Target or it's group
		if (Target != Hit.GetActor())
		{
			if (TargetGroup != MyFuseHelper::FindGroup(Hit.GetActor()))
				continue;
		}

		AsyncData.SourceActor	= Overlap.GetActor();
		AsyncData.DestActor		= Hit.GetActor();
		AsyncData.Location		= Hit.Location;
		AsyncData.Distance		= Hit.Distance;

		AsyncData.SourcePoint	= Hit.ImpactPoint - Hit.Location + Hit.TraceStart;
		AsyncData.ImpactPoint	= Hit.ImpactPoint;
	}
}