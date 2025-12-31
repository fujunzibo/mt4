#ifndef FOREX_COM_API_H
#define FOREX_COM_API_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Forward declarations
class HttpClient;

namespace ForexComAPI {

// API Configuration
struct APIConfig {
    std::string apiKey;           // API Key from forex.com
    std::string apiSecret;        // API Secret from forex.com
    std::string baseUrl;          // API base URL (e.g., "https://api.forex.com/v1")
    std::string wsUrl;            // WebSocket URL (e.g., "wss://api-demo.forex.com/streaming")
    bool useSandbox;              // Use sandbox environment
    int timeoutMs;                // Request timeout in milliseconds
    
    APIConfig() : useSandbox(false), timeoutMs(5000) {}
};

// Quote data structure
struct Quote {
    std::string symbol;           // Trading symbol (e.g., "EURUSD")
    double bid;                   // Bid price
    double ask;                   // Ask price
    unsigned long long timestamp; // Timestamp in milliseconds
    bool isValid;                 // Whether the quote is valid
    
    Quote() : bid(0.0), ask(0.0), timestamp(0), isValid(false) {}
};

// Order types
enum class OrderType {
    MARKET,                       // Market order
    LIMIT,                        // Limit order
    STOP                          // Stop order
};

// Order side
enum class OrderSide {
    BUY,                          // Buy order
    SELL                          // Sell order
};

// Order request structure
struct OrderRequest {
    std::string symbol;           // Trading symbol
    OrderSide side;               // Buy or Sell
    double quantity;              // Order quantity (lots)
    OrderType type;               // Order type
    double price;                // Price for limit/stop orders (0 for market)
    double stopLoss;              // Stop loss price (optional)
    double takeProfit;           // Take profit price (optional)
    std::string clientOrderId;    // Client-side order ID (optional)
    
    OrderRequest() : quantity(0.0), type(OrderType::MARKET), 
                    price(0.0), stopLoss(0.0), takeProfit(0.0) {}
};

// Order response structure
struct OrderResponse {
    std::string orderId;          // Server-side order ID
    std::string clientOrderId;    // Client-side order ID
    std::string symbol;           // Trading symbol
    OrderSide side;               // Buy or Sell
    double quantity;              // Order quantity
    double price;                 // Execution price
    std::string status;           // Order status (e.g., "FILLED", "PENDING", "REJECTED")
    unsigned long long timestamp; // Timestamp in milliseconds
    std::string message;          // Response message (error or success)
    bool isSuccess;               // Whether the order was successful
    
    OrderResponse() : quantity(0.0), price(0.0), timestamp(0), isSuccess(false) {}
};

// Account information structure
struct AccountInfo {
    std::string accountId;        // Account ID
    double balance;               // Account balance
    double equity;                // Account equity
    double margin;                // Used margin
    double freeMargin;            // Free margin
    double marginLevel;           // Margin level (%)
    bool isValid;                 // Whether the data is valid
    
    AccountInfo() : balance(0.0), equity(0.0), margin(0.0), 
                    freeMargin(0.0), marginLevel(0.0), isValid(false) {}
};

// Position information structure
struct PositionInfo {
    std::string positionId;       // Position ID
    std::string symbol;           // Trading symbol
    OrderSide side;               // Buy or Sell
    double quantity;              // Position quantity
    double openPrice;             // Open price
    double currentPrice;          // Current market price
    double profit;                // Unrealized profit/loss
    double stopLoss;              // Stop loss price
    double takeProfit;            // Take profit price
    unsigned long long openTime;  // Open time in milliseconds
    bool isValid;                 // Whether the data is valid
    
    PositionInfo() : quantity(0.0), openPrice(0.0), currentPrice(0.0),
                     profit(0.0), stopLoss(0.0), takeProfit(0.0),
                     openTime(0), isValid(false) {}
};

// Main API Client Class
class ForexComAPIClient {
public:
    ForexComAPIClient(const APIConfig& config);
    ~ForexComAPIClient();
    
    // Initialize connection
    bool Initialize();
    
    // Get real-time quote for a symbol
    bool GetQuote(const std::string& symbol, Quote& quote);
    
    // Get quotes for multiple symbols
    bool GetQuotes(const std::vector<std::string>& symbols, std::vector<Quote>& quotes);
    
    // Place an order
    bool PlaceOrder(const OrderRequest& request, OrderResponse& response);
    
    // Cancel an order
    bool CancelOrder(const std::string& orderId, std::string& message);
    
    // Get order status
    bool GetOrderStatus(const std::string& orderId, OrderResponse& response);
    
    // Get account information
    bool GetAccountInfo(AccountInfo& accountInfo);
    
    // Get all open positions
    bool GetOpenPositions(std::vector<PositionInfo>& positions);
    
    // Get position by symbol
    bool GetPosition(const std::string& symbol, PositionInfo& position);
    
    // Close a position
    bool ClosePosition(const std::string& positionId, std::string& message);
    
    // Close all positions
    bool CloseAllPositions(std::string& message);
    
    // WebSocket real-time quote streaming
    bool ConnectWebSocket();
    void DisconnectWebSocket();
    bool SubscribeQuote(const std::string& symbol);
    bool UnsubscribeQuote(const std::string& symbol);
    bool SubscribeQuotes(const std::vector<std::string>& symbols);
    bool IsWebSocketConnected() const;
    bool ReceiveQuote(Quote& quote, int timeoutMs = 0);
    void SetQuoteCallback(std::function<void(const Quote&)> callback);
    
    // Get last error message
    std::string GetLastError() const { return lastError_; }
    
    // Check if connected
    bool IsConnected() const { return isConnected_; }
    
private:
    APIConfig config_;
    bool isConnected_;
    std::string lastError_;
    void* httpClient_;  // Opaque pointer for HTTP client
    class WebSocketClient* wsClient_;  // WebSocket client for real-time quotes
    std::function<void(const Quote&)> quoteCallback_;  // Quote update callback
    
    // Internal helper methods
    bool MakeRequest(const std::string& method, const std::string& endpoint,
                     const std::string& body, std::string& response);
    bool GenerateSignature(const std::string& method, const std::string& endpoint,
                          const std::string& body, const std::string& timestamp,
                          std::string& signature);
    std::string GetTimestamp();
    bool ParseQuoteJSON(const std::string& json, Quote& quote);
    bool ParseOrderResponseJSON(const std::string& json, OrderResponse& response);
    bool ParseAccountInfoJSON(const std::string& json, AccountInfo& accountInfo);
    bool ParsePositionJSON(const std::string& json, PositionInfo& position);
    std::string OrderSideToString(OrderSide side);
    std::string OrderTypeToString(OrderType type);
    OrderSide StringToOrderSide(const std::string& str);
    OrderType StringToOrderType(const std::string& str);
};

} // namespace ForexComAPI

#endif // FOREX_COM_API_H
