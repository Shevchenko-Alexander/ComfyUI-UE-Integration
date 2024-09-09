#include "ComfUIController.h"

#include "glTFRuntimeFunctionLibrary.h"
#include "glTFRuntimeOBJAssetActor.h"
#include "HttpModule.h"
#include "JsonObjectConverter.h"
#include "Actors/AI_Art.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/GameplayStatics.h"
#include "FuncLib/ImageFuncLib.h"

DEFINE_LOG_CATEGORY(LogComfUIController)


AComfUIController::AComfUIController()
{
    //PrimaryActorTick.bCanEverTick = true;
}

void AComfUIController::BeginPlay()
{
    Super::BeginPlay();

    TArray<AActor*> ArtActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAI_Art::StaticClass(), ArtActors);
    Arts.Reserve(ArtActors.Num());
    for (const AActor* CurrentActor : ArtActors)
    {
        Arts.Add(Cast<AAI_Art>(CurrentActor));
    }

    ConfUIWebSocketConnection = MakeUnique<FComfUIWebSocketConnection>();
    ConfUIWebSocketConnection->RegisterForCommandCallback("crystools.monitor", [&](const TSharedPtr<FJsonObject>& JsonMsg)
    {
        const TSharedPtr<FJsonValue> DataJSONValue = JsonMsg->TryGetField(TEXT("data"));
        if (!DataJSONValue)
        {
            UE_LOG(LogComfUIController, Error, TEXT("Unable to find data field!"));
            return;
        }
        // TODO
        // {"type": "crystools.monitor", "data": {"cpu_utilization": 10.8, "ram_total": 205886779392, "ram_used": 82341232640, "ram_used_percent": 40.0, "hdd_total": 20000586850304, "hdd_used": 19229831716864, "hdd_used_percent": 96.1, "device_type": "cuda", "gpus": [{"gpu_utilization": 99, "gpu_temperature": 48, "vram_total": 25757220864, "vram_used": 25655361536, "vram_used_percent": 99.6045406896271}]}}
    });
    
    ConfUIWebSocketConnection->RegisterForCommandCallback("status", [&](const TSharedPtr<FJsonObject>& JsonMsg)
    {
        const TSharedPtr<FJsonValue> DataJSONValue = JsonMsg->TryGetField(TEXT("data"));
        if (!DataJSONValue)
        {
            UE_LOG(LogComfUIController, Error, TEXT("Unable to find data field!"));
            return;
        }

        const TSharedPtr<FJsonObject> RenderDatabaseTaskObject = DataJSONValue->AsObject();
        FStatusData StatusData;
        if (!FJsonObjectConverter::JsonObjectToUStruct<FStatusData>(RenderDatabaseTaskObject.ToSharedRef(), &StatusData))
        {
            UE_LOG(LogComfUIController, Error, TEXT("Failed to convert json StatusData to object"));
            return;
        }

        UE_LOG(LogComfUIController, Warning, TEXT("[status] queue_remaining: %d"), StatusData.status.exec_info.queue_remaining);

        if (StatusData.status.exec_info.queue_remaining == 0 && IsImageGenerated)
        {
            GetMesh("Mesh.obj");
        }
    });

    ConfUIWebSocketConnection->RegisterForCommandCallback("execution_start", [&](const TSharedPtr<FJsonObject>& JsonMsg)
    {
        const TSharedPtr<FJsonValue> DataJSONValue = JsonMsg->TryGetField(TEXT("data"));
        if (!DataJSONValue)
        {
            UE_LOG(LogComfUIController, Error, TEXT("Unable to find data field!"));
            return;
        }

        const TSharedPtr<FJsonObject> RenderDatabaseTaskObject = DataJSONValue->AsObject();
        FExecutionStartData ExecutionStartData;
        if (!FJsonObjectConverter::JsonObjectToUStruct<FExecutionStartData>(RenderDatabaseTaskObject.ToSharedRef(), &ExecutionStartData))
        {
            UE_LOG(LogComfUIController, Error, TEXT("Failed to convert json ExecutionStartData to object"));
            return;
        }
        CurrentPromptID = ExecutionStartData.prompt_id;
        UE_LOG(LogComfUIController, Warning, TEXT("[execution_start] prompt_id: %s"), *CurrentPromptID);
    });
    
    ConfUIWebSocketConnection->RegisterForCommandCallback("execution_cached", [&](const TSharedPtr<FJsonObject>& JsonMsg)
    {
        UE_LOG(LogComfUIController, Warning, TEXT("[execution_cached]"));
    });

    ConfUIWebSocketConnection->RegisterForCommandCallback("executing", [&](const TSharedPtr<FJsonObject>& JsonMsg)
    {
        const TSharedPtr<FJsonValue> DataJSONValue = JsonMsg->TryGetField(TEXT("data"));
        if (!DataJSONValue)
        {
            UE_LOG(LogComfUIController, Error, TEXT("Unable to find data field!"));
            return;
        }

        const TSharedPtr<FJsonObject> RenderDatabaseTaskObject = DataJSONValue->AsObject();
        FExecutingData ExecutingData;
        if (!FJsonObjectConverter::JsonObjectToUStruct<FExecutingData>(RenderDatabaseTaskObject.ToSharedRef(), &ExecutingData))
        {
            UE_LOG(LogComfUIController, Error, TEXT("Failed to convert json ExecutingData to object"));
            return;
        }

        UE_LOG(LogComfUIController, Warning, TEXT("[executing] node: %s"), *ExecutingData.node);
    });

    ConfUIWebSocketConnection->RegisterForCommandCallback("progress", [&](const TSharedPtr<FJsonObject>& JsonMsg)
    {
        const TSharedPtr<FJsonValue> DataJSONValue = JsonMsg->TryGetField(TEXT("data"));
        if (!DataJSONValue)
        {
            UE_LOG(LogComfUIController, Error, TEXT("Unable to find data field!"));
            return;
        }

        const TSharedPtr<FJsonObject> RenderDatabaseTaskObject = DataJSONValue->AsObject();
        FProgressData ProgressData;
        if (!FJsonObjectConverter::JsonObjectToUStruct<FProgressData>(RenderDatabaseTaskObject.ToSharedRef(), &ProgressData))
        {
            UE_LOG(LogComfUIController, Error, TEXT("Failed to convert json ProgressData to object"));
            return;
        }

        UE_LOG(LogComfUIController, Warning, TEXT("[progress] value: %d, max: %d, node: %s"), ProgressData.value, ProgressData.max, *ProgressData.node);
    });

    ConfUIWebSocketConnection->RegisterForCommandCallback("executed", [&](const TSharedPtr<FJsonObject>& JsonMsg)
    {
        const TSharedPtr<FJsonValue> DataJSONValue = JsonMsg->TryGetField(TEXT("data"));
        if (!DataJSONValue)
        {
            UE_LOG(LogComfUIController, Error, TEXT("Unable to find data field!"));
            return;
        }

        const TSharedPtr<FJsonObject> RenderDatabaseTaskObject = DataJSONValue->AsObject();
        FExecutedData ExecutedData;
        if (!FJsonObjectConverter::JsonObjectToUStruct<FExecutedData>(RenderDatabaseTaskObject.ToSharedRef(), &ExecutedData))
        {
            UE_LOG(LogComfUIController, Error, TEXT("Failed to convert json ExecutedData to object"));
            return;
        }

        if (!ExecutedData.output.images.IsEmpty())
        {
            UE_LOG(LogComfUIController, Warning, TEXT("[executed] node: %s, filename: %s, subfolder: %s, type: %s"), *ExecutedData.node, *ExecutedData.output.images[0].filename, *ExecutedData.output.images[0].subfolder, *ExecutedData.output.images[0].type);
            IsImageGenerated = true;
            GetImage(ExecutedData.output.images[0].filename);
        }
    });
    
    ConfUIWebSocketConnection->OnConnectedToBackendDelegate.AddUObject(this, &ThisClass::OnConnected);
    ConfUIWebSocketConnection->OnDisconnectedFromBackendDelegate.AddUObject(this, &ThisClass::OnDisconnected);
    ConfUIWebSocketConnection->ConnectToBackend(BackendIP, BackendPort, ClientID);
}

void AComfUIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ConfUIWebSocketConnection = nullptr;
}

void AComfUIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AComfUIController::SendPrompt(const FString& Prompt)
{
    const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetVerb("POST");
    const FString PostUIURL = FString::Printf(TEXT("http://%s:%d/prompt"), *BackendIP, BackendPort);
    HttpRequest->SetURL(PostUIURL);
    HttpRequest->SetContentAsString(Prompt);
    UE_LOG(LogComfUIController, Display, TEXT("Sending Prompt JSON:\n%s"), *Prompt);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnPromptRequestComplete);
    HttpRequest->ProcessRequest();
}

void AComfUIController::GetImage(const FString& FileName)
{
    const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("GET");
    const FString GetImageURL = FString::Printf(TEXT("http://%s:%d/view?filename=%s"), *BackendIP, BackendPort, *FileName);
    HttpRequest->SetURL(GetImageURL);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnGetImageRequestComplete);
    HttpRequest->ProcessRequest();
}

void AComfUIController::GetMesh(const FString& FileName)
{
    const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("GET");
    const FString GetImageURL = FString::Printf(TEXT("http://%s:%d/view?filename=%s"), *BackendIP, BackendPort, *FileName);
    HttpRequest->SetURL(GetImageURL);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnGetMeshRequestComplete);
    HttpRequest->ProcessRequest();
}

void AComfUIController::GetMtl(const FString& FileName)
{
    const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("GET");
    const FString GetImageURL = FString::Printf(TEXT("http://%s:%d/view?filename=%s"), *BackendIP, BackendPort, *FileName);
    HttpRequest->SetURL(GetImageURL);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnGetMtlRequestComplete);
    HttpRequest->ProcessRequest();
}

void AComfUIController::GetPng(const FString& FileName)
{
    const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("GET");
    const FString GetImageURL = FString::Printf(TEXT("http://%s:%d/view?filename=%s"), *BackendIP, BackendPort, *FileName);
    HttpRequest->SetURL(GetImageURL);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnGetPngRequestComplete);
    HttpRequest->ProcessRequest();
}


void AComfUIController::OnConnected()
{
    UE_LOG(LogComfUIController, Warning, TEXT("CONNECTED to ComfyUI"));
    OnComfyUIConnectedDelegate.Broadcast();
}

void AComfUIController::OnDisconnected()
{
    UE_LOG(LogComfUIController, Warning, TEXT("DISCONNECTED from ComfyUI"));
    OnComfyUIDisconnectedDelegate.Broadcast();
}

void AComfUIController::OnPromptRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
{
    if (ResponsePtr)
    {
        UE_LOG(LogComfUIController, Warning, TEXT("OnHttpRequestComplete: %s, successful: %s"), *ResponsePtr->GetContentAsString(), bWasSuccessful ? TEXT("true") : TEXT("false"));
    }
}

void AComfUIController::OnGetImageRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
{
    UE_LOG(LogComfUIController, Warning, TEXT("OnGetImageRequestComplete, successful: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
    if (bWasSuccessful && ResponsePtr.IsValid())
    {
        const TArray<uint8> ImageData = ResponsePtr->GetContent();
        auto* ArtTexture = UImageFuncLib::CreateTexture(ImageData);
        for (const auto& CurrentArt : Arts)
        {
            CurrentArt->SetArtTexture(ArtTexture);
        }
    }
}

void AComfUIController::OnGetMeshRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
{
    UE_LOG(LogComfUIController, Warning, TEXT("OnGetFileRequestComplete, successful: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
    if (bWasSuccessful && ResponsePtr.IsValid())
    {
        const TArray<uint8> FileData = ResponsePtr->GetContent();
        const FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "Mesh.obj";
        FFileHelper::SaveArrayToFile(FileData, *FilePath);
        GetMtl("Mesh.mtl");
    }
}

void AComfUIController::OnGetMtlRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
{
    UE_LOG(LogComfUIController, Warning, TEXT("OnGetFileRequestComplete, successful: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
    if (bWasSuccessful && ResponsePtr.IsValid())
    {
        const TArray<uint8> FileData = ResponsePtr->GetContent();
        const FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "Mesh.mtl";
        FFileHelper::SaveArrayToFile(FileData, *FilePath);
        GetPng("Mesh_albedo.png");
    }
}

void AComfUIController::OnGetPngRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
{
    UE_LOG(LogComfUIController, Warning, TEXT("OnGetFileRequestComplete, successful: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
    if (bWasSuccessful && ResponsePtr.IsValid())
    {
        const TArray<uint8> FileData = ResponsePtr->GetContent();
        const FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "Mesh_albedo.png";
        FFileHelper::SaveArrayToFile(FileData, *FilePath);
        OnMeshDownloadedDelegate.Broadcast(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "Mesh.obj");
    }
}


FString AComfUIController::GetSeed() const
{
    return Seed;
}

FString AComfUIController::GenerateRandomSeed()
{
    uint64 Time = FPlatformTime::Cycles64();
    Time = ((Time << 5) + Time) * 0xfb23618f;
    char temp[21];
    sprintf_s(temp, "%llu", Time);
    return temp;
}

void AComfUIController::SetSeed(const FString& SeedUInt64)
{
    Seed = SeedUInt64;
}

void AComfUIController::SetPositivPrompt(const FString& Prompt)
{
    PositivePrompt = Prompt;
}

void AComfUIController::SetNegativePrompt(const FString& Prompt)
{
    NegativePrompt = Prompt;
}

FString AComfUIController::GetPositivPrompt() const
{
    return PositivePrompt;
}

FString AComfUIController::GetNegativePrompt() const
{
    return NegativePrompt;
}


FString JsonString_InstantMesh = R"delim({
    "prompt" : {
      "6": {
        "inputs": {
          "text": "%POSITIVE_PROMPT%",
          "clip": [
            "11",
            0
          ]
        },
        "class_type": "CLIPTextEncode",
        "_meta": {
          "title": "CLIP Text Encode (Positive Prompt)"
        }
      },
      "8": {
        "inputs": {
          "samples": [
            "13",
            0
          ],
          "vae": [
            "10",
            0
          ]
        },
        "class_type": "VAEDecode",
        "_meta": {
          "title": "VAE Decode"
        }
      },
      "9": {
        "inputs": {
          "filename_prefix": "ComfyUI",
          "images": [
            "8",
            0
          ]
        },
        "class_type": "SaveImage",
        "_meta": {
          "title": "Save Image"
        }
      },
      "10": {
        "inputs": {
          "vae_name": "ae.safetensors"
        },
        "class_type": "VAELoader",
        "_meta": {
          "title": "Load VAE"
        }
      },
      "11": {
        "inputs": {
          "clip_name1": "t5xxl_fp16.safetensors",
          "clip_name2": "clip_l.safetensors",
          "type": "flux"
        },
        "class_type": "DualCLIPLoader",
        "_meta": {
          "title": "DualCLIPLoader"
        }
      },
      "12": {
        "inputs": {
          "unet_name": "flux1-dev.safetensors",
          "weight_dtype": "default"
        },
        "class_type": "UNETLoader",
        "_meta": {
          "title": "Load Diffusion Model"
        }
      },
      "13": {
        "inputs": {
          "noise": [
            "25",
            0
          ],
          "guider": [
            "22",
            0
          ],
          "sampler": [
            "16",
            0
          ],
          "sigmas": [
            "17",
            0
          ],
          "latent_image": [
            "27",
            0
          ]
        },
        "class_type": "SamplerCustomAdvanced",
        "_meta": {
          "title": "SamplerCustomAdvanced"
        }
      },
      "16": {
        "inputs": {
          "sampler_name": "euler"
        },
        "class_type": "KSamplerSelect",
        "_meta": {
          "title": "KSamplerSelect"
        }
      },
      "17": {
        "inputs": {
          "scheduler": "simple",
          "steps": 20,
          "denoise": 1,
          "model": [
            "30",
            0
          ]
        },
        "class_type": "BasicScheduler",
        "_meta": {
          "title": "BasicScheduler"
        }
      },
      "22": {
        "inputs": {
          "model": [
            "30",
            0
          ],
          "conditioning": [
            "26",
            0
          ]
        },
        "class_type": "BasicGuider",
        "_meta": {
          "title": "BasicGuider"
        }
      },
      "25": {
        "inputs": {
          "noise_seed": [
            "68",
            0
          ]
        },
        "class_type": "RandomNoise",
        "_meta": {
          "title": "RandomNoise"
        }
      },
      "26": {
        "inputs": {
          "guidance": 3.5,
          "conditioning": [
            "6",
            0
          ]
        },
        "class_type": "FluxGuidance",
        "_meta": {
          "title": "FluxGuidance"
        }
      },
      "27": {
        "inputs": {
          "width": 512,
          "height": 512,
          "batch_size": 1
        },
        "class_type": "EmptySD3LatentImage",
        "_meta": {
          "title": "EmptySD3LatentImage"
        }
      },
      "30": {
        "inputs": {
          "max_shift": 1.15,
          "base_shift": 0.5,
          "width": 512,
          "height": 512,
          "model": [
            "12",
            0
          ]
        },
        "class_type": "ModelSamplingFlux",
        "_meta": {
          "title": "ModelSamplingFlux"
        }
      },
      "38": {
        "inputs": {
          "transparency": false,
          "model": "isnet-general-use",
          "post_processing": false,
          "only_mask": true,
          "alpha_matting": false,
          "alpha_matting_foreground_threshold": 240,
          "alpha_matting_background_threshold": 10,
          "alpha_matting_erode_size": 10,
          "background_color": "white",
          "images": [
            "8",
            0
          ]
        },
        "class_type": "Image Rembg (Remove Background)",
        "_meta": {
          "title": "Image Rembg (Remove Background)"
        }
      },
      "41": {
        "inputs": {
          "channel": "red",
          "image": [
            "38",
            0
          ]
        },
        "class_type": "ImageToMask",
        "_meta": {
          "title": "Convert Image to Mask"
        }
      },
      "49": {
        "inputs": {
          "mesh_file_path": [
            "66",
            0
          ]
        },
        "class_type": "[Comfy3D] Preview 3DMesh",
        "_meta": {
          "title": "Preview 3DMesh"
        }
      },
      "60": {
        "inputs": {
          "model_name": "instant_mesh_large.ckpt"
        },
        "class_type": "[Comfy3D] Load InstantMesh Reconstruction Model",
        "_meta": {
          "title": "Load InstantMesh Reconstruction Model"
        }
      },
      "61": {
        "inputs": {
          "orbit_camera_fovy": 30,
          "texture_resolution": 1024,
          "lrm_model": [
            "60",
            0
          ],
          "multiview_images": [
            "63",
            0
          ],
          "orbit_camera_poses": [
            "63",
            2
          ]
        },
        "class_type": "[Comfy3D] InstantMesh Reconstruction Model",
        "_meta": {
          "title": "InstantMesh Reconstruction Model"
        }
      },
      "62": {
        "inputs": {
          "diffusers_pipeline_name": "Era3DPipeline",
          "repo_id": "pengHTYX/MacLab-Era3D-512-6view",
          "custom_pipeline": "",
          "force_download": false,
          "checkpoint_sub_dir": ""
        },
        "class_type": "[Comfy3D] Load Diffusers Pipeline",
        "_meta": {
          "title": "Load Diffusers Pipeline"
        }
      },
      "63": {
        "inputs": {
          "image_crop_size": 420,
          "seed": [
            "68",
            0
          ],
          "guidance_scale": 3,
          "num_inference_steps": 40,
          "eta": 1,
          "radius": 4,
          "era3d_pipe": [
            "62",
            0
          ],
          "reference_image": [
            "8",
            0
          ],
          "reference_mask": [
            "41",
            0
          ]
        },
        "class_type": "[Comfy3D] Era3D MVDiffusion Model",
        "_meta": {
          "title": "Era3D MVDiffusion Model"
        }
      },
      "64": {
        "inputs": {
          "images": [
            "63",
            0
          ]
        },
        "class_type": "PreviewImage",
        "_meta": {
          "title": "Preview Image"
        }
      },
      "65": {
        "inputs": {
          "axis_x_to": "+y",
          "axis_y_to": "+z",
          "axis_z_to": "+x",
          "flip_normal": false,
          "scale": 1,
          "mesh": [
            "61",
            0
          ]
        },
        "class_type": "[Comfy3D] Switch Mesh Axis",
        "_meta": {
          "title": "Switch Mesh Axis"
        }
      },
      "66": {
        "inputs": {
          "save_path": "Mesh.obj",
          "mesh": [
            "65",
            0
          ]
        },
        "class_type": "[Comfy3D] Save 3D Mesh",
        "_meta": {
          "title": "Save 3D Mesh"
        }
      },
      "68": {
        "inputs": {
          "seed": %SEED%
        },
        "class_type": "Seed Generator",
        "_meta": {
          "title": "Seed Generator"
        }
      },
      "69": {
        "inputs": {
          "images": [
            "38",
            0
          ]
        },
        "class_type": "PreviewImage",
        "_meta": {
          "title": "Preview Image"
        }
      }
    },
        
    "client_id": "%CLIENT_ID%"
})delim";


FString JsonString_StableFast3D = R"delim({
    "prompt" : {
      "6": {
        "inputs": {
          "text": "%POSITIVE_PROMPT%",
          "clip": [
            "11",
            0
          ]
        },
        "class_type": "CLIPTextEncode",
        "_meta": {
          "title": "CLIP Text Encode (Positive Prompt)"
        }
      },
      "8": {
        "inputs": {
          "samples": [
            "13",
            0
          ],
          "vae": [
            "10",
            0
          ]
        },
        "class_type": "VAEDecode",
        "_meta": {
          "title": "VAE Decode"
        }
      },
      "9": {
        "inputs": {
          "filename_prefix": "ComfyUI",
          "images": [
            "8",
            0
          ]
        },
        "class_type": "SaveImage",
        "_meta": {
          "title": "Save Image"
        }
      },
      "10": {
        "inputs": {
          "vae_name": "ae.safetensors"
        },
        "class_type": "VAELoader",
        "_meta": {
          "title": "Load VAE"
        }
      },
      "11": {
        "inputs": {
          "clip_name1": "t5xxl_fp16.safetensors",
          "clip_name2": "clip_l.safetensors",
          "type": "flux"
        },
        "class_type": "DualCLIPLoader",
        "_meta": {
          "title": "DualCLIPLoader"
        }
      },
      "12": {
        "inputs": {
          "unet_name": "flux1-dev.safetensors",
          "weight_dtype": "default"
        },
        "class_type": "UNETLoader",
        "_meta": {
          "title": "Load Diffusion Model"
        }
      },
      "13": {
        "inputs": {
          "noise": [
            "25",
            0
          ],
          "guider": [
            "22",
            0
          ],
          "sampler": [
            "16",
            0
          ],
          "sigmas": [
            "17",
            0
          ],
          "latent_image": [
            "27",
            0
          ]
        },
        "class_type": "SamplerCustomAdvanced",
        "_meta": {
          "title": "SamplerCustomAdvanced"
        }
      },
      "16": {
        "inputs": {
          "sampler_name": "euler"
        },
        "class_type": "KSamplerSelect",
        "_meta": {
          "title": "KSamplerSelect"
        }
      },
      "17": {
        "inputs": {
          "scheduler": "simple",
          "steps": 20,
          "denoise": 1,
          "model": [
            "30",
            0
          ]
        },
        "class_type": "BasicScheduler",
        "_meta": {
          "title": "BasicScheduler"
        }
      },
      "22": {
        "inputs": {
          "model": [
            "30",
            0
          ],
          "conditioning": [
            "26",
            0
          ]
        },
        "class_type": "BasicGuider",
        "_meta": {
          "title": "BasicGuider"
        }
      },
      "25": {
        "inputs": {
          "noise_seed": [
            "50",
            0
          ]
        },
        "class_type": "RandomNoise",
        "_meta": {
          "title": "RandomNoise"
        }
      },
      "26": {
        "inputs": {
          "guidance": 3.5,
          "conditioning": [
            "6",
            0
          ]
        },
        "class_type": "FluxGuidance",
        "_meta": {
          "title": "FluxGuidance"
        }
      },
      "27": {
        "inputs": {
          "width": 512,
          "height": 512,
          "batch_size": 1
        },
        "class_type": "EmptySD3LatentImage",
        "_meta": {
          "title": "EmptySD3LatentImage"
        }
      },
      "30": {
        "inputs": {
          "max_shift": 1.15,
          "base_shift": 0.5,
          "width": 512,
          "height": 512,
          "model": [
            "12",
            0
          ]
        },
        "class_type": "ModelSamplingFlux",
        "_meta": {
          "title": "ModelSamplingFlux"
        }
      },
      "38": {
        "inputs": {
          "transparency": true,
          "model": "isnet-general-use",
          "post_processing": false,
          "only_mask": true,
          "alpha_matting": false,
          "alpha_matting_foreground_threshold": 240,
          "alpha_matting_background_threshold": 10,
          "alpha_matting_erode_size": 10,
          "background_color": "none",
          "images": [
            "8",
            0
          ]
        },
        "class_type": "Image Rembg (Remove Background)",
        "_meta": {
          "title": "Image Rembg (Remove Background)"
        }
      },
      "41": {
        "inputs": {
          "channel": "red",
          "image": [
            "38",
            0
          ]
        },
        "class_type": "ImageToMask",
        "_meta": {
          "title": "Convert Image to Mask"
        }
      },
      "42": {
        "inputs": {
          "model_name": "model.safetensors"
        },
        "class_type": "[Comfy3D] Load SF3D Model",
        "_meta": {
          "title": "Load SF3D Model"
        }
      },
      "43": {
        "inputs": {
          "texture_resolution": 2048,
          "remesh_option": "None",
          "sf3d_model": [
            "42",
            0
          ],
          "reference_image": [
            "8",
            0
          ],
          "reference_mask": [
            "41",
            0
          ]
        },
        "class_type": "[Comfy3D] StableFast3D",
        "_meta": {
          "title": "StableFast3D"
        }
      },
      "44": {
        "inputs": {
          "axis_x_to": "-x",
          "axis_y_to": "+y",
          "axis_z_to": "-z",
          "flip_normal": false,
          "scale": 1,
          "mesh": [
            "43",
            0
          ]
        },
        "class_type": "[Comfy3D] Switch Mesh Axis",
        "_meta": {
          "title": "Switch Mesh Axis"
        }
      },
      "45": {
        "inputs": {
          "save_path": "Mesh.obj",
          "mesh": [
            "44",
            0
          ]
        },
        "class_type": "[Comfy3D] Save 3D Mesh",
        "_meta": {
          "title": "Save 3D Mesh"
        }
      },
      "49": {
        "inputs": {
          "mesh_file_path": [
            "45",
            0
          ],
          "preview3d": null
        },
        "class_type": "[Comfy3D] Preview 3DMesh",
        "_meta": {
          "title": "Preview 3DMesh"
        }
      },
      "50": {
        "inputs": {
          "seed": %SEED%
        },
        "class_type": "Seed Generator",
        "_meta": {
          "title": "Seed Generator"
        }
      },
      "51": {
        "inputs": {
          "images": [
            "38",
            0
          ]
        },
        "class_type": "PreviewImage",
        "_meta": {
          "title": "Preview Image"
        }
      }
    },

    "client_id": "%CLIENT_ID%"
})delim";

void AComfUIController::SendRequest_InstantMesh()
{
    FString Prompt = JsonString_InstantMesh.Replace(TEXT("%CLIENT_ID%"), *ClientID);
    Prompt = Prompt.Replace(TEXT("%POSITIVE_PROMPT%"), *PositivePrompt);
    Prompt = Prompt.Replace(TEXT("%NEGATIVE_PROMPT%"), *NegativePrompt);
    Prompt = Prompt.Replace(TEXT("%SEED%"), *Seed);
    SendPrompt(Prompt);
}

void AComfUIController::SendRequest_StableFast3D()
{
    FString Prompt = JsonString_StableFast3D.Replace(TEXT("%CLIENT_ID%"), *ClientID);
    Prompt = Prompt.Replace(TEXT("%POSITIVE_PROMPT%"), *PositivePrompt);
    Prompt = Prompt.Replace(TEXT("%NEGATIVE_PROMPT%"), *NegativePrompt);
    Prompt = Prompt.Replace(TEXT("%SEED%"), *Seed);
    SendPrompt(Prompt);
}