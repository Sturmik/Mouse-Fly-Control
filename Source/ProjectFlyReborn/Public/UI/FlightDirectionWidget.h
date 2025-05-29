#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlightDirectionWidget.generated.h"

UCLASS()
class PROJECTFLYREBORN_API UFlightDirectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateFlightUI(FVector2D ReticleScreenPosition, FVector2D DirectionIndicatorPosition);
};
