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

	void AffectSpeed(float Speed);

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

	UPROPERTY(EditAnywhere, Category = "Glider Control - Turn Control", meta = (ClampMin = 0.0f))
	FVector TurnTorque = FVector(45.f, 25.f, 45.f);

	UPROPERTY(EditAnywhere, Category = "Glider Control - Turn Control", meta = (ClampMin = 0.0f))
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Turn Control", meta = (ClampMin = 0.0f))
	float TurnAngleSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Turn Control", meta = (ClampMin = 0.0f))
	float AggressiveTurnAngle = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Lift Control", meta = (ClampMin = 0.0f))
	float LiftCoefficientScalar = 0.004f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Lift Control", meta = (ClampMin = 0.0f))
	float MaxLiftForce = 20000.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Speed Control", meta = (ClampMin = 0.0f))
	float MinimumPlaneSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Speed Control", meta = (ClampMin = 0.0f))
	float MaximumPlaneSpeed = 20000.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Speed Control", meta = (ClampMin = 0.0f))
	float StartPlaneSpeed = 12000.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Speed Control", meta = (ClampMin = 0.0f))
	float DiveSpeedIncreaseScalar = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Speed Control", meta = (ClampMin = 0.0f))
	float RiseSpeedDecreaseScalar = 2500.0f;

	float AirControl = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Air Control", meta = (ClampMin = 0.0f))
	float MinimumAirControl = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Air Control", meta = (ClampMin = 0.0f))
	float MaximumAirControl = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Gravity Control", meta = (ClampMin = 0.0f))
	float GravityScalar = 2500.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Gravity Control", meta = (ClampMin = 0.0f))
	float GravityMultiplier = 1.0f; 

	// Forward speed of the plane
	// This is the main variable, which defines speed of the plane 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float ForwardSpeed = 0.0f;

	// Dash Settings
	FTimerHandle DashStopTimer;
	FTimerHandle DashCooldownTimer;

	bool bCanDash = true;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Dash")
	float DashSpeedCost = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Dash")
	float DashStrength = 200000.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Dash")
	float DashCooldown = 3.0f;

	// Halt Settings
	FTimerHandle HaltTimer;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Halt")
	float HaltSpeedCost = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Halt")
	float HaltDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Halt")
	float HaltCooldown = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Glider Control - Halt")
	float HaltSpeedLinearDamping = 2.0f;

	float LinearDampingBeforeHaltBackup = 0.0f;

	bool bCanHalt = true;
	bool bIsHalting = false;

	// Dash
	void StartDash();
	void ResetDashCooldown();

	// Halt
	void StartHalt();
	void StopHalt();
	void ResetHaltCooldown();

	// Mouse input handlers
	void LookUp(float Value);
	void Turn(float Value);

	void RunAutopilot(const FVector& FlyTarget, float& OutYaw, float& OutPitch, float& OutRoll);
};
