#ifndef INVICULUM_PLATFORM_PLATFORM_HPP
#define INVICULUM_PLATFORM_PLATFORM_HPP

#include <string>

/**
 * This is a header file, please see source file in src/main instead
 */
namespace platform {
    namespace files {
        extern const char FILE_SEPARATOR;
        std::string get_resource_folder();
        void create_folder(const std::string& folder);
    }
}

#endif//INVICULUM_PLATFORM_PLATFORM_HPP