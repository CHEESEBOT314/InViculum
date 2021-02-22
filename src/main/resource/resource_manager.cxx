#include "resource/resource_manager.hxx"

#include <memory>
#include <fstream>

/**
 * resource::resource_manager - Resource Manager namespace is used to read binary files for pipeline loading
 */
namespace resource::resource_manager {
    namespace {
        struct info {
            std::string folder;
            char separator = 0;
        };
        std::unique_ptr<info> info_p;
    }
    void init(const std::string& folder, char separator) {
        info_p = std::make_unique<info>();
        info_p->folder = folder;
        info_p->separator = separator;
    }
    /**
     * read_binary_file - Read Binary File function reads all bytes from a binary file
     * @param file_name - file name to read
     * @param folders - parent folders
     * @return - vector of bytes that have been read
     */
    std::vector<uint8_t> read_binary_file(const std::string& file_name, const std::vector<std::string>& folders) {
        std::string full_path = info_p->folder;
        for (const std::string& d : folders) {
            full_path = full_path.append(d).append(&info_p->separator, 1);
        }
        full_path = full_path.append(file_name);
        std::ifstream file(full_path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            return {};
        }
        size_t file_size = (size_t) file.tellg();
        std::vector<uint8_t> buffer(file_size);
        file.seekg(0);
        file.read((char*)buffer.data(), file_size);
        return buffer;
    }
}