#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ProjectFlyReborn/Public/UI/FlightDirectionWidget.h"
#include "FlightHUD.generated.h"

UCLASS()
class PROJECTFLYREBORN_API AFlightHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UFlightDirectionWidget> FlightDirectionWidgetClass;

	UPROPERTY()
	UFlightDirectionWidget* FlightDirectionWidget;
};