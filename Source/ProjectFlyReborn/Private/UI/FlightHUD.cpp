#include "ProjectFlyReborn/Public/UI/FlightHUD.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFlyReborn/Public/Interface/FlightMouseAimInterface.h"

void AFlightHUD::BeginPlay()
{
	Super::BeginPlay();

	if (FlightDirectionWidgetClass)
	{
		FlightDirectionWidget = CreateWidget<UFlightDirectionWidget>(GetWorld(), FlightDirectionWidgetClass);
		if (FlightDirectionWidget)
		{
			FlightDirectionWidget->AddToViewport();
		}
	}
}