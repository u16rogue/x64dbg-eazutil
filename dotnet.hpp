#pragma once

#include "global.hpp"
#include <string>
#include <vector>
#include <optional>

namespace dotnet
{
	struct methods_info_t
	{
		mdMethodDef md_methoddef;
		std::string name;
		PCCOR_SIGNATURE sig;
		ULONG rva;

		void * il_address;
		void * native_address;
	};

	struct typedef_info_t
	{
		std::string name;
		std::vector<methods_info_t> methods;
	};

	struct modules_info_t
	{
		std::string name;
		std::vector<typedef_info_t> typedefs;
	};

	struct assembly_info_t
	{
		std::string name;
		std::vector<modules_info_t> modules;
	};

	struct domain_info_t
	{
		std::string name;
		std::vector<assembly_info_t> assemblies;
	};

	auto initialize() -> bool;
	auto uninitialize() -> bool;
	auto destroy() -> bool;

	auto host_running() -> bool;
	auto host_start() -> bool;
	auto host_end() -> bool;

	auto host_load_library(const char * lib_path) -> bool;

	auto dump() -> std::optional<std::vector<domain_info_t>>;
}