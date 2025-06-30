#include "ProjectFlyReborn/Public/Pawn/GliderPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"

AGliderPawn::AGliderPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Replace Capsule with Static Mesh
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetEnableGravity(false);
	MeshComponent->SetLinearDamping(0.7f);   // Slight drag, prevents overspeed
	MeshComponent->SetAngularDamping(5.0f);  // Dampen rotation for stability
	RootComponent = MeshComponent;

	// Spring Arm for camera orbit
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 300.f;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bEnableCameraLag = true;

	// Camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
}

FVector AGliderPawn::GetTargetAimWorldLocation() const
{
	// Could be calculated from mouse hit on world plane
	return DesiredDirection;
}

FVector AGliderPawn::GetCurrentDirection() const
{
	return MeshComponent->GetForwardVector();
}

FRotator AGliderPawn::GetCurrentRotation() const
{
	return MeshComponent->GetComponentRotation();
}

void AGliderPawn::SetDesiredDirection(FVector WorldDirection)
{
	DesiredDirection = WorldDirection;
}

void AGliderPawn::BeginPlay()
{
	Super::BeginPlay();

	// Add initial speed
	AddSpeed(StartPlaneSpeed);
}

void AGliderPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CalculateSpeed(DeltaTime);

	// Clamp and apply camera rotation
	CameraPitch = FMath::Clamp(CameraPitch, -90.f, 90.f);
	FRotator NewRotation(CameraPitch, CameraYaw, 0.0f);
	SpringArm->SetWorldRotation(NewRotation);

	// Fly target = camera forward
	const FVector FlyTarget = MeshComponent->GetComponentLocation() + Camera->GetForwardVector() * 1000.0f;
	DesiredDirection = FlyTarget;

	// Debug lines
	const FVector Start = MeshComponent->GetComponentLocation();
	DrawDebugLine(GetWorld(), Start, Start + MeshComponent->GetForwardVector() * 1000.0f, FColor::Cyan, false, 0.1f, 0, 2.0f);
	DrawDebugLine(GetWorld(), Start, FlyTarget, FColor::Red, false, 0.1f, 0, 2.0f);

	// Autopilot torque calculation
	float YawInput, PitchInput, RollInput;
	RunAutopilot(FlyTarget, YawInput, PitchInput, RollInput);

	// Apply torque
	const FVector Torque = FVector(
		RollInput * TurnTorque.X,
		PitchInput * TurnTorque.Y,
		YawInput * TurnTorque.Z
	);
	MeshComponent->AddTorqueInRadians(MeshComponent->GetComponentRotation().RotateVector(Torque), NAME_None, true);

	// Lift logic

	// Calculate pitch input (difference from last frame)
	static float LastPitchAngle = MeshComponent->GetComponentRotation().Pitch;
	float CurrentPitchAngle = MeshComponent->GetComponentRotation().Pitch;
	float DeltaPitch = CurrentPitchAngle - LastPitchAngle;
	LastPitchAngle = CurrentPitchAngle;

	// Calculate lift only when pulling up (positive DeltaPitch)
	float LiftForceMag = 0.0f;

	if (DeltaPitch > 0.0f && ForwardSpeed > 0)
	{
		// Approximate lift as combination of speed and pitch rate
		float LiftCoefficient = FMath::Clamp(DeltaPitch * LiftCoefficientPitchScalar, 0.0f, 1.0f);
		LiftForceMag = ForwardSpeed * LiftCoefficient * LiftCoefficientScalar; 
	}

	// Lift direction is always world up to avoid strafing due to roll
	FVector LiftDirection = FVector::UpVector;
	FVector LiftForce = LiftDirection * LiftForceMag;

	// Apply lift force
	MeshComponent->AddForce(LiftForce + (MeshComponent->GetForwardVector() * ForwardSpeed));

	// Debug lift force
	DrawDebugLine(GetWorld(), Start, Start + LiftForce * 0.01f, FColor::Green, false, 0.1f, 0, 2.0f);

}

void AGliderPawn::AddSpeed(float Speed)
{
	ForwardSpeed = FMath::Clamp(ForwardSpeed + Speed, MinimumPlaneSpeed, MaximumPlaneSpeed);
}

void AGliderPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Turn", this, &AGliderPawn::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AGliderPawn::LookUp);
}

void AGliderPawn::CalculateSpeed(float DeltaTime)
{
	// Calculation of the speed change depending on the inclination of the plane
	if (MeshComponent->GetForwardVector().Z <= 0)
	{
		AddSpeed(-MeshComponent->GetForwardVector().Z * DiveSpeedIncreaseScalar);
	}
	else
	{
		AddSpeed(-MeshComponent->GetForwardVector().Z * RiseSpeedDecreaseScalar);
	}

	// Calculate bank angle (roll) in degrees
	float BankAngle = MeshComponent->GetComponentRotation().Roll;

	// Take absolute value for calculation
	float AbsBankAngle = FMath::Abs(BankAngle);

	// Calculate bank speed loss factor
	// Example: No loss until 10°, increasing loss up to MaxTurnSpeedLossFactor at 60°
	float MaxTurnSpeedLossFactor = 0.5f; // Lose up to 50% of speed gain when turning hard

	if (AbsBankAngle > 10.0f) // Ignore small banks
	{
		float BankFactor = FMath::Clamp((AbsBankAngle - 10.0f) / 50.0f, 0.0f, 1.0f); // Scale from 10° to 60°
		float SpeedLoss = ForwardSpeed * BankFactor * MaxTurnSpeedLossFactor * DeltaTime;

		// Apply speed loss
		AddSpeed(-SpeedLoss);
	}
}

void AGliderPawn::Turn(float Value)
{
	CameraYaw += Value * MouseSensitivity;
}

void AGliderPawn::LookUp(float Value)
{
	CameraPitch += Value * MouseSensitivity;
}

void AGliderPawn::RunAutopilot(const FVector& FlyTarget, float& OutYaw, float& OutPitch, float& OutRoll)
{
	const FTransform& ActorTransform = GetActorTransform();
	FVector LocalFlyTarget = ActorTransform.InverseTransformPosition(FlyTarget).GetSafeNormal() * TurnAngleSensitivity;

	// Pitch (Z), Yaw (Y), Roll (X)

	// Pitch and Yaw
	OutPitch = -FMath::Clamp(LocalFlyTarget.Z, -1.0f, 1.0f);
	OutYaw = FMath::Clamp(LocalFlyTarget.Y, -1.0f, 1.0f);

	// Roll
	float AggressiveRoll = FMath::Clamp(LocalFlyTarget.Y, -1.0f, 1.0f);
	float WingsLevelRoll = GetActorRightVector().Z;

	FVector ToTarget = (FlyTarget - GetActorLocation()).GetSafeNormal();
	float AngleOffTarget = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(GetActorForwardVector(), ToTarget)));

	float BlendFactor = FMath::Clamp(AngleOffTarget / AggressiveTurnAngle, 0.0f, 1.0f);
	OutRoll = -FMath::Lerp(WingsLevelRoll, AggressiveRoll, BlendFactor);
}