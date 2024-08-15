#pragma once

struct MyPhysics
{
	MyPhysics() = delete;

	static int AsyncSweepByObjectType(
					UWorld* World,
					EAsyncTraceType InTraceType,
					const FVector& Start,
					const FVector& End, 
					const FQuat& Rot, 
					const FCollisionObjectQueryParams& ObjectQueryParams, 
					const FBodyInstance* BodyInstance,
					const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, 
					const FTraceDelegate* InDelegate = nullptr, 
					uint32 UserData = 0);

	static int SweepSingleByObjectType(
					UWorld* World,
					struct FHitResult& OutHit,
					const FVector& Start, 
					const FVector& End, 
					const FQuat& Rot, 
					const FCollisionObjectQueryParams& ObjectQueryParams, 
					const FBodyInstance* BodyInstance,
					const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam);
};