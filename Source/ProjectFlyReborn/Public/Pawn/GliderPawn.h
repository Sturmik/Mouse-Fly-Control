#pragma once

#include "ProjectFlyReborn/Public/Interface/FlightMouseAimInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GliderPawn.generated.h"

UCLASS()
class PROJECTFLYREBORN_API AGliderPawn : public APawn, public IFlightMouseAimInterface
{
	GENERATED_BODY()

public:
	AGliderPawn();

	virtual FVector GetTargetAimWorldLocation() const override;
	virtual FVector GetCurrentDirection() const override;
	virtual FRotator GetCurrentRotation() const override;
	virtual void SetDesiredDirection(FVector WorldDirection) override;

	virtual void Tick(float DeltaTime) override;

	void AddSpeed(float Speed);

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	// Calculates change of speed from inclination 
	void CalculateSpeed(float DeltaTime);

	UPROPERTY(EditAnywhere)
	class UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere)
	class USpringArmComponent* SpringArm;

	UPROPERTY(EditAnywhere)
	class UCameraComponent* Camera;

	// Input variables
	float CameraYaw;
	float CameraPitch;

	FVector DesiredDirection;

	UPROPERTY(EditAnywhere)
	float ThrustForce = 5000.0f;

	UPROPERTY(EditAnywhere)
	FVector TurnTorque = FVector(45.f, 25.f, 45.f);

	UPROPERTY(EditAnywhere)
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere)
	float TurnAngleSensitivity = 1.0f;

	UPROPERTY(EditAnywhere)
	float AggressiveTurnAngle = 10.0f;

	UPROPERTY(EditAnywhere)
	float LiftCoefficientPitchScalar = 0.2f;

	UPROPERTY(EditAnywhere)
	float LiftCoefficientScalar = 30.0f;

	// Negative value allows plane to move backwards, thus imitating reversed gliding
	float MinimumPlaneSpeed = -100.0f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0f))
	float MaximumPlaneSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0f))
	float StartPlaneSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Plane Control", meta = (ClampMin = 0.0f))
	float DiveSpeedIncreaseScalar = 4.0f;

	UPROPERTY(EditAnywhere,Category = "Plane Control", meta = (ClampMin = 0.0f))
	float RiseSpeedDecreaseScalar = 6.0f;

	float AirControl = 0.0f;

	// Forward speed of the plane
	// This is the main variable, which defines speed of the plane 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float ForwardSpeed = 0.0f;

	// Mouse input handlers
	void LookUp(float Value);
	void Turn(float Value);

	void RunAutopilot(const FVector& FlyTarget, float& OutYaw, float& OutPitch, float& OutRoll);
};
