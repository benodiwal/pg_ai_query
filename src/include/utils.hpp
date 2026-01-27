#pragma once
#include <string>
#include <utility>
#include <algorithm>
#include <unordered_map>

namespace pg_ai::utils {

static std::unordered_map<int, std::pair<std::string, std::string>> error_reasons = {
    {400 , {"Bad Request", "The request was invalid. Check the request format and parameters."}},
    {401 , {"Unauthorised", "Authentication failed. Please check your API key or credentials."}},
    {402 , {"Payment Required", "Quota or billing issue. Please check your subscription or usage limits."}},
    {403 , {"Forbidden", "You do not have permission to access this resource."}},
    {404 , {"Not Found", "The requested resource could not be found. Check the endpoint URL."}},
    {405 , {"Method Not Allowed", "The HTTP method used is not allowed for this endpoint."}},
    {406 , {"Not Acceptable", "The requested resource cannot generate a response acceptable to your client."}},
    {408 , {"Request Timed Out", "The request timed out. Try again later or check your network connection."}},
    {429 , {"Too Many Requests", "Rate limit exceeded. Slow down your requests or wait before retrying."}},
    {500 , {"Internal Server Error", "The server encountered an error. Please try again later."}},
    {502 , {"Bad Gateway", "The service is temporarily unavailable. Please try again later."}},
    {503 , {"Service Slow or Unavailable", "The server is currently overloaded or down. Retry after some time."}},
    {504 , {"Gateway Timeout", "The server did not respond in time. Check your network or try again later."}},
    {511 , {"Network Authentication Required", "Network authentication required. Check your network login credentials."}},
    {599 , {"Network Connection Timeout Error", "Network connection timed out. Please check your internet connection."}}
};

std::pair<bool, std::string> read_file(const std::string& filepath);

std::string read_file_or_throw(const std::string& filepath);

std::string formatAPIError(const std::string& provider , int status_code , const std::string& error_body);

}  // namespace pg_ai::utils