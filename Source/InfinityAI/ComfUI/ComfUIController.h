#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ComfUIWebSocketConnection.h"
#include "Http.h"
#include "ComfUIController.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogComfUIController, Log, All);

class AAI_Art;


//// status
USTRUCT(BlueprintType)
struct FExecInfo
{
    GENERATED_BODY()

    UPROPERTY()
    int32 queue_remaining;
};

USTRUCT(BlueprintType)
struct FStatus
{
    GENERATED_BODY()

    UPROPERTY()
    FExecInfo exec_info;
};

USTRUCT(BlueprintType)
struct FStatusData
{
    GENERATED_BODY()

    UPROPERTY()
    FStatus status;
};


//// execution_start
USTRUCT(BlueprintType)
struct FExecutionStartData
{
    GENERATED_BODY()

    UPROPERTY()
    FString prompt_id;
};


//// execution_cached
USTRUCT(BlueprintType)
struct FExecutionCachedData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FString> nodes;
    
    UPROPERTY()
    FString prompt_id;
};


//// executing
USTRUCT(BlueprintType)
struct FExecutingData
{
    GENERATED_BODY()

    UPROPERTY()
    FString node;
    
    UPROPERTY()
    FString prompt_id;
};


//// progress
USTRUCT(BlueprintType)
struct FProgressData
{
    GENERATED_BODY()
    
    UPROPERTY()
    int32 value;

    UPROPERTY()
    int32 max;

    UPROPERTY()
    FString node;
    
    UPROPERTY()
    FString prompt_id;
};



//// executed
USTRUCT(BlueprintType)
struct FOutputImage
{
    GENERATED_BODY()

    UPROPERTY()
    FString filename;

    UPROPERTY()
    FString subfolder;

    UPROPERTY()
    FString type;
};

USTRUCT(BlueprintType)
struct FOutput
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FOutputImage> images;
    
    UPROPERTY()
    FString prompt_id;
};

USTRUCT(BlueprintType)
struct FExecutedData
{
    GENERATED_BODY()

    UPROPERTY()
    FOutput output;

    UPROPERTY()
    FString node;
    
    UPROPERTY()
    FString prompt_id;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeshDownloadedDelegate, const FString&, Filename);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComfyUIConnectedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComfyUIDisconnectedDelegate);

UCLASS()
class INFINITYAI_API AComfUIController : public AActor
{
    GENERATED_BODY()

public:
    AComfUIController();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere)
    FString BackendIP = "localhost";

    UPROPERTY(EditAnywhere)
    int BackendPort = 8188;


    UFUNCTION(BlueprintCallable)
    void SendPrompt(const FString& Prompt);

    UFUNCTION(BlueprintCallable)
    void GetImage(const FString& FileName);

    UFUNCTION(BlueprintCallable)
    void GetMesh(const FString& FileName);

    UFUNCTION(BlueprintCallable)
    void GetMtl(const FString& FileName);

    UFUNCTION(BlueprintCallable)
    void GetPng(const FString& FileName);
    
    UFUNCTION(BlueprintCallable)
    FString GetSeed() const;

    UFUNCTION(BlueprintCallable)
    FString GenerateRandomSeed();

    UFUNCTION(BlueprintCallable)
    void SetSeed(const FString& SeedUInt64);

    UFUNCTION(BlueprintCallable)
    void SetPositivPrompt(const FString& Prompt);

    UFUNCTION(BlueprintCallable)
    void SetNegativePrompt(const FString& Prompt);

    UFUNCTION(BlueprintCallable)
    FString GetPositivPrompt() const;

    UFUNCTION(BlueprintCallable)
    FString GetNegativePrompt() const;

    UFUNCTION(BlueprintCallable)
    void SendRequest_InstantMesh();

    UFUNCTION(BlueprintCallable)
    void SendRequest_StableFast3D();
    
    UPROPERTY(BlueprintAssignable)
    FOnMeshDownloadedDelegate OnMeshDownloadedDelegate;

    UPROPERTY(BlueprintAssignable)
    FOnComfyUIConnectedDelegate OnComfyUIConnectedDelegate;

    UPROPERTY(BlueprintAssignable)
    FOnComfyUIDisconnectedDelegate OnComfyUIDisconnectedDelegate;
    
private:

    UPROPERTY(EditAnywhere)
    FString ClientID = "97ef7d4c-4121-4010-baed-a7b71acb2c7a";

    UPROPERTY(EditAnywhere)
    FString Seed = "617990359517930";

    UPROPERTY(EditAnywhere)
    FString PositivePrompt = "Gray clay style,(masterpiece, best quality), Grayscale,a man,dainty, perfect face, pretty face, lush detail, half body";
    
    UPROPERTY(EditAnywhere)
    FString NegativePrompt = "badhandv4, paintings, sketches, (worst qualit:2), (low quality:2), (normal quality:2), lowers, normal quality, ((monochrome)), ((grayscale)), skin spots, acnes, skin blemishes, age spot, (outdoor:1.6), manboobs, (backlight:1.2), double navel, muted arms, hused arms, neck lace, analog, analog effects, (sunglass:1.4), nipples, nsfw, bad architecture, watermark, (mole:1.5)";

    
    
    FString CurrentPromptID;
    bool IsImageGenerated = false;
    
    UPROPERTY()
    TArray<TSoftObjectPtr<AAI_Art>> Arts;
    
    TUniquePtr<FComfUIWebSocketConnection> ConfUIWebSocketConnection;

    void OnConnected();
    void OnDisconnected();
    void OnPromptRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful);
    void OnGetImageRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful);
    void OnGetMeshRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful);
    void OnGetMtlRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful);
    void OnGetPngRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful);
};
