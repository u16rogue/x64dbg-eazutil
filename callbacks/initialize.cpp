#include "callbacks.hpp"
#include <string_view>
#include <filesystem>
#include <string>
#include <memory>
#include <future>

#include "../dotnet.hpp"
#include "../eazlib.hpp"

#include "../msdnlib.hpp"
#include <TlHelp32.h>

#include "../global.hpp"
#include "../xsfd_utils.hpp"
#include "../menu.hpp"

auto dump_test(kita::events::on_render * e) -> void
{
	static std::vector<dotnet::domain_info_t> v_domains;

	ImGui::Text("Domains:");
	for (const auto & domains : v_domains)
	{
		if (!ImGui::TreeNode(domains.name.c_str()))
			continue;

		ImGui::Text("Assemblies:");
		for (const auto & assemblies : domains.assemblies)
		{
			if (!ImGui::TreeNode(assemblies.name.c_str()))
				continue;

			ImGui::Text("Modules:");
			for (const auto & modules : assemblies.modules)
			{
				if (!ImGui::TreeNode(modules.name.c_str()))
					continue;

				ImGui::Text("Type definitions:");
				for (const auto & td : modules.typedefs)
				{
					if (!ImGui::TreeNode(td.name.c_str()))
						continue;

					ImGui::BeginChild("Methods:");
					for (const auto & mth : td.methods)
					{
						ImGui::Text("%s - md: 0x%x - RVA: 0x%lx - IL: 0x%p - Native:", mth.name.c_str(), mth.md_methoddef, mth.rva, mth.il_address);
						ImGui::SameLine();
						ImGui::Text("0x%p", mth.native_address);
						if (mth.native_address)
						{
							if (ImGui::IsItemClicked())
								GuiDisasmAt((duint)mth.native_address, 0);
							if (ImGui::IsItemHovered())
								ImGui::SetTooltip("Go to address in disassembler");
						}
					}
					ImGui::EndChild();

					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}

	static std::future<void> async_dump;

	if (ImGui::Button("Clear"))
	{
		async_dump = std::future<void> {};
		v_domains.clear();
	}

	ImGui::SameLine();

	if (!async_dump.valid() && ImGui::Button("Dump"))
	{
		xsfd::log("!Dumping...\n");
		v_domains.clear();
		async_dump = std::async([&]
		{
			v_domains = *dotnet::dump();
			xsfd::log("!Dump completed.\n");
		});
	}
}


auto callbacks::initialize() -> void
{
	dotnet::uninitialize();
	if (dotnet::initialize())
	{
		if (!dotnet::host_running())
		{
			dotnet::host_start();
			if (!dotnet::host_running())
			{
				xsfd::log("!ERROR: Failed to start host.\n");
				return;
			}
		}

		if (!eaz::eazlib_module)
		{
			auto lib = dotnet::host_load_library("eazlib.dll");
			if (!lib)
			{
				xsfd::log("!ERROR: Failed to load eazlib");
				return;
			}
			eaz::eazlib_module = *lib;

			if (!eaz::decode_fn)
			{
				// Find decode method
				eaz::eazlib_module.enumerate_types([](dotnet::wrap_Type t) -> bool {
					if (eaz::decode_fn)
						return false;

					if (t.get_name() != L"SymbolDecoder")
						return true;

					t.enumerate_methods([](dotnet::wrap_MethodInfo m) -> bool {
						if (m.get_name() != L"Decode")
							return true;
						eaz::decode_fn = m;
						return false;
					});

					return true;
				});

				if (!eaz::decode_fn)
				{
					xsfd::log("!ERROR: SymbolDecoder::Decode could not be found.\n");
					return;
				}
			}
		}

		global::initialized = true;
		_plugin_menuentrysetname(global::pluginHandle, menuid_initalize, "Re-Initialize");
		menu::add_render(dump_test, "dump test");
		menu::show();
	}
}
