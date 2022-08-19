#include "menu.hpp"
#include <vector>
#include <memory>
#include <thread>
#include "xsfd_utils.hpp"

static std::unique_ptr<kita::kita_instance> instance;
static std::vector<std::pair<kita::events::details::on_render_cb_t, const char *>> render_cbs;
static std::unique_ptr<std::thread> render_thread;

static bool visible = false;
static bool render_proc_running = false;

static auto render_dispatch(kita::events::on_render * e) -> void
{
	if (!ImGui::BeginTabBar("#tabitems"))
		return;

	for (auto & [f, tab_title] : render_cbs)
	{
		if(!ImGui::BeginTabItem(tab_title))
			continue;
		f(e);
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
}

static auto render_proc() -> void
{
	if (!instance)
	{
		instance = std::make_unique<kita::kita_instance>("x64dbg-eazutil", 400, 600);
		if (!instance)
		{
			xsfd::log("!ERROR: Failed to initialize menu.\n");
		}

		instance->callbacks(
			render_dispatch,
			[](kita::events::on_close * e) {
				e->cancel = true;
				visible = false;
			},
			[](kita::events::on_pre_render * e) {
				if (!visible)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
					e->render = false;
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}

				static bool last_visibility = visible;
				if (last_visibility != visible)
				{
					if (visible)
						instance->show();
					else
						instance->hide();
					last_visibility = visible;
				}
			}
		);
	}

	instance->show();
	visible = true;
	render_proc_running = true;
	instance->run();
	render_proc_running = false;
}

auto menu::valid() -> bool
{
	return instance && *instance;
}

auto menu::toggle() -> bool
{
	return visible ? menu::hide() : menu::show();
}

auto menu::show() -> bool
{
	if (!render_thread)
	{
		render_thread = std::make_unique<std::thread>(render_proc);
		if (!render_thread)
			return false;
	}

	visible = true;
	return true;
}

auto menu::hide() -> bool
{
	visible = false;
	return true;
}

auto menu::dispose() -> bool
{
	render_cbs.clear();	

	if (instance)
	{
		instance->close();
		xsfd::log("!Waiting for render proc to end.\n");
		while (render_proc_running) std::this_thread::sleep_for(std::chrono::milliseconds(50));
		instance = nullptr;
	}

	if (render_thread)
	{
		render_thread->join();
		render_thread = nullptr;
	}
	return true;
}

auto menu::add_render(kita::events::details::on_render_cb_t f, const char * title) -> bool
{
	for (auto & [fn, t] : render_cbs)
	{
		if (fn == f)
		{
			t = title;
			return true;
		}
	}

	render_cbs.emplace_back(f, title);
	return true;
}
