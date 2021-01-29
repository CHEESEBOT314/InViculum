#include "platform/platform.hxx"

#include "windows.h"

namespace platform::files {
    const char FILE_SEPARATOR = '\\';

    std::string get_resource_folder() {
        HMODULE hModule = GetModuleHandle(nullptr);
        char path[MAX_PATH];
        GetModuleFileName(nullptr, path, sizeof(path));
        std::string pathStr = std::string(path);
        return pathStr.substr(0, pathStr.find_last_of('\\') + 1).append("resources\\");
    }
    void create_folder(const std::string &folder) {
    }
}