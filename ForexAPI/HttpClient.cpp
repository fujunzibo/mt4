// HTTP Client Implementation using WinHTTP (Windows) or CURL (cross-platform)

#include "HttpClient.h"
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
    #include <winhttp.h>
    #pragma comment(lib, "winhttp.lib")
    
    struct WinHttpSession {
        HINTERNET hSession;
        HINTERNET hConnect;
        std::string currentHost;
        int currentPort;
        
        WinHttpSession() : hSession(NULL), hConnect(NULL), currentPort(0) {}
    };
#else
    #include <curl/curl.h>
    
    struct CurlSession {
        CURL* curl;
        
        CurlSession() : curl(nullptr) {}
    };
#endif

HttpClient::HttpClient() : httpSession_(nullptr) {
    lastError_ = "";
}

HttpClient::~HttpClient() {
    Cleanup();
}

bool HttpClient::Initialize() {
    lastError_ = "";
    
#ifdef _WIN32
    WinHttpSession* session = new WinHttpSession();
    
    // Initialize WinHTTP session
    session->hSession = WinHttpOpen(
        L"ForexComAPI/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    
    if (!session->hSession) {
        lastError_ = "Failed to open WinHTTP session";
        delete session;
        return false;
    }
    
    httpSession_ = session;
    return true;
    
#else
    // CURL implementation
    CurlSession* session = new CurlSession();
    session->curl = curl_easy_init();
    
    if (!session->curl) {
        lastError_ = "Failed to initialize CURL";
        delete session;
        return false;
    }
    
    httpSession_ = session;
    return true;
#endif
}

void HttpClient::Cleanup() {
    if (!httpSession_) return;
    
#ifdef _WIN32
    WinHttpSession* session = static_cast<WinHttpSession*>(httpSession_);
    if (session->hConnect) {
        WinHttpCloseHandle(session->hConnect);
    }
    if (session->hSession) {
        WinHttpCloseHandle(session->hSession);
    }
    delete session;
#else
    CurlSession* session = static_cast<CurlSession*>(httpSession_);
    if (session->curl) {
        curl_easy_cleanup(session->curl);
    }
    delete session;
#endif
    
    httpSession_ = nullptr;
}

bool HttpClient::ParseUrl(const std::string& url, std::string& scheme,
                         std::string& host, std::string& path, int& port) {
    // Simple URL parsing
    size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) {
        lastError_ = "Invalid URL format";
        return false;
    }
    
    scheme = url.substr(0, schemeEnd);
    std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
    
    size_t hostStart = schemeEnd + 3;
    size_t pathStart = url.find('/', hostStart);
    size_t portStart = url.find(':', hostStart);
    
    if (pathStart == std::string::npos) {
        pathStart = url.length();
        path = "/";
    } else {
        path = url.substr(pathStart);
    }
    
    size_t hostEnd = (portStart != std::string::npos && portStart < pathStart) 
                     ? portStart : pathStart;
    
    if (portStart != std::string::npos && portStart < pathStart) {
        host = url.substr(hostStart, portStart - hostStart);
        std::string portStr = url.substr(portStart + 1, pathStart - portStart - 1);
        port = std::stoi(portStr);
    } else {
        host = url.substr(hostStart, hostEnd - hostStart);
        port = (scheme == "https") ? 443 : 80;
    }
    
    return true;
}

bool HttpClient::MakeRequest(const std::string& method,
                            const std::string& url,
                            const std::string& headers,
                            const std::string& body,
                            std::string& response,
                            int& statusCode,
                            int timeoutMs) {
    lastError_ = "";
    response.clear();
    statusCode = 0;
    
    if (!httpSession_) {
        lastError_ = "HTTP client not initialized";
        return false;
    }
    
    // Parse URL
    std::string scheme, host, path;
    int port;
    if (!ParseUrl(url, scheme, host, path, port)) {
        return false;
    }
    
#ifdef _WIN32
    WinHttpSession* session = static_cast<WinHttpSession*>(httpSession_);
    
    // Convert strings to wide strings
    std::wstring wHost(host.begin(), host.end());
    
    // Connect to host (reuse connection if same host/port)
    if (session->hConnect == NULL || 
        session->currentHost != host || 
        session->currentPort != port) {
        if (session->hConnect) {
            WinHttpCloseHandle(session->hConnect);
        }
        
        session->hConnect = WinHttpConnect(session->hSession, wHost.c_str(), port, 0);
        if (!session->hConnect) {
            lastError_ = "Failed to connect to host";
            return false;
        }
        
        session->currentHost = host;
        session->currentPort = port;
    }
    
    // Create request
    std::wstring wPath(path.begin(), path.end());
    HINTERNET hRequest = WinHttpOpenRequest(
        session->hConnect,
        std::wstring(method.begin(), method.end()).c_str(),
        wPath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        (scheme == "https") ? WINHTTP_FLAG_SECURE : 0
    );
    
    if (!hRequest) {
        lastError_ = "Failed to create HTTP request";
        return false;
    }
    
    // Set timeout
    DWORD timeout = timeoutMs;
    WinHttpSetTimeouts(hRequest, timeout, timeout, timeout, timeout);
    
    // Set headers
    std::wstring wHeaders;
    if (!headers.empty()) {
        wHeaders = std::wstring(headers.begin(), headers.end());
    }
    
    // Send request
    BOOL bResult = WinHttpSendRequest(
        hRequest,
        wHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : wHeaders.c_str(),
        -1,
        body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.c_str(),
        body.length(),
        body.length(),
        0
    );
    
    if (!bResult) {
        lastError_ = "Failed to send HTTP request";
        WinHttpCloseHandle(hRequest);
        return false;
    }
    
    // Receive response
    bResult = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResult) {
        lastError_ = "Failed to receive HTTP response";
        WinHttpCloseHandle(hRequest);
        return false;
    }
    
    // Get status code
    DWORD dwStatusCode = 0;
    DWORD dwStatusCodeSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &dwStatusCode,
        &dwStatusCodeSize,
        WINHTTP_NO_HEADER_INDEX
    );
    statusCode = dwStatusCode;
    
    // Read response body
    DWORD dwBytesAvailable = 0;
    do {
        if (!WinHttpQueryDataAvailable(hRequest, &dwBytesAvailable)) {
            break;
        }
        
        if (dwBytesAvailable == 0) break;
        
        char* pszOutBuffer = new char[dwBytesAvailable + 1];
        DWORD dwBytesRead = 0;
        
        if (WinHttpReadData(hRequest, pszOutBuffer, dwBytesAvailable, &dwBytesRead)) {
            pszOutBuffer[dwBytesRead] = '\0';
            response += pszOutBuffer;
        }
        
        delete[] pszOutBuffer;
    } while (dwBytesAvailable > 0);
    
    WinHttpCloseHandle(hRequest);
    
    return (statusCode >= 200 && statusCode < 300);
    
#else
    // CURL implementation
    CurlSession* session = static_cast<CurlSession*>(httpSession_);
    CURL* curl = session->curl;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)timeoutMs);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, 
                                                       size_t nmemb, void* userp) -> size_t {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    // Set headers
    struct curl_slist* headerList = NULL;
    if (!headers.empty()) {
        std::istringstream iss(headers);
        std::string header;
        while (std::getline(iss, header)) {
            headerList = curl_slist_append(headerList, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }
    
    // Set request body
    if (!body.empty() && (method == "POST" || method == "PUT")) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    }
    
    CURLcode res = curl_easy_perform(curl);
    
    if (headerList) {
        curl_slist_free_all(headerList);
    }
    
    if (res != CURLE_OK) {
        lastError_ = curl_easy_strerror(res);
        return false;
    }
    
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    
    return (statusCode >= 200 && statusCode < 300);
#endif
}


