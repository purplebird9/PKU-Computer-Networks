#pragma once

#include "protocol.h"

bool open_conn_request();
void list_request(struct Header& hc);
void change_dir_request();
void get_request(struct Header& hc, const char* filename);
void put_request();
void sha_request();
void quit_request();
