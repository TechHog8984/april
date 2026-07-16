#pragma once

#include "classreader.hpp"
#include <optional>

int generateLuau(Class& _class, std::string& output, std::optional<std::string_view> jar_parent_path = std::nullopt);
