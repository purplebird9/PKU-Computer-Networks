#pragma once

void open_conn_reply(int sock);
void list_reply(int client_sock);
void change_dir_reply(int sock);
void get_reply(int sock);   
void put_reply(int sock);
void sha_reply(int sock);
void quit_reply(int sock);

