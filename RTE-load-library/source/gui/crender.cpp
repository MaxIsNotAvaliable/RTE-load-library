#include "crender.h"
#include <render/drawing.h>
#include "../injector/injector.h"

#include "../dnd.h"

#include <encryption/encstring.h>

#include <fonts/fonts.hpp>
#include <files_helper/files.h>
#include "../injector/GetProcAddress.h"
#include "../injector/ProcessManager.h"
#include "../inject_target.h"

#include "dialogs.h"

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

    if (ImGui::Begin(ENCSTR("RTE load library"), &this->running,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
    {
        // ============ HEADER: version chip ============
        ImGui::PushFont(fonts::bold_28);
        ImGui::PushStyleColor(ImGuiCol_Button, colors::surface);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::surface);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::surface);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 8));
        ImGui::BeginDisabled();
        ImGui::PushStyleColor(ImGuiCol_Button, colors::transparent);
#ifdef _WIN64
        
        ImGui::Button(ENCSTR("RTE-load-library"));
#else
        ImGui::Button(ENCSTR("RTE-load-library x86"));
#endif
        ImGui::PopStyleColor();
        ImGui::EndDisabled();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        ImGui::PopFont();

        // ============ DND OVERLAY ============
        static Animation animation_dnd = Animation(0.123f);
        animation_dnd.Proceed();
        animation_dnd.Start(Animation::ToAnimationState(pDropTarget->dragStatus == DragStatus_hover));
        RenderBodyDND(animationValue, animation_dnd.GetValueSin(), windowPos, windowSize);

        // ============ WINDOW CONTROLS ============
        const ImVec2 titleBtnSize = ImVec2(30, 30);
        const float titleBtnWidth = titleBtnSize.x;
        const float titleBtnPd = titleBtnWidth / 4.5f;

        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - titleBtnWidth * 2 - style->ItemSpacing.x);
        ImVec2 HidePos = tools::LocalToGlobalPos(ImGui::GetCursorPos());
        ImVec2 CrossPos = HidePos + ImVec2(titleBtnWidth + style->ItemSpacing.x, 0);

        if (items::ShadowButton(ENCSTR("##minimizeBtn"), titleBtnSize))
            this->show_animation.Start(Animation::back);
        ImGui::SameLine();
        if (items::ShadowButton(ENCSTR("##closeBtn"), titleBtnSize))
        {
            this->show_animation.Start(Animation::back);
            this->running = false;
        }

        draw::Underline(ImVec2(HidePos.x + titleBtnWidth / 2, HidePos.y + titleBtnWidth / 2),
            titleBtnPd, colors::text);
        draw::Cross(ImVec2(CrossPos.x + titleBtnWidth / 2, CrossPos.y + titleBtnWidth / 2),
            titleBtnPd * this->show_animation.GetValueSin(), colors::text);

        // ============ MAIN CONTENT ============
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::BeginChild(ENCSTR("TitleChild"), ImVec2(0, 0), 0,
            ImGuiWindowFlags_AlwaysUseWindowPadding);
        {
            const float contentAvail = ImGui::GetContentRegionAvail().x;

            // ---------- LEFT PANEL: input + process list ----------
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::backgroundPopUp);

            ImGui::BeginChild(ENCSTR("ProcessSelectChild"),
                ImVec2(contentAvail * 0.74f, 0),
                ImGuiChildFlags_AlwaysUseWindowPadding);
            {
                // === DLL FILE section ===
                ImGui::PushStyleColor(ImGuiCol_Text, colors::textMuted);
                ImGui::Text(ENCSTR("DLL FILE"));
                ImGui::PopStyleColor();

                const float openBtnW = 70.f;
                const float inputW = ImGui::GetContentRegionAvail().x - openBtnW - style->ItemSpacing.x;

                ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::frame);
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors::frameHovered);
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors::frameActive);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                ImGui::SetNextItemWidth(inputW);
                ImGui::InputText(ENCSTR("##dllFilenamePath"),
                    InjectTarget::filePath, sizeof(InjectTarget::filePath));
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, colors::surface);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::surfaceHover);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::surfaceActive);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                if (ImGui::Button(ENCSTR("Open"), ImVec2(openBtnW, 0)))
                    sprintf_s(InjectTarget::filePath, "%s", FileDialog::Open().c_str());
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 6));

                ImGui::AlignTextToFramePadding();
                ImGui::PushStyleColor(ImGuiCol_Text, colors::textMuted);
                ImGui::Text(ENCSTR("PROCESS"));
                ImGui::PopStyleColor();

                ImGui::SameLine();

#ifdef _WIN64
                static int selectedARCH = 0 | ARCH_x64 | 0;
#else
                static int selectedARCH = 0 | 0 | ARCH_x86;
#endif
                static const std::vector<int>         arches = { ARCH_x64, ARCH_x86, ARCH_UNKNOWN };
                static const std::vector<std::string> archNames = { "x64", "x86", "unknown" };

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                for (size_t i = 0; i < arches.size(); i++)
                {
#ifndef _WIN64
                    if (i == 0) // x64 in x86 space
                        continue;
#endif

                    int arch = arches[i];
                    bool isSelected = (selectedARCH & arch) != 0;

                    if (i > 0) ImGui::SameLine(0, 6);

                    if (isSelected)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, colors::activeDark);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::activeDark);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::activeDark);
                        ImGui::PushStyleColor(ImGuiCol_Text, colors::active);
                        ImGui::PushStyleColor(ImGuiCol_Border, colors::active);
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, colors::transparent);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::surfaceHover);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::surfaceActive);
                        ImGui::PushStyleColor(ImGuiCol_Text, colors::textMuted);
                        ImGui::PushStyleColor(ImGuiCol_Border, colors::border);
                    }
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

                    if (ImGui::Button(archNames[i].c_str()))
                        selectedARCH ^= arch;

                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(5);
                }

                ImGui::PopStyleVar(2);
                ImGui::PopStyleVar();

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                static char bufProc[0x100];
                ImGui::InputTextWithHint(ENCSTR("##searchByName"), ENCSTR("Search by name..."),
                    bufProc, sizeof(bufProc));
                ImGui::PopStyleVar();


                ImGui::PopStyleVar(3);

                ImGui::Dummy(ImVec2(0, 4));

                // === PROCESS LIST ===
                ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::background);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
                ImGui::BeginChild(ENCSTR("ProcessesChild"), ImVec2(0, 0),
                    ImGuiChildFlags_AlwaysUseWindowPadding);
                {
                    auto test = CProcessManager::GetProcessList(bufProc, selectedARCH);
                    auto procList = test.get();
                    static DWORD targetPid = 0;

                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.02f, 0.5f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));

                    for (size_t i = 0; i < procList->size(); i++)
                    {
                        //if ((selectedARCH & procList[i].arch) == 0)
                        //    continue;

                        EString procPid = "[" + EString((int)procList->at(i).pid) + "] ";
                        EString procArch = "  [" + EString(
                            procList->at(i).arch & ARCH_UNKNOWN ? "UNK" :
                            (procList->at(i).arch & ARCH_x64 ? "x64" : "x86")) + "]";
                        EString estr = procPid + procList->at(i).processName + procArch;

                        bool isSelected = procList->at(i).pid == targetPid;

                        ImGui::PushID((int)i);
                        if (isSelected)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Button, colors::activeDark);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::activeDark);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::activeDark);
                            ImGui::PushStyleColor(ImGuiCol_Text, colors::white);
                        }
                        else
                        {
                            ImGui::PushStyleColor(ImGuiCol_Button, colors::transparent);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::surfaceHover);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::surfaceActive);
                            ImGui::PushStyleColor(ImGuiCol_Text, colors::text);
                        }

                        if (ImGui::Button(estr.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                        {
                            targetPid = procList->at(i).pid;
                            InjectTarget::pid = targetPid;
                        }

                        // Левая акцентная полоска для selected
                        if (isSelected)
                        {
                            ImVec2 rmin = ImGui::GetItemRectMin();
                            ImVec2 rmax = ImGui::GetItemRectMax();
                            ImGui::GetWindowDrawList()->AddRectFilled(
                                ImVec2(rmin.x, rmin.y + 4),
                                ImVec2(rmin.x + 3, rmax.y - 4),
                                ImGui::GetColorU32(colors::active), 2.0f);
                        }

                        ImGui::PopStyleColor(4);
                        ImGui::PopID();
                    }

                    ImGui::PopStyleVar(3);
                }
                ImGui::EndChild();
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor();
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();// ImGuiCol_ChildBg

            ImGui::SameLine();

            // ---------- RIGHT PANEL: summary + LOAD ----------
            ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::backgroundPopUp);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));
            ImGui::BeginChild("DllFileSelectChild", ImVec2(0, 0),
                ImGuiChildFlags_AlwaysUseWindowPadding);
            {
                // --- notification popup (existing logic) ---
                static bool waitingToClose = false;
                static int  lastId = -1;
                if (lastId != InjectQueueHandler::GetLastId())
                {
                    lastId = InjectQueueHandler::GetLastId();
                    items::OpenNotify(ENCSTR("Library load"));
                    waitingToClose = true;
                }
                if (waitingToClose)
                {
                    int  resultCode = 0;
                    bool finished = InjectQueueHandler::GetResult(
                        InjectQueueHandler::GetLastId(), &resultCode);

                    EString text = finished ? (resultCode == 0
                        ? ENCSTR("Library loaded successfully!\n ")
                        : EString(ENCSTR("Unable to load library.\nError code: "))
                        + CInjector::ReturnCodeToStr(resultCode))
                        : ENCSTR("Loading...\n ");

                    auto lmFn = [&]() {
                        bool pressed = false;
                        if (ImGui::Button(ENCSTR("OK"), ImVec2(120, 0)) && finished)
                        {
                            ImGui::CloseCurrentPopup(); pressed = true;
                        }
                        ImGui::SetItemDefaultFocus();
                        ImGui::SameLine();
                        if (ImGui::Button(ENCSTR("Close"), ImVec2(120, 0)) && finished)
                        {
                            ImGui::CloseCurrentPopup(); pressed = true;
                        }
                        return false;
                        };
                    if (items::ShowNotifyLm(ENCSTR("Library load"), text.c_str(), lmFn))
                        waitingToClose = false;
                }

                // --- info header ---
                ImGui::PushStyleColor(ImGuiCol_Text, colors::textMuted);
                ImGui::Text(ENCSTR("TARGET"));
                ImGui::PopStyleColor();
                ImGui::Spacing();

                // --- info blocks (label + value) ---
                auto infoBlock = [](const char* label, const char* value, ImVec4 valColor) {
                    ImGui::PushStyleColor(ImGuiCol_Text, colors::textDim);
                    ImGui::TextUnformatted(label);
                    ImGui::PopStyleColor();
                    ImGui::PushStyleColor(ImGuiCol_Text, valColor);
                    ImGui::TextWrapped("%s", value);
                    ImGui::PopStyleColor();

                    ImGui::Dummy(ImVec2(0, 6));
                    };

                std::string dllNameRaw = strlen(InjectTarget::filePath) == 0
                    ? "Not selected"
                    : std::filesystem::path(InjectTarget::filePath).filename().string();
                infoBlock("DLL", dllNameRaw.c_str(),
                    strlen(InjectTarget::filePath) == 0 ? colors::textDim : colors::text);

                std::string pidStr = InjectTarget::pid == 0
                    ? "None" : std::to_string(InjectTarget::pid);
                infoBlock("PID", pidStr.c_str(),
                    InjectTarget::pid == 0 ? colors::textDim : colors::text);

                auto targetProc = CProcessManager::GetProcessByPid(InjectTarget::pid);

                std::string procName = targetProc.has_value() ? targetProc.value().processName : std::string({});
                infoBlock("PROCESS", targetProc.has_value() ? procName.c_str() : "None", targetProc.has_value() ? colors::text : colors::textDim);

                // --- status chip ---
                const bool isReady = strlen(InjectTarget::filePath) != 0
                    && InjectTarget::pid != 0
                    && procName.length() != 0;

                {
                    const char* statusText = isReady ? "Ready" : "Not ready";
                    ImVec4 chipBg = isReady ? colors::successDark : colors::dangerDark;
                    ImVec4 chipFg = isReady ? colors::success : colors::activeRed;

                    const ImVec2 textSz = ImGui::CalcTextSize(statusText);
                    const float  dotR = 4.0f;
                    const ImVec2 pad(10.0f, 5.0f);
                    const float  chipW = textSz.x + dotR * 2 + 8 + pad.x * 2;
                    const float  chipH = textSz.y + pad.y * 2;

                    ImVec2 chipPos = ImGui::GetCursorScreenPos();
                    ImGui::Dummy(ImVec2(chipW, chipH));

                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddRectFilled(chipPos, chipPos + ImVec2(chipW, chipH),
                        ImGui::GetColorU32(chipBg), chipH * 0.5f);
                    dl->AddRect(chipPos, chipPos + ImVec2(chipW, chipH),
                        ImGui::GetColorU32(ImVec4(chipFg.x, chipFg.y, chipFg.z, 0.45f)),
                        chipH * 0.5f, 0, 1.0f);
                    dl->AddCircleFilled(chipPos + ImVec2(pad.x + dotR, chipH * 0.5f),
                        dotR, ImGui::GetColorU32(chipFg));
                    dl->AddText(chipPos + ImVec2(pad.x + dotR * 2 + 8, pad.y),
                        ImGui::GetColorU32(chipFg), statusText);
                }

                // --- LOAD button (anchored bottom) ---
                ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 54 - style->WindowPadding.y);

                if (!isReady)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, colors::surface);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::surface);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::surface);
                    ImGui::PushStyleColor(ImGuiCol_Text, colors::textDim);
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, colors::active);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::activeHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::activePressed);
                    ImGui::PushStyleColor(ImGuiCol_Text, colors::white);
                }
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                ImGui::PushFont(fonts::bold_20);
                bool loadClick = ImGui::Button(ENCSTR("LOAD"),
                    ImVec2(ImGui::GetContentRegionAvail().x, 54));
                ImGui::PopFont();

                

                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);

                if (loadClick && isReady)
                    InjectQueueHandler::AddToQueue(InjectTarget::pid, InjectTarget::filePath);
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2); // ImGuiStyleVar_ChildRounding, ImGuiStyleVar_WindowPadding
            ImGui::PopStyleColor(); // ImGuiCol_ChildBg
        }
        ImGui::EndChild();
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

void CRenderApp::RenderBodyDND(float animationValue, float animationDNDValue, const ImVec2& windowPos, const ImVec2& windowSize)
{
	if (animationDNDValue <= 0)
		return;

	ImGuiStyle* style = &ImGui::GetStyle();

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, animationValue * animationDNDValue);
	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	const float time = (float)ImGui::GetTime();
	const float pulse = 0.5f + 0.5f * sinf(time * 3.5f);

	// ---------- 1. Затемняем всё окно ----------
	drawList->AddRectFilled(
		windowPos, windowPos + windowSize,
		ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.55f)),
		style->WindowRounding);

	// ---------- 2. Карточка ----------
	const float scale = 0.90f + 0.10f * animationDNDValue;   // pop-in
	const ImVec2 cardBase(340.0f, 190.0f);
	const ImVec2 cardSize(cardBase.x * scale, cardBase.y * scale);
	const ImVec2 cardPos = windowPos + (windowSize - cardSize) * 0.5f;

	// 2a. Мягкая тень
	for (int i = 6; i >= 1; --i)
	{
		const float e = (float)i * 2.5f;
		drawList->AddRectFilled(
			cardPos - ImVec2(e, e) + ImVec2(0, 4),
			cardPos + cardSize + ImVec2(e, e) + ImVec2(0, 4),
			ImGui::GetColorU32(ImVec4(0, 0, 0, 0.06f)),
			14.0f + e);
	}

	// 2b. Тело карточки
	drawList->AddRectFilled(
		cardPos, cardPos + cardSize,
		ImGui::GetColorU32(ImVec4(0.10f, 0.10f, 0.12f, 0.97f)),
		14.0f);

	// 2c. Пульсирующая рамка
	drawList->AddRect(
		cardPos, cardPos + cardSize,
		ImGui::GetColorU32(ImVec4(1, 1, 1, 0.20f + 0.30f * pulse)),
		14.0f, 0, 1.5f);

	// 2d. Corner-брандмауэры (scanner-стиль)
	{
		const float bl = 22.0f;
		const float th = 2.5f;
		const ImU32 bc = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.85f));
		const ImVec2 tl = cardPos;
		const ImVec2 br = cardPos + cardSize;
		const ImVec2 tr = ImVec2(br.x, tl.y);
		const ImVec2 blp = ImVec2(tl.x, br.y);

		drawList->AddLine(tl, tl + ImVec2(bl, 0), bc, th);
		drawList->AddLine(tl, tl + ImVec2(0, bl), bc, th);

		drawList->AddLine(tr, tr - ImVec2(bl, 0), bc, th);
		drawList->AddLine(tr, tr + ImVec2(0, bl), bc, th);

		drawList->AddLine(blp, blp + ImVec2(bl, 0), bc, th);
		drawList->AddLine(blp, blp - ImVec2(0, bl), bc, th);

		drawList->AddLine(br, br - ImVec2(bl, 0), bc, th);
		drawList->AddLine(br, br - ImVec2(0, bl), bc, th);
	}

	// ---------- 3. Иконка "download to tray" ----------
	{
		const ImVec2 c = cardPos + ImVec2(cardSize.x * 0.5f, 45.0f * scale);
		const float  r = 26.0f * scale;
		const ImU32 ic = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));
		const float thickness = 2.5f * scale;

		// Круг
		drawList->AddCircleFilled(c, r, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.10f)));
		drawList->AddCircle(c, r, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f)), 0, 1.0f);

		// 3a. Стрелка (стержень через AddRectFilled для идеального центра)
		drawList->AddRectFilled(
			ImVec2(c.x - thickness * 0.5f, c.y - 10.0f * scale),
			ImVec2(c.x + thickness * 0.5f, c.y + 2.0f * scale),
			ic);

		// 3b. Наконечник стрелки (треугольник)
		drawList->AddTriangleFilled(
			ImVec2(c.x - 7.0f * scale, c.y - 1.0f * scale),
			ImVec2(c.x + 7.0f * scale, c.y - 1.0f * scale),
			ImVec2(c.x, c.y + 8.0f * scale),
			ic);

		// 3c. Трей (U-образная форма через AddPolyline)
		const float tray_w = 10.0f * scale;
		const float tray_y_top = c.y + 10.0f * scale;
		const float tray_y_bottom = c.y + 16.0f * scale;

		const ImVec2 tray_points[] = {
			ImVec2(c.x - tray_w, tray_y_top),
			ImVec2(c.x - tray_w, tray_y_bottom),
			ImVec2(c.x + tray_w, tray_y_bottom),
			ImVec2(c.x + tray_w, tray_y_top)
		};
		// AddPolyline автоматически и красиво соединяет углы
		drawList->AddPolyline(tray_points, 4, ic, ImDrawFlags_None, thickness);
	}

	// ---------- 4. Заголовок ----------
	ImGui::PushFont(fonts::bold_28);
	{
		const char* title = "Drop DLL here";
		const ImVec2 sz = ImGui::CalcTextSize(title);
		drawList->AddText(
			ImGui::GetFont(), ImGui::GetFontSize(),
			cardPos + ImVec2((cardSize.x - sz.x) * 0.5f, 82.0f * scale),
			ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), title);
	}
	ImGui::PopFont();

	// ---------- 5. Подзаголовок + пилюля с файлом ----------
	ImGui::PushFont(fonts::bold_16);
	{
		const char* sub = "Release to load  -  .dll only";
		const ImVec2 sz = ImGui::CalcTextSize(sub);
		drawList->AddText(
			ImGui::GetFont(), ImGui::GetFontSize(),
			cardPos + ImVec2((cardSize.x - sz.x) * 0.5f, 114.0f * scale),
			ImGui::GetColorU32(ImVec4(1, 1, 1, 0.60f)), sub);
	}

	auto fileList = pDropTarget->GetDragFiles();
	if (!fileList.empty())
	{
		const size_t n = fileList.size();
		std::string fname = fileList[0].filename().string();
		if (fname.size() > 28) fname = fname.substr(0, 25) + "...";

		std::string info = (n == 1)
			? fname
			: (fname + "  +" + std::to_string(n - 1) + " more");

		const ImVec2 ts = ImGui::CalcTextSize(info.c_str());
		const ImVec2 ps = ts + ImVec2(20, 8);
		const ImVec2 pp = cardPos + ImVec2((cardSize.x - ps.x) * 0.5f, 144.0f * scale);

		drawList->AddRectFilled(pp, pp + ps,
			ImGui::GetColorU32(ImVec4(1, 1, 1, 0.10f)), ps.y * 0.5f);
		drawList->AddText(pp + ImVec2(10, 4),
			ImGui::GetColorU32(ImVec4(1, 1, 1, 0.85f)), info.c_str());
	}
	ImGui::PopFont();

	ImGui::PopStyleVar();
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

