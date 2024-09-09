#pragma once

#include "Engine/EngineTypes.h"
#include "Containers/UnrealString.h"
#include "Delegates/IDelegateInstance.h"

DECLARE_LOG_CATEGORY_EXTERN(LogComfUIWebSocketConnection, Log, All);

class IWebSocket;

DECLARE_MULTICAST_DELEGATE(FOnConnectedToBackendDelegate);
DECLARE_MULTICAST_DELEGATE(FOnDisconnectedFromBackendDelegate);

class FComfUIWebSocketConnection
{
public:

    void RegisterForCommandCallback(const FString& MsgType, const TFunction<void(const TSharedPtr<FJsonObject>&)>& CallbackFunction);

    FComfUIWebSocketConnection();
    ~FComfUIWebSocketConnection();

    void ConnectToBackend(const FString& BackendIP, const int BackendPort, const FString& ClientID);
    void DisconnectFromBackend();
   
    FOnConnectedToBackendDelegate OnConnectedToBackendDelegate;
    FOnDisconnectedFromBackendDelegate OnDisconnectedFromBackendDelegate;

private:
    void ReconnectToBackend() const;
    void OnConnected() const;
    void OnConnectionError(const FString& Error) const;
    void OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean) const;
    void OnMessage(const FString& Msg) const;

    FDelegateHandle OnConnectedHandle;
    FDelegateHandle OnConnectionErrorHandle;
    FDelegateHandle OnClosedHandle;
    FDelegateHandle OnMessageHandle;

    TSharedPtr<IWebSocket> WebSocket;

    TMultiMap<FString, TFunction<void(const TSharedPtr<FJsonObject>& JsonMsg)>> MessageTypeCallbacks;

    // DEBUG
    double ComputeTime = 0.f;
};
