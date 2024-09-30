#pragma once
#include <thread>

static const unsigned NUM_THREADS = std::thread::hardware_concurrency();

static const char* DB_URL = std::getenv("DB_URL");
