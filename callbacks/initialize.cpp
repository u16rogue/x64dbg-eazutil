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

auto callbacks::initialize() -> void
{
	dotnet::uninitialize();
	if (dotnet::initialize())
	{
		global::initialized = true;
		_plugin_menuentrysetname(global::pluginHandle, menuid_initalize, "Re-Initialize");
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

struct methods_info_t
{
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

#if 0
auto dump_test(kita::events::on_render * e) -> void
{
	static std::vector<domain_info_t> v_domains;

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
						ImGui::Text("%s - RVA: 0x%p - IL: 0x%p - Native:", mth.name.c_str(), mth.rva, mth.il_address);
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
	// Domains
	ICorDebugAppDomainEnum * appdomain = nullptr;
	if (dotnet::cor_debug_process->EnumerateAppDomains(&appdomain) != S_OK)
		return;

	XSFD_DEFER { appdomain->Release(); };

	ULONG domain_count = 0;
	appdomain->GetCount(&domain_count);
	xsfd::log("!Domain count: %d\n", domain_count);
	auto domains = std::make_unique<ICorDebugAppDomain * []>(domain_count);
	if (!domains)
	{
		xsfd::log("!Domain container alloc failed.\n");
		return;
	}

	XSFD_DEFER {
		for (int i = 0; i < domain_count; ++i)
			domains[i]->Release();
	};

	ULONG domain_count_fetched = 0;
	if (appdomain->Next(domain_count, domains.get(), &domain_count_fetched) != S_OK || domain_count != domain_count_fetched)
	{
		xsfd::log("!Domain fetch failed (%d:%d)\n", domain_count, domain_count_fetched);
		return;
	}

	for (int i_domain = 0; i_domain < domain_count; ++i_domain)
	{
		ICorDebugAppDomain * domain = domains[i_domain];
		WCHAR nbuff[128] = {};
		ULONG32 nlen = 0;

		if (domain->GetName(127, &nlen, nbuff) != S_OK)
		{
			xsfd::log("!ICorDebugAppDomain::GetName failed.\n");
			continue;
		}

		auto & v_domain = v_domains.emplace_back(domain_info_t { xsfd::wc2u8(nbuff), {} });

		// Assembly
		ICorDebugAssemblyEnum * assembly_enum = nullptr;
		if (domain->EnumerateAssemblies(&assembly_enum) != S_OK)
		{
			xsfd::log("!ICorDebugAppDomain::EnumerateAssemblies failed.\n");
			continue;
		}
		XSFD_DEFER { assembly_enum->Release(); };

		ULONG assembly_fetched = 0;
		ICorDebugAssembly * assembly = nullptr;
		while (assembly_enum->Next(1, &assembly, &assembly_fetched) == S_OK && assembly_fetched == 1)
		{
			XSFD_DEFER { assembly->Release(); };
			if (assembly->GetName(127, &nlen, nbuff) != S_OK)
			{
				xsfd::log("!ICorDebugAssembly::GetName failed.\n");
				continue;
			}

			auto s_assembly = xsfd::wc2u8(nbuff);
			auto & v_assembly = v_domain.assemblies.emplace_back(assembly_info_t { s_assembly.substr(s_assembly.find_last_of('\\') + 1), {} });

			// Modules
			ICorDebugModuleEnum * module_enum = nullptr;
			if (assembly->EnumerateModules(&module_enum) != S_OK)
			{
				xsfd::log("!ICorDebugAssembly::EnumerateModules failed.\n");
				continue;
			}
			XSFD_DEFER { module_enum->Release(); };

			ICorDebugModule * mod = nullptr;
			ULONG mod_fetched = 0;
			while (module_enum->Next(1, &mod, &mod_fetched) == S_OK && mod_fetched == 1)
			{
				XSFD_DEFER { mod->Release(); };
				CORDB_ADDRESS address = 0;
				if (mod->GetName(127, &nlen, nbuff) != S_OK || mod->GetBaseAddress(&address) != S_OK)
				{
					xsfd::log("!ICorDebugModule::GetName failed.\n");
					continue;
				}

				auto & v_modules = v_assembly.modules.emplace_back(modules_info_t { xsfd::wc2u8(nbuff), {} });

				// Meta data
				IMetaDataImport * metadata = nullptr;
				if (mod->GetMetaDataInterface(IID_IMetaDataImport, reinterpret_cast<IUnknown **>(&metadata)) != S_OK)
				{
					xsfd::log("!ICorDebugModule::GetMetaDataInterface failed.\n");
					continue;
				}
				XSFD_DEFER { metadata->Release(); };

				// Type definitions
				HCORENUM td_hce = 0;
				ULONG td_count = 0;

				metadata->EnumTypeDefs(&td_hce, nullptr, 0, nullptr);
				if (metadata->CountEnum(td_hce, &td_count) != S_OK)
				{
					xsfd::log("!IMetaDataImport::CountEnum failed.\n");
					continue;
				}

				auto typedefs = std::make_unique<mdTypeDef[]>(td_count);
				if (!typedefs || metadata->EnumTypeDefs(&td_hce, typedefs.get(), td_count, &td_count) != S_OK)
				{
					xsfd::log("!IMetaDataImport::EnumTypeDefs failed.\n");
					continue;
				}

				for (int i_tds = 0; i_tds < td_count; ++i_tds)
				{
					// Type definition props
					ULONG td_pch = 0;
					DWORD td_flags = 0;
					mdToken ext;
					if (metadata->GetTypeDefProps(typedefs[i_tds], nbuff, sizeof(nbuff) / sizeof(nbuff[0]), &td_pch, &td_flags, &ext) != S_OK)
					{
						xsfd::log("!IMetaDataImport::GetTypeDefProps failed.\n");
						continue;
					}

					auto & v_typedefs = v_modules.typedefs.emplace_back(typedef_info_t { xsfd::wc2u8(nbuff) });

					// Methods
					HCORENUM mth_hce = 0;
					ULONG mth_count = 0;
					metadata->EnumMethods(&mth_hce, typedefs[i_tds], nullptr, 0, nullptr);
					if (metadata->CountEnum(mth_hce, &mth_count) != S_OK)
					{
						xsfd::log("!IMetaDataImport::CountEnum failed.\n");
						continue;
					}

					auto methods = std::make_unique<mdMethodDef[]>(mth_count);
					if (!methods || metadata->EnumMethods(&mth_hce, typedefs[i_tds], methods.get(), mth_count, &mth_count) != S_OK)
					{
						//xsfd::log("!IMetaDataImport::EnumMethods failed.\n");
						continue;
					}

					for (int i_mth = 0; i_mth < mth_count; ++i_mth)
					{
						mdTypeDef mdtd = 0;
						ULONG pchmth = 0;
						DWORD attr = 0;
						PCCOR_SIGNATURE sig = 0;
						ULONG sigblob = 0;
						ULONG rva = 0;
						DWORD flags = 0;
						if (metadata->GetMethodProps(methods[i_mth], &mdtd, nbuff, 128, &pchmth, &attr, &sig, &sigblob, &rva, &flags) != S_OK)
						{
							//xsfd::log("!IMetaDataImport::GetMethodProps failed.\n");
							continue;
						}

						ICorDebugFunction * mth_fn = nullptr;
						if (mod->GetFunctionFromToken(methods[i_mth], &mth_fn) != S_OK)
						{
							//xsfd::log("!IMetaDataImport::GetFunctionFromToken failed.\n");
							continue;
						}
						XSFD_DEFER { mth_fn->Release(); };

						ICorDebugCode * mth_ilcode = nullptr;
						if (mth_fn->GetILCode(&mth_ilcode) != S_OK)
						{
							//xsfd::log("!ICorDebugFunction::GetILCode failed.\n");
							continue;
						}
						XSFD_DEFER { mth_ilcode->Release(); };

						CORDB_ADDRESS il_address = 0;
						if (mth_ilcode->GetAddress(&il_address) != S_OK)
						{
							//xsfd::log("!ICorDebugCode::GetAddress failed.\n");
							continue;
						}
						
						ICorDebugCode * mth_natcode = nullptr;
						if (mth_fn->GetNativeCode(&mth_natcode) != S_OK)
						{
							//xsfd::log("!ICorDebugFunction::GetNativeCode failed.\n");
						}
						XSFD_DEFER { if (mth_natcode) mth_natcode->Release(); };

						CORDB_ADDRESS native_address = 0;
						if (mth_natcode && mth_natcode->GetAddress(&native_address) != S_OK)
						{
							//xsfd::log("!ICorDebugCode::GetAddress failed.\n");
						}

						methods_info_t mi = {};
						mi.name = xsfd::wc2u8(nbuff);
						mi.rva  = rva;
						mi.sig  = sig;
						mi.il_address = (void*)il_address;
						mi.native_address = (void*)native_address;
						v_typedefs.methods.emplace_back(mi);
					}

				}
			}
		}
	}

	xsfd::log("!Dump completed.\n");
}
#endif
