#ifndef LIBFFSHIT_SYSTEM_H
#define LIBFFSHIT_SYSTEM_H

#include <string>
#include <filesystem>
#include <system_error>

namespace System {

bool is_file_exists(const std::filesystem::path path);
bool is_directory_exists(const std::filesystem::path path);
bool remove_directory(const std::filesystem::path path, std::error_code &error_code);
bool create_directory(const std::filesystem::path path, std::filesystem::perms perms, std::error_code &error_code);

void to_lower(std::string &str);
void to_upper(std::string &str);

};

#endif
