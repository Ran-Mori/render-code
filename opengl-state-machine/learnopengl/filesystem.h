#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <cstdlib>
#include <string>

#include "root_directory.h"

class FileSystem
{
public:
    static std::string getPath(const std::string& path)
    {
        const char* environmentRoot = std::getenv("LOGL_ROOT_PATH");
        const char* root = environmentRoot != nullptr ? environmentRoot : logl_root;
        return root != nullptr && root[0] != '\0'
            ? std::string(root) + "/" + path
            : path;
    }
};

#endif
