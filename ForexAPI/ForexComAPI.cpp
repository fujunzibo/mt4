// Forex.com API Client Implementation

#define _CRT_SECURE_NO_WARNINGS

#include "ForexComAPI.h"
#include "HttpClient.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <regex>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
    #include <wincrypt.h>
    #pragma comment(lib, "crypt32.lib")
#else
    #include <openssl/hmac.h>
    #include <openssl/sha.h>
#endif

namespace ForexComAPI {

// Forward declaration for WebSocketClient (simplified version)
class WebSocketClient {
public:
    bool Initialize() { return true; }
    void Cleanup() {}
    bool Connect(const std::string& url, const std::string& headers) { return false; }
    void Disconnect() {}
    bool SendText(const std::string& text) { return false; }
    bool Receive(std::string& message, int timeoutMs) { return false; }
    bool IsConnected() const { return false; }
    void SetMessageCallback(std::function<void(const std::string&)> callback) {}
};

// Constructor
ForexComAPIClient::ForexComAPIClient(const APIConfig& config)
    : config_(config), isConnected_(false), httpClient_(nullptr), wsClient_(nullptr) {
    lastError_ = "";
    httpClient_ = new HttpClient();
    wsClient_ = new WebSocketClient();
}

// Destructor
ForexComAPIClient::~ForexComAPIClient() {
    DisconnectWebSocket();
    if (wsClient_) {
        wsClient_->Cleanup();
        delete wsClient_;
    }
    if (httpClient_) {
        static_cast<HttpClient*>(httpClient_)->Cleanup();
        delete static_cast<HttpClient*>(httpClient_);
    }
}

// Initialize
bool ForexComAPIClient::Initialize() {
    lastError_ = "";
    
    if (config_.apiKey.empty() || config_.apiSecret.empty()) {
        lastError_ = "API Key or Secret is empty";
        return false;
    }
    
    if (!httpClient_) {
        httpClient_ = new HttpClient();
    }
    
    if (!static_cast<HttpClient*>(httpClient_)->Initialize()) {
        lastError_ = "Failed to initialize HTTP client: " + 
                     static_cast<HttpClient*>(httpClient_)->GetLastError();
        return false;
    }
    
    isConnected_ = true;
    return true;
}

// Get Quote
bool ForexComAPIClient::GetQuote(const std::string& symbol, Quote& quote) {
    lastError_ = "";
    quote.symbol = symbol;
    quote.isValid = false;
    
    std::string endpoint = "/quotes/" + symbol;
    std::string response;
    
    if (!MakeRequest("GET", endpoint, "", response)) {
        return false;
    }
    
    if (!ParseQuoteJSON(response, quote)) {
        lastError_ = "Failed to parse quote JSON";
        return false;
    }
    
    quote.isValid = true;
    return true;
}

// Get Quotes (multiple)
bool ForexComAPIClient::GetQuotes(const std::vector<std::string>& symbols, std::vector<Quote>& quotes) {
    lastError_ = "";
    quotes.clear();
    
    for (const auto& symbol : symbols) {
        Quote quote;
        if (GetQuote(symbol, quote)) {
            quotes.push_back(quote);
        }
    }
    
    return !quotes.empty();
}

// Place Order
bool ForexComAPIClient::PlaceOrder(const OrderRequest& request, OrderResponse& response) {
    lastError_ = "";
    
    std::string endpoint = "/orders";
    std::ostringstream body;
    body << "{"
         << "\"symbol\":\"" << request.symbol << "\","
         << "\"side\":\"" << OrderSideToString(request.side) << "\","
         << "\"quantity\":" << request.quantity << ","
         << "\"type\":\"" << OrderTypeToString(request.type) << "\"";
    
    if (request.price > 0.0) {
        body << ",\"price\":" << request.price;
    }
    if (request.stopLoss > 0.0) {
        body << ",\"stopLoss\":" << request.stopLoss;
    }
    if (request.takeProfit > 0.0) {
        body << ",\"takeProfit\":" << request.takeProfit;
    }
    if (!request.clientOrderId.empty()) {
        body << ",\"clientOrderId\":\"" << request.clientOrderId << "\"";
    }
    body << "}";
    
    std::string jsonResponse;
    if (!MakeRequest("POST", endpoint, body.str(), jsonResponse)) {
        return false;
    }
    
    if (!ParseOrderResponseJSON(jsonResponse, response)) {
        lastError_ = "Failed to parse order response JSON";
        return false;
    }
    
    return true;
}

// Cancel Order
bool ForexComAPIClient::CancelOrder(const std::string& orderId, std::string& message) {
    lastError_ = "";
    
    std::string endpoint = "/orders/" + orderId;
    std::string response;
    
    if (!MakeRequest("DELETE", endpoint, "", response)) {
        message = lastError_;
        return false;
    }
    
    message = "Order cancelled successfully";
    return true;
}

// Get Order Status
bool ForexComAPIClient::GetOrderStatus(const std::string& orderId, OrderResponse& response) {
    lastError_ = "";
    
    std::string endpoint = "/orders/" + orderId;
    std::string jsonResponse;
    
    if (!MakeRequest("GET", endpoint, "", jsonResponse)) {
        return false;
    }
    
    if (!ParseOrderResponseJSON(jsonResponse, response)) {
        lastError_ = "Failed to parse order status JSON";
        return false;
    }
    
    return true;
}

// Get Account Info
bool ForexComAPIClient::GetAccountInfo(AccountInfo& accountInfo) {
    lastError_ = "";
    accountInfo.isValid = false;
    
    std::string endpoint = "/accounts";
    std::string response;
    
    if (!MakeRequest("GET", endpoint, "", response)) {
        return false;
    }
    
    if (!ParseAccountInfoJSON(response, accountInfo)) {
        lastError_ = "Failed to parse account info JSON";
        return false;
    }
    
    accountInfo.isValid = true;
    return true;
}

// Get Open Positions
bool ForexComAPIClient::GetOpenPositions(std::vector<PositionInfo>& positions) {
    lastError_ = "";
    positions.clear();
    
    std::string endpoint = "/positions";
    std::string response;
    
    if (!MakeRequest("GET", endpoint, "", response)) {
        return false;
    }
    
    // Simple JSON parsing (should use proper JSON library)
    std::regex posRegex("\\{\"positionId\":\"([^\"]+)\",\"symbol\":\"([^\"]+)\",\"side\":\"([^\"]+)\",\"quantity\":([\\d.]+),");
    std::sregex_iterator iter(response.begin(), response.end(), posRegex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        PositionInfo pos;
        pos.positionId = (*iter)[1].str();
        pos.symbol = (*iter)[2].str();
        pos.side = StringToOrderSide((*iter)[3].str());
        pos.quantity = std::stod((*iter)[4].str());
        pos.isValid = true;
        positions.push_back(pos);
    }
    
    return !positions.empty();
}

// Get Position by Symbol
bool ForexComAPIClient::GetPosition(const std::string& symbol, PositionInfo& position) {
    lastError_ = "";
    position.isValid = false;
    
    std::vector<PositionInfo> positions;
    if (!GetOpenPositions(positions)) {
        return false;
    }
    
    for (const auto& pos : positions) {
        if (pos.symbol == symbol) {
            position = pos;
            return true;
        }
    }
    
    lastError_ = "Position not found for symbol: " + symbol;
    return false;
}

// Close Position
bool ForexComAPIClient::ClosePosition(const std::string& positionId, std::string& message) {
    lastError_ = "";
    
    std::string endpoint = "/positions/" + positionId;
    std::string response;
    
    if (!MakeRequest("DELETE", endpoint, "", response)) {
        message = lastError_;
        return false;
    }
    
    message = "Position closed successfully";
    return true;
}

// Close All Positions
bool ForexComAPIClient::CloseAllPositions(std::string& message) {
    lastError_ = "";
    
    std::vector<PositionInfo> positions;
    if (!GetOpenPositions(positions)) {
        message = "No positions to close";
        return false;
    }
    
    bool allSuccess = true;
    std::ostringstream msg;
    for (const auto& pos : positions) {
        std::string closeMsg;
        if (!ClosePosition(pos.positionId, closeMsg)) {
            allSuccess = false;
            msg << "Failed to close " << pos.positionId << ": " << closeMsg << "; ";
        }
    }
    
    message = allSuccess ? "All positions closed successfully" : msg.str();
    return allSuccess;
}

// WebSocket Methods
bool ForexComAPIClient::ConnectWebSocket() {
    lastError_ = "";
    
    if (!wsClient_) {
        wsClient_ = new WebSocketClient();
    }
    
    if (!wsClient_->Initialize()) {
        lastError_ = "Failed to initialize WebSocket client";
        return false;
    }
    
    // Prepare authentication headers
    std::string timestamp = GetTimestamp();
    std::string signature;
    if (!GenerateSignature("GET", "/streaming", "", timestamp, signature)) {
        lastError_ = "Failed to generate WebSocket signature";
        return false;
    }
    
    std::ostringstream headers;
    headers << "X-API-Key: " << config_.apiKey << "\r\n"
            << "X-Timestamp: " << timestamp << "\r\n"
            << "X-Signature: " << signature << "\r\n";
    
    if (!wsClient_->Connect(config_.wsUrl, headers.str())) {
        lastError_ = "Failed to connect WebSocket";
        return false;
    }
    
    return true;
}

void ForexComAPIClient::DisconnectWebSocket() {
    if (wsClient_ && wsClient_->IsConnected()) {
        wsClient_->Disconnect();
    }
}

bool ForexComAPIClient::SubscribeQuote(const std::string& symbol) {
    if (!wsClient_ || !wsClient_->IsConnected()) {
        lastError_ = "WebSocket not connected";
        return false;
    }
    
    std::ostringstream msg;
    msg << "{\"action\":\"subscribe\",\"symbol\":\"" << symbol << "\"}";
    
    return wsClient_->SendText(msg.str());
}

bool ForexComAPIClient::UnsubscribeQuote(const std::string& symbol) {
    if (!wsClient_ || !wsClient_->IsConnected()) {
        lastError_ = "WebSocket not connected";
        return false;
    }
    
    std::ostringstream msg;
    msg << "{\"action\":\"unsubscribe\",\"symbol\":\"" << symbol << "\"}";
    
    return wsClient_->SendText(msg.str());
}

bool ForexComAPIClient::SubscribeQuotes(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        if (!SubscribeQuote(symbol)) {
            return false;
        }
    }
    return true;
}

bool ForexComAPIClient::IsWebSocketConnected() const {
    return wsClient_ && wsClient_->IsConnected();
}

bool ForexComAPIClient::ReceiveQuote(Quote& quote, int timeoutMs) {
    if (!wsClient_ || !wsClient_->IsConnected()) {
        lastError_ = "WebSocket not connected";
        return false;
    }
    
    std::string message;
    if (!wsClient_->Receive(message, timeoutMs)) {
        return false;
    }
    
    return ParseQuoteJSON(message, quote);
}

void ForexComAPIClient::SetQuoteCallback(std::function<void(const Quote&)> callback) {
    quoteCallback_ = callback;
    if (wsClient_) {
        wsClient_->SetMessageCallback([this](const std::string& msg) {
            Quote quote;
            if (ParseQuoteJSON(msg, quote) && quoteCallback_) {
                quoteCallback_(quote);
            }
        });
    }
}

// Private Helper Methods
bool ForexComAPIClient::MakeRequest(const std::string& method, const std::string& endpoint,
                                    const std::string& body, std::string& response) {
    if (!httpClient_) {
        lastError_ = "HTTP client not initialized";
        return false;
    }
    
    std::string url = config_.baseUrl + endpoint;
    std::string timestamp = GetTimestamp();
    std::string signature;
    
    if (!GenerateSignature(method, endpoint, body, timestamp, signature)) {
        lastError_ = "Failed to generate signature";
        return false;
    }
    
    std::ostringstream headers;
    headers << "X-API-Key: " << config_.apiKey << "\r\n"
            << "X-Timestamp: " << timestamp << "\r\n"
            << "X-Signature: " << signature << "\r\n"
            << "Content-Type: application/json\r\n";
    
    int statusCode = 0;
    HttpClient* client = static_cast<HttpClient*>(httpClient_);
    if (!client->MakeRequest(method, url, headers.str(), body, response, statusCode, config_.timeoutMs)) {
        lastError_ = "HTTP request failed: " + client->GetLastError();
        return false;
    }
    
    if (statusCode < 200 || statusCode >= 300) {
        lastError_ = "HTTP error: " + std::to_string(statusCode) + " - " + response;
        return false;
    }
    
    return true;
}

bool ForexComAPIClient::GenerateSignature(const std::string& method, const std::string& endpoint,
                                          const std::string& body, const std::string& timestamp,
                                          std::string& signature) {
    // Create signature string
    std::ostringstream sigStr;
    sigStr << method << "\n" << endpoint << "\n" << body << "\n" << timestamp;
    
    // TODO: Implement proper HMAC-SHA256
    // For now, use a placeholder
    signature = "placeholder_signature_" + timestamp;
    return true;
}

std::string ForexComAPIClient::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
#ifdef _WIN32
    struct tm timeinfo;
    gmtime_s(&timeinfo, &time_t);
    oss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
#else
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
#endif
    oss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    return oss.str();
}

bool ForexComAPIClient::ParseQuoteJSON(const std::string& json, Quote& quote) {
    // Simple regex-based parsing (should use proper JSON library)
    std::regex bidRegex("\"bid\":([\\d.]+)");
    std::regex askRegex("\"ask\":([\\d.]+)");
    std::regex symbolRegex("\"symbol\":\"([^\"]+)\"");
    std::regex timestampRegex("\"timestamp\":(\\d+)");
    
    std::smatch match;
    if (std::regex_search(json, match, bidRegex)) {
        quote.bid = std::stod(match[1].str());
    }
    if (std::regex_search(json, match, askRegex)) {
        quote.ask = std::stod(match[1].str());
    }
    if (std::regex_search(json, match, symbolRegex)) {
        quote.symbol = match[1].str();
    }
    if (std::regex_search(json, match, timestampRegex)) {
        quote.timestamp = std::stoull(match[1].str());
    }
    
    return quote.bid > 0.0 && quote.ask > 0.0;
}

bool ForexComAPIClient::ParseOrderResponseJSON(const std::string& json, OrderResponse& response) {
    std::regex orderIdRegex("\"orderId\":\"([^\"]+)\"");
    std::regex statusRegex("\"status\":\"([^\"]+)\"");
    std::regex priceRegex("\"price\":([\\d.]+)");
    std::regex quantityRegex("\"quantity\":([\\d.]+)");
    
    std::smatch match;
    if (std::regex_search(json, match, orderIdRegex)) {
        response.orderId = match[1].str();
    }
    if (std::regex_search(json, match, statusRegex)) {
        response.status = match[1].str();
        response.isSuccess = (response.status == "FILLED" || response.status == "PENDING");
    }
    if (std::regex_search(json, match, priceRegex)) {
        response.price = std::stod(match[1].str());
    }
    if (std::regex_search(json, match, quantityRegex)) {
        response.quantity = std::stod(match[1].str());
    }
    
    return !response.orderId.empty();
}

bool ForexComAPIClient::ParseAccountInfoJSON(const std::string& json, AccountInfo& accountInfo) {
    std::regex balanceRegex("\"balance\":([\\d.]+)");
    std::regex equityRegex("\"equity\":([\\d.]+)");
    std::regex marginRegex("\"margin\":([\\d.]+)");
    
    std::smatch match;
    if (std::regex_search(json, match, balanceRegex)) {
        accountInfo.balance = std::stod(match[1].str());
    }
    if (std::regex_search(json, match, equityRegex)) {
        accountInfo.equity = std::stod(match[1].str());
    }
    if (std::regex_search(json, match, marginRegex)) {
        accountInfo.margin = std::stod(match[1].str());
        accountInfo.freeMargin = accountInfo.equity - accountInfo.margin;
    }
    
    return accountInfo.balance > 0.0;
}

bool ForexComAPIClient::ParsePositionJSON(const std::string& json, PositionInfo& position) {
    std::regex posIdRegex("\"positionId\":\"([^\"]+)\"");
    std::regex symbolRegex("\"symbol\":\"([^\"]+)\"");
    std::regex sideRegex("\"side\":\"([^\"]+)\"");
    std::regex quantityRegex("\"quantity\":([\\d.]+)");
    std::regex openPriceRegex("\"openPrice\":([\\d.]+)");
    std::regex currentPriceRegex("\"currentPrice\":([\\d.]+)");
    std::regex profitRegex("\"profit\":([\\d.-]+)");
    
    std::smatch match;
    if (std::regex_search(json, match, posIdRegex)) {
        position.positionId = match[1].str();
    }
    if (std::regex_search(json, match, symbolRegex)) {
        position.symbol = match[1].str();
    }
    if (std::regex_search(json, match, sideRegex)) {
        position.side = StringToOrderSide(match[1].str());
    }
    if (std::regex_search(json, match, quantityRegex)) {
        position.quantity = std::stod(match[1].str());
    }
    if (std::regex_search(json, match, openPriceRegex)) {
        position.openPrice = std::stod(match[1].str());
    }
    if (std::regex_search(json, match, currentPriceRegex)) {
        position.currentPrice = std::stod(match[1].str());
    }
    if (std::regex_search(json, match, profitRegex)) {
        position.profit = std::stod(match[1].str());
    }
    
    return !position.positionId.empty();
}

std::string ForexComAPIClient::OrderSideToString(OrderSide side) {
    return (side == OrderSide::BUY) ? "BUY" : "SELL";
}

std::string ForexComAPIClient::OrderTypeToString(OrderType type) {
    switch (type) {
        case OrderType::MARKET: return "MARKET";
        case OrderType::LIMIT: return "LIMIT";
        case OrderType::STOP: return "STOP";
        default: return "MARKET";
    }
}

OrderSide ForexComAPIClient::StringToOrderSide(const std::string& str) {
    if (str == "BUY" || str == "buy") return OrderSide::BUY;
    return OrderSide::SELL;
}

OrderType ForexComAPIClient::StringToOrderType(const std::string& str) {
    if (str == "LIMIT" || str == "limit") return OrderType::LIMIT;
    if (str == "STOP" || str == "stop") return OrderType::STOP;
    return OrderType::MARKET;
}

} // namespace ForexComAPI
