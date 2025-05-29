#pragma once

#include "ProjectFlyReborn/Public/Interface/FlightMouseAimInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlyingPawn.generated.h"

UCLASS()
class PROJECTFLYREBORN_API AFlyingPawn : public APawn, public IFlightMouseAimInterface
{
	GENERATED_BODY()

public:
	AFlyingPawn();

	virtual FVector GetTargetAimWorldLocation() const override;
	virtual FVector GetCurrentDirection() const override;
	virtual FRotator GetCurrentRotation() const override;
	virtual void SetDesiredDirection(FVector WorldDirection) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(EditAnywhere)
	class UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere)
	class USpringArmComponent* SpringArm;

	UPROPERTY(EditAnywhere)
	class UCameraComponent* Camera;

	// Input variables
	float CameraYaw;
	float CameraPitch;

	// Sensitivity
	UPROPERTY(EditAnywhere)
	float MouseSensitivity = 1.0f;

	FVector DesiredDirection;

	// Mouse input handlers
	void LookUp(float Value);
	void Turn(float Value);
};