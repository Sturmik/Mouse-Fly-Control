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

	// Apply torque (arcade feel: strong responsiveness)
	const FVector Torque = FVector(
		RollInput * TurnTorque.X,
		PitchInput * TurnTorque.Y,
		YawInput * TurnTorque.Z
	);
	MeshComponent->AddTorqueInRadians(MeshComponent->GetComponentRotation().RotateVector(Torque), NAME_None, true);

	// Glider simulation

	float Speed = MeshComponent->GetComponentVelocity().Size();
	float AoA = MeshComponent->GetForwardVector().Z;

	float LiftSpeedThreshold = 300.0f;
	float SpeedFactor = FMath::Clamp((Speed - LiftSpeedThreshold) / LiftSpeedThreshold, 0.0f, 1.0f);

	float StallAngle = 0.5f;
	float LiftCoefficient;

	if (AoA > StallAngle)
	{
		LiftCoefficient = FMath::Clamp(1.0f - (AoA - StallAngle) * 5.0f, 0.0f, 1.0f);
	}
	else
	{
		LiftCoefficient = FMath::Clamp(AoA, 0.0f, 1.0f);
	}

	LiftCoefficient *= SpeedFactor;

	float LiftForceMag = Speed * Speed * LiftCoefficient * LiftCoefficientScalar;
	LiftForceMag = FMath::Min(LiftForceMag, MaxLiftForce);
	FVector LiftForce = FVector::UpVector * LiftForceMag;

	// Full gravity for snappy fall
	FVector GravityForce = FVector::DownVector * GravityScalar;

	// Quadratic drag force
	float DragCoefficient = 0.002f;
	FVector Velocity = MeshComponent->GetComponentVelocity();
	FVector DragForce = -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;

	FVector TotalForce = LiftForce + GravityForce + DragForce + (MeshComponent->GetForwardVector() * ForwardSpeed);
	MeshComponent->AddForce(TotalForce);

	// Debug lift and turbulence
	DrawDebugLine(GetWorld(), Start, Start + LiftForce * 0.01f, FColor::Green, false, 0.1f, 0, 2.0f);

	// Dramatic gravity addition

	const float CriticalPitchAngle = 75.0f;
	float InclinationDegrees = FMath::RadiansToDegrees(FMath::Asin(MeshComponent->GetForwardVector().Z));

	float DramaticGravityMultiplier = 1.0f;
	bool bCriticalCondition = false;

	// Smoothly calculate stall gravity multiplier based on low speed (0 to 3)
	if (ForwardSpeed < MinimumPlaneSpeed * 2.0f)
	{
		bCriticalCondition = true;
		float StallFactor = FMath::GetMappedRangeValueClamped(
			FVector2D(MinimumPlaneSpeed * 2.0f, MinimumPlaneSpeed * 0.5f),
			FVector2D(1.0f, 3.0f),
			ForwardSpeed
		);
		DramaticGravityMultiplier = StallFactor;
	}

	// Diving or rising more than critical pitch
	if (FMath::Abs(InclinationDegrees) > CriticalPitchAngle)
	{
		bCriticalCondition = true;

		if (InclinationDegrees < -CriticalPitchAngle)
		{
			// Diving: smoothly increase multiplier up to x5
			float DiveMultiplier = FMath::GetMappedRangeValueClamped(
				FVector2D(-CriticalPitchAngle, -90.f),
				FVector2D(1.0f, 5.0f),
				InclinationDegrees
			);
			DramaticGravityMultiplier = FMath::Max(DramaticGravityMultiplier, DiveMultiplier);
		}
		else
		{
			// Rising steeply: smoothly increase multiplier up to x4
			float RiseMultiplier = FMath::GetMappedRangeValueClamped(
				FVector2D(CriticalPitchAngle, 90.f),
				FVector2D(1.0f, 4.0f),
				InclinationDegrees
			);
			DramaticGravityMultiplier = FMath::Max(DramaticGravityMultiplier, RiseMultiplier);
		}
	}

	// Apply dramatic gravity force if any condition met
	if (bCriticalCondition)
	{
		// Smoothly interpolate to avoid abrupt force application
		static float SmoothedGravityMultiplier = 1.0f;
		SmoothedGravityMultiplier = FMath::FInterpTo(SmoothedGravityMultiplier, DramaticGravityMultiplier, DeltaTime, 3.0f);

		FVector DramaticGravityForce = FVector::DownVector * GravityScalar * SmoothedGravityMultiplier;
		MeshComponent->AddForce(DramaticGravityForce);

		// Optional debug
		DrawDebugLine(GetWorld(), Start, Start + DramaticGravityForce * 0.01f, FColor::Purple, false, 0.1f, 0, 2.0f);
	}
}

void AGliderPawn::AddSpeed(float Speed)
{
	ForwardSpeed = FMath::Clamp(ForwardSpeed + Speed, MinimumPlaneSpeed, MaximumPlaneSpeed);

	// The less speed we have the less control user has over it's plane
	AirControl = FMath::GetMappedRangeValueClamped(
		FVector2D(MinimumPlaneSpeed, MaximumPlaneSpeed),
		FVector2D(MinimumAirControl, MaximumAirControl),
		ForwardSpeed
	);
}

void AGliderPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Turn", this, &AGliderPawn::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AGliderPawn::LookUp);
}

void AGliderPawn::CalculateSpeed(float DeltaTime)
{
	// Calculate plane inclination
	float Inclination = MeshComponent->GetForwardVector().Z;

	if (Inclination < 0)
	{
		// Drastic dive speed increase: use squared or cubed for non-linear growth
		float DiveFactor = -Inclination; // Convert to positive

		// Example: cube the factor for strong acceleration at steep dives
		float DiveAcceleration = FMath::Pow(DiveFactor, 3) * DiveSpeedIncreaseScalar;

		AddSpeed(DiveAcceleration * DeltaTime);
	}
	else
	{
		// Normal rise speed decrease remains linear
		AddSpeed(-Inclination * RiseSpeedDecreaseScalar * DeltaTime);
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
	// Base autopilot control signals (full responsiveness)
	float BasePitch = -FMath::Clamp(LocalFlyTarget.Z, -1.0f, 1.0f);
	float BaseYaw = FMath::Clamp(LocalFlyTarget.Y, -1.0f, 1.0f);

	float AggressiveRoll = FMath::Clamp(LocalFlyTarget.Y, -1.0f, 1.0f);
	float WingsLevelRoll = GetActorRightVector().Z;

	FVector ToTarget = (FlyTarget - GetActorLocation()).GetSafeNormal();
	float AngleOffTarget = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(GetActorForwardVector(), ToTarget)));

	float BlendFactor = FMath::Clamp(AngleOffTarget / AggressiveTurnAngle, 0.0f, 1.0f);
	float BaseRoll = -FMath::Lerp(WingsLevelRoll, AggressiveRoll, BlendFactor);

	// Calculate responsiveness factor [0..1] based on ForwardSpeed
	// Normalize AirControl between MinimumAirControl and MaximumAirControl to [0..1]
	float Responsiveness = FMath::GetMappedRangeValueClamped(
		FVector2D(MinimumAirControl, MaximumAirControl),
		FVector2D(0.1f, 1.0f),
		AirControl
	);

	// Normal autopilot control with responsiveness scaling
	OutPitch = BasePitch * Responsiveness;
	OutYaw = BaseYaw * Responsiveness;
	OutRoll = BaseRoll * Responsiveness;
}
