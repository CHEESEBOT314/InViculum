#ifndef INVICULUM_RESOURCE_RESOURCEMANAGER_HPP
#define INVICULUM_RESOURCE_RESOURCEMANAGER_HPP

#include <string>
#include <vector>

/**
 * This is a header file, please see source file in src/main instead
 */
namespace resource::resource_manager {
    void init(const std::string& folder, char separator);
    std::vector<uint8_t> read_binary_file(const std::string& file_name, const std::vector<std::string>& folders);
}

#endif//INVICULUM_RESOURCEMANAGER_HPP
