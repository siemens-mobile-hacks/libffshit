#ifndef FF_SYSTEM_H
#define FF_SYSTEM_H

#include <string>
#include <filesystem>

namespace System {

bool is_file_exists(const std::string &path);
bool is_directory_exists(const std::string &path);
bool remove_directory(const std::filesystem::path path);
bool create_directory(const std::string &path, std::filesystem::perms perms);

void to_lower(std::string &str);
void to_upper(std::string &str);

};

#endif
