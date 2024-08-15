#include "MyPhysics.h"

#include "PhysicsEngine/BodySetup.h"
#include "Components/PrimitiveComponent.h"

int MyPhysics::AsyncSweepByObjectType(
					UWorld* World,
					EAsyncTraceType InTraceType,
					const FVector& Start,
					const FVector& End, 
					const FQuat& Rot, 
					const FCollisionObjectQueryParams& ObjectQueryParams, 
					const FBodyInstance* BodyInstance,
					const FCollisionQueryParams& Params, 
					const FTraceDelegate* InDelegate, 
					uint32 UserData)
{
	if (!World)
		return 0;

	if (!BodyInstance)
		return 0;

	auto* BS = BodyInstance->GetBodySetup();
	if (!BS)
		return 0;

	int SweepCount = 0;
	for (auto& Box : BS->AggGeom.BoxElems) {
		auto BoxLoc = Rot.RotateVector(Box.Center);
		auto BoxRot = Rot * Box.Rotation.Quaternion();
		auto BoxExtent = FVector(Box.X, Box.Y, Box.Z) * 0.5;

		auto TraceStart = Start + BoxLoc;
		auto TraceEnd   = End   + BoxLoc;

		//DrawDebugLine(World, TraceStart, TraceEnd, FColor::Yellow);
		//DrawDebugBox(World,  TraceStart, BoxExtent, BoxRot, FColor::Yellow);

		World->AsyncSweepByObjectType(
					InTraceType, 
					TraceStart, TraceEnd, BoxRot,
					ObjectQueryParams,
					FCollisionShape::MakeBox(BoxExtent),
					Params, 
					InDelegate, UserData);
		SweepCount++;
	}

	// TODO - Support more shape other than Box

	return SweepCount;
}

int MyPhysics::SweepSingleByObjectType(
				UWorld* World,
				struct FHitResult& OutHit,
				const FVector& Start, 
				const FVector& End, 
				const FQuat& Rot, 
				const FCollisionObjectQueryParams& ObjectQueryParams, 
				const FBodyInstance* BodyInstance,
				const FCollisionQueryParams& Params)
{
	if (!World)
		return 0;

	if (!BodyInstance)
		return 0;

	auto* BS = BodyInstance->GetBodySetup();
	if (!BS)
		return 0;

	OutHit.Distance = FVector::Distance(Start, End);
	int HitCount = 0;

	for (auto& Box : BS->AggGeom.BoxElems) {
		auto BoxLoc = Rot.RotateVector(Box.Center);
		auto BoxRot = Rot * Box.Rotation.Quaternion();
		auto BoxExtent = FVector(Box.X, Box.Y, Box.Z) * 0.5;

		auto TraceStart = Start + BoxLoc;
		auto TraceEnd   = End   + BoxLoc;

		//DrawDebugLine(World, TraceStart, TraceEnd, FColor::Yellow);
		//DrawDebugBox(World,  TraceStart, BoxExtent, BoxRot, FColor::Yellow);

		FHitResult TempHitResult;
		bool Hit = World->SweepSingleByObjectType(
						TempHitResult,
						TraceStart, TraceEnd, BoxRot,
						ObjectQueryParams,
						FCollisionShape::MakeBox(BoxExtent),
						Params);
		if (!Hit)
			continue;

		HitCount++;

		if (OutHit.Distance > TempHitResult.Distance)
			OutHit = TempHitResult;
	}

	// TODO - Support more shape other than Box

	return HitCount;
}

