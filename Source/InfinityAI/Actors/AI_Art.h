// Infinity Reply, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI_Art.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class INFINITYAI_API AAI_Art : public AActor
{
    GENERATED_BODY()

public:
    AAI_Art();

    void SetArtTexture(UTexture2D* ArtTexture) const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    TObjectPtr<USceneComponent> Origin;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Plane;

private:
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> ArtMaterial;
};
