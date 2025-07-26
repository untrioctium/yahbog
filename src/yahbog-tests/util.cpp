#include <string>
#include <print>
#include <fstream>
#include <curl/curl.h>
#include <filesystem>

void check_curl_init() {
    struct init_helper {
        init_helper() {
            curl_global_init(CURL_GLOBAL_ALL);
        }
        ~init_helper() {
            curl_global_cleanup();
        }
    };
    static init_helper helper;
}

void clear_console_line() {
    std::print("\r\x1b[K");
}

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    file->write(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

bool download_to_path(std::string_view url, std::string_view path) { 
    check_curl_init();

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::println("Failed to initialize curl");
        return false;
    }

    std::ofstream file(std::string(path), std::ios::binary);
    if (!file) {
        std::println("Failed to open file: {}", path);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.data());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::println("Failed to download file at {}: {}", url, curl_easy_strerror(res));
        file.close();
        std::filesystem::remove(path);
        return false;
    }

    curl_easy_cleanup(curl);
    return true;
}