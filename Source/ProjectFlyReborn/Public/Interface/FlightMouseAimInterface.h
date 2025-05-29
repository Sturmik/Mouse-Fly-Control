#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FlightMouseAimInterface.generated.h"

UINTERFACE(MinimalAPI)
class UFlightMouseAimInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTFLYREBORN_API IFlightMouseAimInterface
{
	GENERATED_BODY()

public:
	// Reticle location in world (projected from mouse)
	virtual FVector GetTargetAimWorldLocation() const = 0;

	// Current aircraft direction
	virtual FVector GetCurrentDirection() const = 0;

	// Mesh or actor root for orientation
	virtual FRotator GetCurrentRotation() const = 0;

	// Called when reticle moves
	virtual void SetDesiredDirection(FVector WorldDirection) = 0;
};