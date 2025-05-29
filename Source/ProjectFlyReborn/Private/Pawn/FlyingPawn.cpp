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