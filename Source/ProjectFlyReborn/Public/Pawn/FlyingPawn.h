#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlyingPawn.generated.h"

UCLASS()
class PROJECTFLYREBORN_API AFlyingPawn : public APawn
{
	GENERATED_BODY()

public:
	AFlyingPawn();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	virtual void Tick(float DeltaTime) override;

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

	// Mouse input handlers
	void LookUp(float Value);
	void Turn(float Value);
};