#include "callbacks.hpp"
#include <string_view>
#include <filesystem>
#include <string>
#include <memory>

#include "../dotnet.hpp"

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
						ImGui::Text("%s - RVA: 0x%lu - IL: 0x%p - Native:", mth.name.c_str(), mth.rva, mth.il_address);
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

	if (ImGui::Button("Clear"))
		v_domains.clear();

	ImGui::SameLine();

	if (!ImGui::Button("Dump"))
		return;

	v_domains.clear();

	xsfd::log("!Dumping...\n");
	v_domains = *dotnet::dump();
	xsfd::log("!Dump completed.\n");
}


auto callbacks::initialize() -> void
{
	dotnet::uninitialize();
	if (dotnet::initialize())
	{
		global::initialized = true;
		_plugin_menuentrysetname(global::pluginHandle, menuid_initalize, "Re-Initialize");
		menu::add_render(dump_test, "dump test");
		menu::show();
	}
}

/*
* RuntimeHelpers.PrepareMethod() - v4.0.30319
* 89 8D ? ? ? ? 33 DB 85 C9 75 22 68 ? ? ? ? 53 6A 04 - \x89\x8D\x00\x00\x00\x00\x33\xDB\x85\xC9\x75\x22\x68\x00\x00\x00\x00\x53\x6A\x04 xx????xxxxxxx????xxx @ clr.dll
* 68 98 00 00 00                                  push    98h ; '˜'
* B8 94 3F 6E 10                                  mov     eax, offset sub_106E3F94
* E8 96 CE B6 FF                                  call    sub_10010C95
* 8B F2                                           mov     esi, edx
* 89 B5 E0 FF FF FF                               mov     [ebp-20h], esi
* B8 F0 3D 4A 10                                  mov     eax, offset RuntimeHelpers_PrepareMethod
* 89 85 D0 FF FF FF                               mov     [ebp-30h], eax
* 89 8D CC FF FF FF                               mov     [ebp-34h], ecx                            <<<
* 33 DB                                           xor     ebx, ebx
* 85 C9                                           test    ecx, ecx
* 75 22                                           jnz     short loc_104A3E40
*/

