#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <mongocxx/instance.hpp>
#include "../doctest/doctest.h"

static mongocxx::instance m_instance{};