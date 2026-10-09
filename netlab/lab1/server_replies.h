#pragma once

#include <filesystem>

void open_conn_reply(int sock);
void list_reply(int client_sock, const std::filesystem::path& cwd);
void change_dir_reply(int sock, std::filesystem::path& cwd);
void get_reply(int sock, const std::filesystem::path& cwd);
void put_reply(int sock, const std::filesystem::path& cwd);
void sha_reply(int sock, const std::filesystem::path& cwd);
void quit_reply(int sock);
