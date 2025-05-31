#include "ProjectFlyReborn/Public/Pawn/FlyingPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"

AFlyingPawn::AFlyingPawn()
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

FVector AFlyingPawn::GetTargetAimWorldLocation() const
{
	// Could be calculated from mouse hit on world plane
	return DesiredDirection;
}

FVector AFlyingPawn::GetCurrentDirection() const
{
	return MeshComponent->GetForwardVector();
}

FRotator AFlyingPawn::GetCurrentRotation() const
{
	return MeshComponent->GetComponentRotation();
}

void AFlyingPawn::SetDesiredDirection(FVector WorldDirection)
{
	DesiredDirection = WorldDirection;
}

void AFlyingPawn::BeginPlay()
{
	Super::BeginPlay();

	CameraYaw = 0.0f;
	CameraPitch = 0.0f;
}

void AFlyingPawn::Tick(float DeltaTime)
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

	// Constant forward thrust
	MeshComponent->AddForce(MeshComponent->GetForwardVector() * ThrustForce);
}

void AFlyingPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Turn", this, &AFlyingPawn::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AFlyingPawn::LookUp);
}

void AFlyingPawn::Turn(float Value)
{
	CameraYaw += Value * MouseSensitivity;
}

void AFlyingPawn::LookUp(float Value)
{
	CameraPitch += Value * MouseSensitivity;
}

void AFlyingPawn::RunAutopilot(const FVector& FlyTarget, float& OutYaw, float& OutPitch, float& OutRoll)
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