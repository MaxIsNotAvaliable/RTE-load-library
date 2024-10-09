#pragma once
#include <base.h>
#include <d3d11.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>
#include <anime/animation.h>
#include "../dnd.h"

class CRenderApp
{
private:
	bool running = true;
	void RenderBody(const HWND& window);
	void RenderEffects();
	void SetupStyle();
	void InitializeImGui();

public:
	Animation show_animation = Animation(0.25f);
	POINTS position = {};
	ImVec2 size;
	float margin;
	bool Initialize(const HWND& window, const WNDCLASSEXA& wc, const int& cmd_show);
	bool Running() { return this->running; }
	static inline DropTarget* pDropTarget = new DropTarget();
};