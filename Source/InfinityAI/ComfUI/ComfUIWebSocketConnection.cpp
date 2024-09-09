#include "ComfUIWebSocketConnection.h"
#include "Serialization/JsonSerializer.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Policies/CondensedJsonPrintPolicy.h"

DEFINE_LOG_CATEGORY(LogComfUIWebSocketConnection)

inline FString JsonToString(const TSharedPtr<FJsonObject>& JsonObj, bool bPretty = true)
{
    FString Res;
    if (bPretty)
    {
        const auto JsonWriter = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Res);
        FJsonSerializer::Serialize(JsonObj.ToSharedRef(), JsonWriter);
    }
    else
    {
        const auto JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Res);
        FJsonSerializer::Serialize(JsonObj.ToSharedRef(), JsonWriter);
    }
    return Res;
}

void FComfUIWebSocketConnection::RegisterForCommandCallback(const FString& MsgType, const TFunction<void(const TSharedPtr<FJsonObject>&)>& CallbackFunction)
{
    MessageTypeCallbacks.Add(MsgType, CallbackFunction);
}

FComfUIWebSocketConnection::FComfUIWebSocketConnection()
{
    FModuleManager::LoadModuleChecked<FWebSocketsModule>("WebSockets");
}


FComfUIWebSocketConnection::~FComfUIWebSocketConnection()
{
    DisconnectFromBackend();
}

void FComfUIWebSocketConnection::ConnectToBackend(const FString& BackendIP, const int BackendPort, const FString& ClientID)
{
    const FString ComfUIURL = FString::Printf(TEXT("ws://%s:%d/ws?clientId=%s"), *BackendIP, BackendPort, *ClientID);
    WebSocket = FWebSocketsModule::Get().CreateWebSocket(ComfUIURL, TEXT(""));
    OnConnectedHandle = WebSocket->OnConnected().AddLambda([this]() { OnConnected(); });
    OnConnectionErrorHandle = WebSocket->OnConnectionError().AddLambda([this](const FString& Error) { OnConnectionError(Error); });
    OnClosedHandle = WebSocket->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean) { OnClosed(StatusCode, Reason, bWasClean); });
    OnMessageHandle = WebSocket->OnMessage().AddLambda([this](const FString& Msg) { OnMessage(Msg); });

    const FString StringLog = FString::Printf(TEXT("Connecting to ComfUI %s"), *ComfUIURL);
    UE_LOG(LogComfUIWebSocketConnection, Log, TEXT("%s"), *StringLog);
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, StringLog);

    WebSocket->Connect();
}

void FComfUIWebSocketConnection::ReconnectToBackend() const
{
    if (!WebSocket)
    {
        UE_LOG(LogComfUIWebSocketConnection, Log, TEXT("Can't reconnect to ComfUI, WebSocket was not created."));
        return;
    }

    const FString StringLog(TEXT("Reconnecting to ComfUI..."));
    UE_LOG(LogComfUIWebSocketConnection, Log, TEXT("%s"), *StringLog);
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, StringLog);
    WebSocket->Connect();
}

void FComfUIWebSocketConnection::DisconnectFromBackend()
{
    if (WebSocket)
    {
        WebSocket->OnConnected().Remove(OnConnectedHandle);
        WebSocket->OnConnectionError().Remove(OnConnectionErrorHandle);
        WebSocket->OnClosed().Remove(OnClosedHandle);
        WebSocket->OnMessage().Remove(OnMessageHandle);
        WebSocket->Close();
        WebSocket = nullptr;
    }
}

void FComfUIWebSocketConnection::OnConnected() const
{
    UE_LOG(LogComfUIWebSocketConnection, Log, TEXT("Connected to ComfUI"));
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("Connected to ComfUI"));
    OnConnectedToBackendDelegate.Broadcast();
}

void FComfUIWebSocketConnection::OnConnectionError(const FString& Error) const
{
    UE_LOG(LogComfUIWebSocketConnection, Error, TEXT("Failed to connect to ComfUI: %s"), *Error);
    OnDisconnectedFromBackendDelegate.Broadcast();
    ReconnectToBackend();
}

void FComfUIWebSocketConnection::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean) const
{
    UE_LOG(LogComfUIWebSocketConnection, Log, TEXT("Connection to ComfUI closed: \n\tstatus %d\n\treason: %s\n\twas clean: %s"), StatusCode, *Reason, bWasClean ? TEXT("true") : TEXT("false"));
    OnDisconnectedFromBackendDelegate.Broadcast();
    ReconnectToBackend();
}

void FComfUIWebSocketConnection::OnMessage(const FString& Msg) const
{
    UE_LOG(LogComfUIWebSocketConnection, Log, TEXT("Message from ComfUI: %s"), *Msg);

    TSharedPtr<FJsonObject> JsonMsg;
    const auto JsonReader = TJsonReaderFactory<TCHAR>::Create(Msg);
    if (!FJsonSerializer::Deserialize(JsonReader, JsonMsg))
    {
        UE_LOG(LogComfUIWebSocketConnection, Log, TEXT("Failed to parse ComfUI message:\n%s"), *Msg);
        return;
    }

    FString MsgType;
    if (!JsonMsg->TryGetStringField(TEXT("type"), MsgType))
    {
        UE_LOG(LogComfUIWebSocketConnection, Log, TEXT("Cannot find `type` field in ComfUI message:\n%s"), *Msg);
        return;
    }

    TArray<TFunction<void(const TSharedPtr<FJsonObject>&)>> Callbacks;
    MessageTypeCallbacks.MultiFind(MsgType, Callbacks);
    if (Callbacks.Num() == 0)
    {
        UE_LOG(LogComfUIWebSocketConnection, Error, TEXT("Unsupported message type `%s` received from ComfUI"), *MsgType);
        //WebSocket->Close(4001, TEXT("Unsupported message received: ") + MsgType);
    }
    else
    {
        for (auto& Callback : Callbacks)
        {
            Callback(JsonMsg);
        }
    }
}
