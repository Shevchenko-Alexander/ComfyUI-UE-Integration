// Infinity Reply, All Rights Reserved

#include "AI_Art.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AAI_Art::AAI_Art()
{
    PrimaryActorTick.bCanEverTick = false;
    Origin = CreateDefaultSubobject<USceneComponent>("Origin");
    SetRootComponent(Origin);
    Plane = CreateDefaultSubobject<UStaticMeshComponent>("Plane");
    SetRootComponent(Origin);
    Plane->SetupAttachment(Origin);
}

void AAI_Art::BeginPlay()
{
    Super::BeginPlay();
    ArtMaterial = Plane->CreateAndSetMaterialInstanceDynamic(0);
}

void AAI_Art::SetArtTexture(UTexture2D* ArtTexture) const
{
    if (ArtTexture)
    {
        ArtMaterial->SetTextureParameterValue("Art", ArtTexture);
    }
}