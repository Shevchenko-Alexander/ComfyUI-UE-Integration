// Infinity Reply, All Rights Reserved

#include "Framework/AI_GameModeBase.h"
#include "TP_ThirdPerson/TP_ThirdPersonCharacter.h"

AAI_GameModeBase::AAI_GameModeBase()
{
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
    if (PlayerPawnBPClass.Class)
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }
}
