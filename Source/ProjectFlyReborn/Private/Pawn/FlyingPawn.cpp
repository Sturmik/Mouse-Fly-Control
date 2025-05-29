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

	CameraPitch = FMath::Clamp(CameraPitch, -89.9f, 89.9f);
	FRotator NewRotation(CameraPitch, CameraYaw, 0.0f);
	SpringArm->SetWorldRotation(NewRotation);

	// Direction of mesh component
	FVector Start = MeshComponent->GetComponentLocation();
	FVector End = Start + MeshComponent->GetForwardVector() * 1000.0f;
	DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 0.1f, 0, 2.0f);
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