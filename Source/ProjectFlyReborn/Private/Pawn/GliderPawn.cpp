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

	CameraYaw = 0.0f;
	CameraPitch = 0.0f;

	MeshComponent->AddForce(MeshComponent->GetForwardVector() * InitialThrustForce, NAME_None, true);
}

void AGliderPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Clamp and apply camera rotation
	CameraPitch = FMath::Clamp(CameraPitch, -90.f, 90.f);
	FRotator NewRotation(CameraPitch, CameraYaw, 0.0f);
	SpringArm->SetWorldRotation(NewRotation);

	// Fly target = camera forward
	const FVector FlyTarget = MeshComponent->GetComponentLocation() + Camera->GetForwardVector() * 1000.0f;
	DesiredDirection = FlyTarget;

	// Debug: current direction and target
	const FVector Start = MeshComponent->GetComponentLocation();
	DrawDebugLine(GetWorld(), Start, Start + MeshComponent->GetForwardVector() * 1000.0f, FColor::Cyan, false, 0.1f, 0, 2.0f);
	DrawDebugLine(GetWorld(), Start, FlyTarget, FColor::Red, false, 0.1f, 0, 2.0f);

	// Autopilot: calculate control inputs
	float YawInput, PitchInput, RollInput;
	RunAutopilot(FlyTarget, YawInput, PitchInput, RollInput);

	// Apply torque using correct Unreal axis mapping:
	// X = Roll, Y = Pitch, Z = Yaw
	const FVector Torque = FVector(
		RollInput * TurnTorque.X,
		PitchInput * TurnTorque.Y,
		YawInput * TurnTorque.Z
	);

	// Apply torque in local space
	MeshComponent->AddTorqueInRadians(MeshComponent->GetComponentRotation().RotateVector(Torque), NAME_None, true);

	// Compute current forward speed (airspeed)
	const FVector Velocity = MeshComponent->GetPhysicsLinearVelocity();
	const float ForwardSpeed = FVector::DotProduct(Velocity, MeshComponent->GetForwardVector());

	// Lift depends on square of speed and pitch angle (simplified AOA)
	float PitchAngleRad = FMath::DegreesToRadians(MeshComponent->GetComponentRotation().Pitch);
	float LiftCoefficient = FMath::Clamp(FMath::Cos(PitchAngleRad), 0.0f, 1.0f);

	// Calculate lift force
	FVector LiftDirection = MeshComponent->GetUpVector();
	float LiftForceMagnitude = LiftCoefficient * ForwardSpeed * ForwardSpeed * LiftScalar;

	MeshComponent->AddForce(LiftDirection * LiftForceMagnitude);

	// Simulate gravity (custom so we can tune it)
	MeshComponent->AddForce(FVector(0, 0, -GravityForce));

	// Auto-dive if speed too low
	if (MeshComponent->GetComponentVelocity().Size() < 300.0f) // tune threshold
	{
		FVector DiveTarget = GetActorLocation() + FVector(0, 0, -1);
		float DummyYaw, DummyPitch, DummyRoll;
		RunAutopilot(DiveTarget, DummyYaw, DummyPitch, DummyRoll);

		// Add small torque to encourage nose down
		FVector DiveTorque = FVector(
			DummyRoll * TurnTorque.X * 0.5f,
			DummyPitch * TurnTorque.Y * 0.5f,
			DummyYaw * TurnTorque.Z * 0.5f
		);

		MeshComponent->AddTorqueInRadians(MeshComponent->GetComponentRotation().RotateVector(DiveTorque), NAME_None, true);
	}

	if (MeshComponent->GetForwardVector().Z <= 0)
	{
		ThrustForce += -MeshComponent->GetForwardVector().Z * DiveSpeedIncreaseScalar;
	}
	else
	{
		ThrustForce += -MeshComponent->GetForwardVector().Z * RiseSpeedDecreaseScalar;
	}
	ThrustForce = FMath::Clamp(ThrustForce, MinimumThrustForce, MaximumThrustForce);

	// Forward thrust
	MeshComponent->AddForce(MeshComponent->GetForwardVector() * ThrustForce, NAME_None, true);
}

void AGliderPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Turn", this, &AGliderPawn::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AGliderPawn::LookUp);
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