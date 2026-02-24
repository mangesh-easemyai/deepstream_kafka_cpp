#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H
#include <iostream>
#include <vector>
bool ensure_directory(const std::string &dir);
std::vector<std::string> read_class_label(const std::string &filename);
void generate_ts_rfc3339_(char *buf, size_t buf_size);
std::string get_absolute_file_path(const std::string &cfg_file_path, const std::string &file_path);

void meta_free_func(gpointer data, gpointer user_data);
gpointer meta_copy_func(gpointer data, gpointer user_data);
 

#endif