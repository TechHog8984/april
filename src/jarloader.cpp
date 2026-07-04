#include "jarloader.hpp"

#include <iostream>

#include <unordered_map>
#include <zip.h>

std::unique_ptr<std::unordered_map<std::string, std::string>> readJar(const char* filename) {
    int err = 0;
    zip* archive = zip_open(filename, 0, &err);

    if (!archive) {
        struct zip_error error;
        zip_error_init_with_code(&error, err);
        std::cerr << "Failed to open the zip file: " << zip_error_strerror(&error) << std::endl;
        zip_error_fini(&error);
        return nullptr;
    }

    int entry_count = zip_get_num_entries(archive, 0);

    auto filemap = std::make_unique<std::unordered_map<std::string, std::string>>();

    std::string content;

    for (int i = 0; i < entry_count; ++i) {
        struct zip_stat fileInfo;
        zip_stat_init(&fileInfo);
    
        if (zip_stat_index(archive, i, 0, &fileInfo) == 0) {
            std::string filename = fileInfo.name;
            auto extensionpos = filename.rfind(".class");
            if (extensionpos == std::string::npos)
                continue;
            if (extensionpos + 6 != filename.size())
                continue;

            std::string classname = std::string(filename, 0, extensionpos);

            zip_file* file = zip_fopen_index(archive, i, 0);

            char buffer[1024];
            zip_int64_t read;
            while ((read = zip_fread(file, buffer, sizeof(buffer))) > 0)
                content.append(buffer, read);
            zip_fclose(file);

            filemap->emplace(classname, content);
            content.clear();
        }
    }

    zip_close(archive);

    return filemap;
}
