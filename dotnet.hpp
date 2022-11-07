#pragma once

#include "global.hpp"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

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

	class xsfd_dn_module
	{
	public:
		xsfd_dn_module(const char * path);
		~xsfd_dn_module();

		operator bool() const noexcept;
		auto get_raw() -> void *;
		auto get_size() -> std::size_t;
	private:
	};

	auto initialize() -> bool;
	auto uninitialize() -> bool;
	auto destroy() -> bool;

	auto host_running() -> bool;
	auto host_start() -> bool;
	auto host_end() -> bool;

	auto host_load_library(const char * lib_path) -> std::optional<xsfd_dn_module>;

	auto dump() -> std::optional<std::vector<domain_info_t>>;
}