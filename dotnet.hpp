#pragma once

#include "global.hpp"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include "xsfd_utils.hpp"

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

	class wrap_base
	{
	public:
		wrap_base();
		wrap_base(void * i);
		auto get_name() -> std::wstring;

		operator bool() const noexcept;
	protected:
		void * instance;
	};

	class wrap_MethodInfo : public wrap_base
	{
	public:
	};

	class wrap_Type : public wrap_base
	{
	public:
		// Return true to continue enumeration and false to end
		template <typename T>
		auto enumerate_methods(T callback) -> void
		{
			SAFEARRAY * methods = nullptr;
			if (((HRESULT(__stdcall***)(int, SAFEARRAY **))instance)[0][0x0b8](2 | 32 | 16 | 4 | 8, &methods) != S_OK)
				return;

			XSFD_DEFER {
				SafeArrayDestroy(methods);
			};

			wrap_MethodInfo * data = (decltype(data))methods->pvData;
			for (int i = 0; i < methods->rgsabound->cElements; ++i)
			{
				if (!callback(data[i]))
					return;
			}
		}
	};

	class wrap_Assembly : public wrap_base
	{
	public:
		wrap_Assembly() = default;
		wrap_Assembly(void * assembly);
		~wrap_Assembly();

		// Return true to continue enumeration and false to end
		template <typename T>
		auto enumerate_types(T callback) -> void
		{
			SAFEARRAY * types = nullptr;
			if (((HRESULT(__stdcall***)(SAFEARRAY **))instance)[0][0x50](&types) != S_OK)
				return;

			XSFD_DEFER {
				SafeArrayDestroy(types);
			};

			wrap_Type * data = (decltype(data))types->pvData;
			for (int i = 0; i < types->rgsabound->cElements; ++i)
			{
				if (!callback(data[i]))
					return;
			}
		}
	};

	auto initialize() -> bool;
	auto uninitialize() -> bool;
	auto destroy() -> bool;

	auto host_running() -> bool;
	auto host_start() -> bool;
	auto host_end() -> bool;

	auto host_load_library(const char * lib_path) -> std::optional<wrap_Assembly>;

	auto dump() -> std::optional<std::vector<domain_info_t>>;
}