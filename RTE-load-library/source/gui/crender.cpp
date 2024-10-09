#include "crender.h"
#include <render/drawing.h>
#include "../injector/injector.h"

#include "../dnd.h"

#include <encryption/encstring.h>

#include <fonts/fonts.hpp>
#include <files_helper/files.h>
#include "../injector/GetProcAddress.h"
#include "../injector/ProcessManager.h"

#include <windows.h>
#include <commdlg.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>

static int currentTabId = 0;
static int nextTabId = currentTabId;

static Animation contentAnim = Animation(Animation::forward, 0.25f);

void CRenderApp::RenderBody(const HWND& window)
{
	ImGuiStyle* style = &ImGui::GetStyle();
	ImGuiIO* io = &ImGui::GetIO();

	this->show_animation.Proceed();

	contentAnim.Proceed();
	if (contentAnim.StartAfter(contentAnim, Animation::back, Animation::forward))
		currentTabId = nextTabId;

	ImVec2 windowPos = ImVec2(this->margin, this->margin);
	ImVec2 windowSize = this->size - ImVec2(this->margin, this->margin) * 2;
	float animationValue = this->show_animation.GetValueSin();
	float alpha = Animation::lerp(0, 0.98f, animationValue);

	ImGui::SetNextWindowSize(windowSize);
	ImGui::SetNextWindowPos(windowPos);
	ImGui::SetNextWindowBgAlpha(alpha);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, animationValue);

	const float fontSize = ImGui::GetFontSize();

	if (ImGui::Begin(ENCSTR("RTE load library"), &this->running, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushFont(fonts::bold_28);

		//ImGui::Text(ENCSTR("RTE-load-library"));
		ImGui::BeginDisabled();
		ImGui::Button(ENCSTR("RTE-load-library"));
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::PopFont();


		static Animation animation_dnd = Animation(0.123f);
		animation_dnd.Proceed();
		animation_dnd.Start(Animation::ToAnimationState(pDropTarget->dragStatus == DragStatus_hover));
		float dnd_value = animation_dnd.GetValueSin();

		if (dnd_value > 0)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, animationValue * dnd_value);
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			{
				constexpr ImVec2 dndsize = ImVec2(240, 88);
				ImVec2 deltaPos = (windowSize - dndsize);
				deltaPos = windowPos + ImVec2(deltaPos.x / 2, deltaPos.y / 2);
				//deltaPos = ImGui::GetMousePos() - ImVec2(deltaPos.x / 2, deltaPos.y / 2);
				//deltaPos = ImGui::GetMousePos();
				drawList->AddRectFilled(deltaPos, deltaPos + dndsize, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3f)), style->WindowRounding);

				ImGui::PushFont(fonts::bold_28);
				drawList->AddText(deltaPos, ImGui::GetColorU32(colors::white), "Drag and drop");
				ImGui::PopFont();
				ImGui::PushFont(fonts::bold_20);
				drawList->AddText(deltaPos + ImVec2(0, ImGui::GetFontSize() + style->FramePadding.y), ImGui::GetColorU32(colors::white), "any .dll file here");
				auto fileList = pDropTarget->GetFiles();
				for (size_t i = 0; i < fileList.size(); i++)
				{
					drawList->AddText(deltaPos + ImVec2(0, ImGui::GetFontSize() * (i + 2) + style->FramePadding.y), ImGui::GetColorU32(colors::white), fileList[i].filename().string().c_str());
				}
				ImGui::PopFont();
			}

			ImGui::PopStyleVar();
		}

		const ImVec2 titleBtnSize = ImVec2(30, 30);
		const ImVec2 injectBtnSz = ImVec2(170, 48);

		const float titleBtnWidth = titleBtnSize.x;
		const float titleBtnPd = titleBtnWidth / 4.5f;

		ImGui::SetCursorPosX(10);


		ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - titleBtnWidth * 2 - style->ItemSpacing.x);
		ImVec2 HidePos = tools::LocalToGlobalPos(ImGui::GetCursorPos());
		ImVec2 CrossPos = HidePos + ImVec2(titleBtnWidth + style->ItemSpacing.x, 0);

		if (items::ShadowButton(ENCSTR("##minimizeBtn"), titleBtnSize))
		{
			this->show_animation.Start(Animation::back);
		}

		ImGui::SameLine();
		if (items::ShadowButton(ENCSTR("##closeBtn"), titleBtnSize))
		{
			this->show_animation.Start(Animation::back);
			this->running = false;
		}

		draw::Underline(ImVec2(HidePos.x + titleBtnWidth / 2, HidePos.y + titleBtnWidth / 2), titleBtnPd, colors::text);
		draw::Cross(ImVec2(CrossPos.x + titleBtnWidth / 2, CrossPos.y + titleBtnWidth / 2), titleBtnPd * this->show_animation.GetValueSin(), colors::text);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));

		ImGui::BeginChild(ENCSTR("TitleChild"), ImVec2(0, tools::CalcChildSize().y - 140), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
		{
			/*ImGui::PushFont(fonts::bold_28);*/

			float fontSize = ImGui::GetFontSize();
			ImVec2 wndSize = ImGui::GetWindowSize();
			//float posY = (wndSize.y - fontSize) / 2;
			//ImGui::SetCursorPosY(posY);
			//ImGui::Text(ENCSTR("RTE-load-library"));

			//ImGui::PopFont();
			
			ImGui::BeginChild("ProcessSelectChild", ImVec2(ImGui::GetContentRegionAvail().x / 2, 0));
			{
				ImGui::Text("Select process");
				static char buf[0x100];
				ImGui::InputText("##searchByName", buf, sizeof(buf));
				ImGui::BeginChild("ProcessesChild");
				{
					static int counter = 0;
					if (++counter % 100 == 0)
						CProcessManager::UpdateProcessList();
					auto procList = CProcessManager::GetProcessList(buf);
					static DWORD targetPid = 0;
					for (size_t i = 0; i < procList.size(); i++)
					{
						EString estr = "[" + EString((int)procList[i].pid) + "] " + procList[i].processName;
						
						if (ImGui::RadioButton(estr.c_str(), procList[i].pid == targetPid))
						{
							targetPid = procList[i].pid;
						}
					}
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("DllFileSelectChild");
			{
				ImGui::Text("Select dll file");
				static char buf[0x100];
				ImGui::InputText("##dllFilenamePath", buf, sizeof(buf));
				ImGui::SameLine();
				ImGui::Button("Open");
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.03f, 0.03f, 0.03f, 0.4f));
		ImGui::BeginChild(ENCSTR("footerChild"), ImVec2(), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
		{
			ImGui::PushStyleColor(ImGuiCol_Button, colors::active);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::active * colors::darker * colors::darker);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::active * colors::darker);

			auto GetMinecraftPID = []()
				{
					HWND hWnd = FindWindowA(ENCSTR("LWJGL"), nullptr);
					if (!hWnd)
						hWnd = FindWindowA(ENCSTR("GLFW30"), nullptr);

					DWORD PID = 0;
					GetWindowThreadProcessId(hWnd, &PID);

					return PID;
				};

			ImGui::PushFont(fonts::bold_20);
			bool loadClientClick = ImGui::Button(ENCSTR("LOAD CLIENT"), ImVec2(186, 54));
			ImGui::PopFont();

			if (loadClientClick)
			{
				DWORD targetPID = GetMinecraftPID();
				HMODULE hMods[1024];
				DWORD cbNeeded;

				InjectQueueHandler::AddToQueue(targetPID, ENCSTR("sc_proj.dll"));
			}

			static bool waitingToClose = false;
			static int lastId = -1;
			if (lastId != InjectQueueHandler::GetLastId())
			{
				lastId = InjectQueueHandler::GetLastId();
				items::OpenNotify(ENCSTR("Client load"));
				waitingToClose = true;
			}
			if (waitingToClose)
			{
				int resultCode = 0;
				bool finished = InjectQueueHandler::GetResult(InjectQueueHandler::GetLastId(), &resultCode);

				EString text = finished ? (resultCode == 0 ?
					ENCSTR("Client loaded successfully!\n ") :
					EString(ENCSTR("Unable to load client.\nError code: ")) + CInjector::ReturnCodeToStr(resultCode)) : ENCSTR("Loading...\n ");

				auto lmFn = [&]()
					{
						bool pressed = false;
						if (ImGui::Button(ENCSTR("OK"), ImVec2(120, 0)) && finished)
						{
							ImGui::CloseCurrentPopup();
							pressed = true;
						}

						ImGui::SetItemDefaultFocus();
						ImGui::SameLine();

						if (ImGui::Button(ENCSTR("Close"), ImVec2(120, 0)) && finished)
						{
							ImGui::CloseCurrentPopup();
							pressed = true;
						}
						return false;
					};

				//if (items::ShowNotify("Client load", text.c_str()))
				if (items::ShowNotifyLm(ENCSTR("Client load"), text.c_str(), lmFn))
				{
					waitingToClose = false;
				}
			}





			ImGui::SameLine();

			ImVec2 timerChildSize = ImVec2(256, 50);
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - timerChildSize.x - style->WindowPadding.x, ImGui::GetCursorPosY() + 2));
			ImGui::BeginChild(ENCSTR("timerChild"), timerChildSize);
			{
				constexpr float clockRadius = 13;
				float parentPadding = (style->WindowPadding.x + style->FramePadding.x) / 2;
				ImVec2 clockPos = ImVec2(clockRadius + ImGui::GetCursorPosX() + parentPadding, ImGui::GetWindowHeight() / 2);
				float clockTime = 0.84f;

				static Animation clockAnim = Animation(Animation::forward, 30);
				clockAnim.Proceed();

				if (clockAnim.AnimationEnded())
				{
					clockAnim.Reverse();
					if (clockAnim.GetState() == clockAnim.forward)
					{
						clockAnim.SetDuration(3);
					}
					else
					{
						clockAnim.SetDuration(30);
					}
				}

				draw::Clock(clockPos, clockAnim.GetValueSin(), clockRadius, 3.5f);

				float fontSize = ImGui::GetFontSize();
				ImGui::SetCursorPos(ImVec2(clockPos.x + clockRadius + style->FramePadding.x, (timerChildSize.y - fontSize * 2) / 2));

				int days = 19;
				int hours = 14;
				int minutes = 51;
				ImGui::Text(ENCSTR("Time left: %i days\n%i hours %i minutes"), days, hours, minutes);
				ImGui::SameLine();

				ImGui::PushFont(fonts::bold_16);

				ImVec2 btnSize = tools::CalcItemSize(ENCSTR("BUY"));
				ImGui::SetCursorPos(ImVec2(timerChildSize.x - btnSize.x - parentPadding, (timerChildSize.y - btnSize.y) / 2));

				bool buyClientClick = ImGui::Button(ENCSTR("BUY"), btnSize);
				ImGui::PopFont();

				if (buyClientClick)
				{
					items::OpenNotify(ENCSTR("Buy subscription"));
				}

				items::ShowNotify(ENCSTR("Buy subscription"), ENCSTR("Buy what? noob"));

			}
			ImGui::EndChild();

			ImGui::PopStyleColor(3); // button colors

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (style->WindowPadding.y + style->FramePadding.y) / 2);

			ImGui::BeginChild("footerFooterPanel");
			{
				ImVec2 panelSize = ImGui::GetWindowSize();
				ImVec2 newBtnSize = ImVec2(0, panelSize.y);
				ImGui::PushStyleColor(ImGuiCol_Text, colors::text * ImVec4(1, 1, 1, 0.5f));

				ImGui::PushStyleColor(ImGuiCol_Button, colors::transparent);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::transparent);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::transparent);

				static std::vector<const char*> btns = {
					"Check for updates",
					"Official site",
					"Subscription",
					"Settings"
				};
				static int hoveredId = -1;
				bool isHovered = false;
				for (size_t i = 0; i < btns.size(); i++)
				{
					if (i) ImGui::SameLine();

					if (i == btns.size() - 1)
					{
						ImGui::SetCursorPosX(panelSize.x - tools::CalcItemSize(btns[i]).x);
					}

					bool clicked = false;
					if (i == hoveredId)
					{
						ImGui::PushStyleColor(ImGuiCol_Text, colors::text);
						clicked |= ImGui::Button(btns[i], newBtnSize);
						ImGui::PopStyleColor();
					}
					else
					{
						clicked |= ImGui::Button(btns[i], newBtnSize);
					}

					if (clicked)
					{
						items::OpenNotify(btns[i]);
					}

					if (ImGui::IsItemHovered())
					{
						isHovered = true;
						hoveredId = i;
					}
				}
				if (!isHovered) hoveredId = -1;


				ImGui::PopStyleColor(3);
				ImGui::PopStyleColor();

				items::ShowNotify("Check for updates", "You using newest version of client");
				items::ShowNotify("Official site", "Open www.google.com?");
				items::ShowNotify("Subscription", "Open www.google.com/subscriptions?");

				auto lmSettings = [&]()->bool
					{
						ImVec2 settingsSize = ImVec2(200, 150);
						ImGui::BeginChild("Settings", settingsSize);
						{
							bool loggedIn = true;

							ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::transparent);
							ImGui::BeginChild("innersettings");

							if (loggedIn)
							{
								float winWidth = ImGui::GetWindowWidth();

								float ts = 0;

								EString loginStr = "Login: ABOBA_USER";
								ImGui::SetCursorPosX((winWidth - ImGui::CalcTextSize(loginStr.c_str()).x) / 2);
								ImGui::Text(loginStr.c_str());

								EString passStr = "Password: *****";
								ImGui::SetCursorPosX((winWidth - ImGui::CalcTextSize(passStr.c_str()).x) / 2);
								ImGui::Text(passStr.c_str());

								EString sidStr = "SID: Valid";
								ImGui::SetCursorPosX((winWidth - ImGui::CalcTextSize(sidStr.c_str()).x) / 2);
								ImGui::Text(sidStr.c_str());

								EString logoutBtnStr = "Logout";
								ImGui::SetCursorPosX((winWidth - tools::CalcItemSize(logoutBtnStr.c_str()).x) / 2);
								ImGui::Button("Logout");
							}
							else
							{
								ImGui::Button("gay");
							}

							ImGui::EndChild();
							ImGui::PopStyleColor();
						}
						ImGui::EndChild();

						if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
						return false;
					};

				items::ShowNotifyLm("Settings", "Account parameters", lmSettings);

			}
			ImGui::EndChild();



		}
		ImGui::EndChild();
		ImGui::PopStyleColor();

		ImGui::PopStyleVar();

	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void CRenderApp::RenderEffects()
{

}

void CRenderApp::SetupStyle()
{
	ImGuiStyle* style = &ImGui::GetStyle();
	style->WindowRounding = 4;
	style->WindowBorderSize = 0;
	style->WindowShadowSize = this->margin;
	style->WindowMinSize = ImVec2(100, 100);
	style->WindowPadding = ImVec2(0, 0);

	style->FrameRounding = 4;
	style->FramePadding = ImVec2(8, 8);

	style->ChildRounding = 4;


	style->Colors[ImGuiCol_WindowBg] = colors::background;

	style->Colors[ImGuiCol_Text] = colors::text;

	style->Colors[ImGuiCol_ChildBg] = colors::transparent;
	style->Colors[ImGuiCol_DragDropTarget] = colors::text;

	style->Colors[ImGuiCol_Button] = colors::frame * ImVec4(1, 1, 1, 0.0f);
	style->Colors[ImGuiCol_ButtonHovered] = colors::frameHovered * ImVec4(1, 1, 1, 0.3f);
	style->Colors[ImGuiCol_ButtonActive] = colors::frameActive * ImVec4(1, 1, 1, 0.3f);

	style->Colors[ImGuiCol_BorderShadow] = colors::black;
	style->Colors[ImGuiCol_WindowShadow] = colors::black;

	style->Colors[ImGuiCol_TitleBg] = colors::transparent;
	style->Colors[ImGuiCol_TitleBgActive] = colors::transparent;
	style->Colors[ImGuiCol_TitleBgCollapsed] = colors::transparent;

	style->Colors[ImGuiCol_CheckMark] = colors::text;
	style->Colors[ImGuiCol_PopupBg] = colors::background;

	style->Colors[ImGuiCol_FrameBg] = colors::frame;
	style->Colors[ImGuiCol_FrameBgHovered] = colors::frameHovered;
	style->Colors[ImGuiCol_FrameBgActive] = colors::frameActive;

	style->Colors[ImGuiCol_SliderGrab] = colors::white;
	style->Colors[ImGuiCol_SliderGrabActive] = colors::text;

	style->Colors[ImGuiCol_ScrollbarBg] = colors::transparent;
	style->Colors[ImGuiCol_ScrollbarGrab] = colors::frame;
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = colors::frameActive;
	style->Colors[ImGuiCol_ScrollbarGrabActive] = colors::text;

	style->Colors[ImGuiCol_ModalWindowDimBg] = colors::transparent;
}

void CRenderApp::InitializeImGui()
{
	//std::string dir = manage_files::GetFolder();
	ImGuiIO* io = &ImGui::GetIO();

	io->IniFilename = NULL;
	io->LogFilename = NULL;

	ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = false;

	static const ImWchar ranges[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0400, 0x044F, // Cyrillic
		0,
	};

	fonts::regular_14 = io->Fonts->AddFontFromMemoryTTF(ttf_bytes_inter_regular, sizeof(ttf_bytes_inter_regular), 14.f, &fontConfig, ranges);
	fonts::bold_16 = io->Fonts->AddFontFromMemoryTTF(ttf_bytes_inter_bold, sizeof(ttf_bytes_inter_bold), 16.f, &fontConfig, ranges);
	fonts::bold_20 = io->Fonts->AddFontFromMemoryTTF(ttf_bytes_inter_bold, sizeof(ttf_bytes_inter_bold), 20.f, &fontConfig, ranges);
	fonts::bold_28 = io->Fonts->AddFontFromMemoryTTF(ttf_bytes_inter_bold, sizeof(ttf_bytes_inter_bold), 28.f, &fontConfig, ranges);
}

bool CRenderApp::Initialize(const HWND& window, const WNDCLASSEXA& wc, const int& cmd_show)
{
	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.RefreshRate.Numerator = 60U;
	sd.BufferDesc.RefreshRate.Denominator = 1U;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1U;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2U;
	sd.OutputWindow = window;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	constexpr D3D_FEATURE_LEVEL levels[2]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};

	ID3D11Device* pDevice{ nullptr };
	ID3D11DeviceContext* pContext{ nullptr };
	IDXGISwapChain* pSwap_chain{ nullptr };
	ID3D11RenderTargetView* pTarget_view{ nullptr };
	D3D_FEATURE_LEVEL level{};

	HRESULT hrCreateDevice = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&sd,
		&pSwap_chain,
		&pDevice,
		&level,
		&pContext
	);

	if (hrCreateDevice != S_OK)
		return false;

	ID3D11Texture2D* back_buffer{ nullptr };
	if (pSwap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer)) != S_OK)
		return false;

	if (back_buffer)
	{
		if (pDevice->CreateRenderTargetView(back_buffer, nullptr, &pTarget_view) != S_OK)
			return false;

		back_buffer->Release();
	}
	else return false;

	ShowWindow(window, cmd_show);
	UpdateWindow(window);

	// IMGUI 

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	SetupStyle();

	InitializeImGui();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);

	bool inCycle = this->running || !this->show_animation.AnimationEnded(Animation::back);
	while (inCycle)
	{
		inCycle = this->running || !this->show_animation.AnimationEnded(Animation::back);
		MSG msg;
		while (PeekMessageA(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);

			if (msg.message == WM_QUIT)
				running = false;
		}

		if (!inCycle)
			break;

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		RenderBody(window);

		ImGui::Render();
		constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };
		pContext->OMSetRenderTargets(1U, &pTarget_view, nullptr);
		pContext->ClearRenderTargetView(pTarget_view, color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		pSwap_chain->Present(1U, 0U);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	auto ReleaseDx = [&pSwap_chain, &pDevice, &pContext, &pTarget_view]() {
		if (pSwap_chain)	pSwap_chain->Release();
		if (pDevice)		pDevice->Release();
		if (pContext)		pContext->Release();
		if (pTarget_view)	pTarget_view->Release();
		};

	ReleaseDx();

	DestroyWindow(window);
	UnregisterClassA(wc.lpszClassName, wc.hInstance);

	return true;
}

