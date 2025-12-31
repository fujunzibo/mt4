#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>

// HTTP Client interface for making REST API calls
class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    // Initialize HTTP client
    bool Initialize();
    
    // Make HTTP request
    bool MakeRequest(
        const std::string& method,      // GET, POST, DELETE, etc.
        const std::string& url,         // Full URL
        const std::string& headers,     // Additional headers (newline separated)
        const std::string& body,        // Request body (for POST/PUT)
        std::string& response,          // Response body
        int& statusCode,                 // HTTP status code
        int timeoutMs = 5000            // Timeout in milliseconds
    );
    
    // Cleanup
    void Cleanup();
    
    // Get last error
    std::string GetLastError() const { return lastError_; }
    
private:
    void* httpSession_;  // Opaque pointer to HTTP session
    std::string lastError_;
    
    // Helper methods
    bool ParseUrl(const std::string& url, std::string& scheme, 
                  std::string& host, std::string& path, int& port);
};

#endif // HTTP_CLIENT_H


