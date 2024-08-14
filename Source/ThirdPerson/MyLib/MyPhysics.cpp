#include "MyPhysics.h"

#include "PhysicsEngine/BodySetup.h"
#include "Components/PrimitiveComponent.h"

int MyPhysics::BodyInstanceAsyncSweepByObjectType(
	UWorld* World,
	EAsyncTraceType InTraceType,
	const FVector& Start,
	const FVector& End, 
	const FQuat& Rot, 
	const FCollisionObjectQueryParams& ObjectQueryParams, 
	const FBodyInstance* Body,
	const FCollisionQueryParams& Params, 
	const FTraceDelegate* InDelegate, 
	uint32 UserData)
{
	if (!World)
		return 0;

	if (!Body)
		return 0;

	auto* BS = Body->GetBodySetup();
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

int MyPhysics::PrimitiveComponentAsyncSweepByObjectType(
	UWorld* World,
	EAsyncTraceType InTraceType,
	const FVector& Start,
	const FVector& End, 
	const FQuat& Rot, 
	const FCollisionObjectQueryParams& ObjectQueryParams, 
	const UPrimitiveComponent* PrimComp,
	const FCollisionQueryParams& Params, 
	const FTraceDelegate* InDelegate,
	uint32 UserData)
{
	if (!PrimComp)
		return 0;

	// The bounding box from Shape size might change base on rotation, just like AABB
	// therefore using BodyInstance instead
	// auto Shape = PrimComp->GetCollisionShape();

	auto* Body = PrimComp->GetBodyInstance();
	if (!Body)
		return 0;

	int SweepCount = BodyInstanceAsyncSweepByObjectType(
						World, InTraceType,
						Start, End, Rot,
						ObjectQueryParams, 
						Body,
						Params, 
						InDelegate, UserData);
	return SweepCount;
}