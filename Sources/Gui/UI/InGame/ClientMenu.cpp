/*
 Copyright (c) 2026 Fran6nd, ZeroSpades developers.

 This file is part of ZeroSpades, a fork of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "ClientMenu.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <Client/ClientUI.h>
#include <Client/ClientUIHelper.h>
#include <Client/Fonts.h>
#include <Client/NetClient.h>
#include <Core/Strings.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/Widgets/Button.h>
#include <Gui/UI/Widgets/Label.h>
#include <Gui/UI/Widgets/GuiIcons.h>

namespace spades {
	namespace client {
		using gui::PreferenceView;
		using gui::PreferenceViewOptions;
		using gui::PreferenceViewPersistedState;
		using gui::ui::Button;
		using gui::ui::Label;
		using gui::ui::UIElement;
		using gui::ui::UIManager;

		namespace {
			class InGameMenuButton : public Button {
			public:
				gui::GuiIconType iconType = gui::GuiIconType::None;
				Vector4 iconColor = MakeVector4(0.85F, 0.90F, 0.95F, 0.90F);

				InGameMenuButton(UIManager* manager, gui::GuiIconType icon, const Vector4& iconCol)
				    : Button(manager), iconType(icon), iconColor(iconCol) {}

				void Render() override {
					client::IRenderer& r = GetManager().GetRenderer();
					Vector2 pos = GetScreenPosition();
					Vector2 sz = size;

					Vector4 bgCol = MakeVector4(0.0F, 0.0F, 0.0F, 0.60F);
					Vector4 icCol = iconColor;

					if (IsEnabled()) {
						if (pressed && hover) {
							bgCol = MakeVector4(0.20F, 0.20F, 0.20F, 0.95F);
							icCol = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
						} else if (hover) {
							bgCol = MakeVector4(0.12F, 0.12F, 0.12F, 0.85F);
							icCol = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
						}
					} else {
						bgCol.w *= 0.5F;
						icCol.w *= 0.4F;
					}

					gui::ui::SetColorNP(r, bgCol);
					r.DrawFilledRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

					Vector4 borderCol = (pressed && hover) ? MakeVector4(0.85F, 0.85F, 0.85F, 0.95F)
					                    : hover ? MakeVector4(0.60F, 0.60F, 0.60F, 0.80F)
					                            : MakeVector4(0.0F, 0.0F, 0.0F, 0.90F);
					gui::ui::SetColorNP(r, borderCol);
					r.DrawOutlinedRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

					float iconX = (sz.y < 30.0F) ? 14.0F : 20.0F;
					float iconScale = (sz.y < 30.0F) ? 0.90F : 1.15F;
					float textX = (sz.y < 30.0F) ? 30.0F : 42.0F;
					float textScale = (sz.y < 30.0F) ? 0.85F : 1.0F;

					if (iconType != gui::GuiIconType::None) {
						gui::DrawGuiIcon(r, iconType, MakeVector2(pos.x + iconX, pos.y + sz.y * 0.5F), icCol, iconScale);
					}

					client::IFont* font = GetFont();
					if (font && !caption.empty()) {
						Vector4 textCol = hover ? MakeVector4(1.0F, 1.0F, 1.0F, 1.0F)
						                        : MakeVector4(0.92F, 0.92F, 0.92F, 0.95F);
						Vector2 textSize = font->Measure(caption) * textScale;
						Vector2 textPos = MakeVector2(pos.x + textX, pos.y + (sz.y - textSize.y) * 0.5F);
						font->DrawShadow(caption, textPos, textScale, textCol, MakeVector4(0.0F, 0.0F, 0.0F, 0.75F));
					}
				}
			};
		} // namespace

		ClientMenu::ClientMenu(ClientUI* ui)
		    : UIElement(&ui->GetUIManager()), ui(ui), helper(&ui->GetHelper()) {
			preferenceState = Handle<PreferenceViewPersistedState>::New();

			UIManager* manager = &GetManager();
			float sw = manager->screenWidth;
			float sh = manager->screenHeight;

			bool isSpectator = ui->IsLocalPlayerSpectator();

			float winW = isSpectator ? 170.0F : 240.0F;
			float btnH = isSpectator ? 25.0F : 34.0F;
			float spacing = isSpectator ? 4.0F : 6.0F;
			float headerH = isSpectator ? 24.0F : 34.0F;
			float cardPadX = isSpectator ? 8.0F : 14.0F;
			float cardPadY = isSpectator ? 8.0F : 12.0F;

			// winH encompasses title header + 4 buttons + spacing
			float winH = headerH + 2.0F + (btnH + spacing) * 3.0F + btnH;
			float winX = isSpectator ? 16.0F : (sw - winW) * 0.5F;
			float winY = isSpectator ? 16.0F : (sh - winH) * 0.5F;

			// draw full background overlay (dim 3D gameplay) ONLY when NOT in spectator mode
			if (!isSpectator) {
				Handle<Label> overlay = Handle<Label>::New(manager);
				overlay->backgroundColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.70F);
				overlay->SetBounds(AABB2(0.0F, 0.0F, sw, sh));
				AddChild(overlay.GetPointerOrNull());
			}

			// modal menu card background in HD transparent black (strictly surrounds all content)
			{
				Handle<Label> label = Handle<Label>::New(manager);
				label->backgroundColor = isSpectator ? MakeVector4(0.02F, 0.02F, 0.02F, 0.75F)
				                                     : MakeVector4(0.02F, 0.02F, 0.02F, 0.90F);
				label->outlineColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.95F);
				label->SetBounds(AABB2(winX - cardPadX, winY - cardPadY, winW + cardPadX * 2.0F, winH + cardPadY * 2.0F));
				AddChild(label.GetPointerOrNull());
			}

			// menu title header
			{
				Handle<Label> title = Handle<Label>::New(manager);
				title->text = isSpectator ? _Tr("Client", "Spectator Menu") : _Tr("Client", "ySpades - Menu");
				title->textColor = MakeVector4(1.0F, 1.0F, 1.0F, 0.95F);
				title->alignment = MakeVector2(0.5F, 0.5F);
				title->SetBounds(AABB2(winX, winY, winW, headerH));
				AddChild(title.GetPointerOrNull());
			}

			float curBtnY = winY + headerH + 2.0F;

			// 1. Back to Game (Connect/Play icon)
			{
				Handle<InGameMenuButton> button = Handle<InGameMenuButton>::New(
				    manager, gui::GuiIconType::Connect, MakeVector4(0.35F, 0.85F, 1.0F, 1.0F));
				button->caption = _Tr("Client", "Back to Game");
				button->SetBounds(AABB2(winX, curBtnY, winW, btnH));
				button->activated = [this](UIElement& s) { OnBackToGame(s); };
				AddChild(button.GetPointerOrNull());
			}
			curBtnY += btnH + spacing;

			// 2. Chat Log (Chat bubble icon)
			{
				Handle<InGameMenuButton> button = Handle<InGameMenuButton>::New(
				    manager, gui::GuiIconType::Chat, MakeVector4(0.40F, 0.75F, 1.0F, 1.0F));
				button->caption = _Tr("Client", "Chat Log");
				button->SetBounds(AABB2(winX, curBtnY, winW, btnH));
				button->activated = [this](UIElement& s) { OnChatLog(s); };
				AddChild(button.GetPointerOrNull());
			}
			curBtnY += btnH + spacing;

			// 3. Setup / Settings (Gear icon)
			{
				Handle<InGameMenuButton> button = Handle<InGameMenuButton>::New(
				    manager, gui::GuiIconType::Settings, MakeVector4(0.85F, 0.90F, 0.95F, 1.0F));
				button->caption = _Tr("Client", "Setup");
				button->SetBounds(AABB2(winX, curBtnY, winW, btnH));
				button->activated = [this](UIElement& s) { OnSetup(s); };
				AddChild(button.GetPointerOrNull());
			}
			curBtnY += btnH + spacing;

			// 4. Disconnect (Exit Door icon)
			{
				Handle<InGameMenuButton> button = Handle<InGameMenuButton>::New(
				    manager, gui::GuiIconType::Disconnect, MakeVector4(0.95F, 0.35F, 0.35F, 1.0F));
				button->caption = helper->IsDemoMode() ? _Tr("Client", "Exit Demo")
				                                       : _Tr("Client", "Disconnect");
				button->SetBounds(AABB2(winX, curBtnY, winW, btnH));
				button->activated = [this](UIElement& s) { OnDisconnect(s); };
				AddChild(button.GetPointerOrNull());
			}
		}

		void ClientMenu::OnBackToGame(UIElement&) { ui->SetActiveUI(nullptr); }

		void ClientMenu::OnSetup(UIElement&) {
			PreferenceViewOptions opt;
			opt.gameActive = true;
			opt.persistedState = preferenceState;

			Handle<PreferenceView> al =
				Handle<PreferenceView>::New(this, opt, &ui->GetFontManager());
			al->closed = [](UIElement&) {};
			al->onDetailedRequested = [](UIElement&) {
#ifdef _WIN32
				STARTUPINFOA si;
				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				PROCESS_INFORMATION pi;
				ZeroMemory(&pi, sizeof(pi));
				char exePath[MAX_PATH] = {0};
				GetModuleFileNameA(NULL, exePath, MAX_PATH);
				std::string cmd = std::string("\"") + exePath + "\" --setup";
				if (CreateProcessA(NULL, &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
#endif
			};
			al->onVentanaRequested = al->onDetailedRequested;
			al->Run();
		}

		void ClientMenu::OnChatLog(UIElement&) { ui->EnterChatLogWindow(); }

		void ClientMenu::OnDisconnect(UIElement&) { ui->shouldExit = true; }

		void ClientMenu::HotKey(const std::string& key) {
			if (IsEnabled() && key == "Escape") {
				ui->SetActiveUI(nullptr);
			} else {
				UIElement::HotKey(key);
			}
		}

		// IModalMenu implementation
		bool ClientMenu::IsActive() const { return IsEnabled(); }
		void ClientMenu::Open() { /* Managed by ui->SetActiveUI(this) */ }
		void ClientMenu::Close() { if (ui) ui->SetActiveUI(nullptr); }
		bool ClientMenu::KeyEvent(const std::string& key, bool down) {
			if (down) KeyDown(key);
			else KeyUp(key);
			return true;
		}
		void ClientMenu::TextInputEvent(const std::string& text) { KeyPress(text); }
		bool ClientMenu::AcceptsTextInput() const { return IsEnabled(); }
		void ClientMenu::Draw() { Render(); }

		void ClientMenu::Render() {
			UIElement::Render();

			client::IRenderer& r = GetManager().GetRenderer();
			IFont& guiFont = ui->GetFontManager().GetGuiFont();
			IFont& headingFont = ui->GetFontManager().GetHeadingFont();

			Vector4 white = MakeVector4(1, 1, 1, 1);
			Vector4 shadow = MakeVector4(0, 0, 0, 0.6F);
			Vector4 cyan = MakeVector4(0.35F, 0.85F, 1.0F, 1.0F);
			Vector4 green = MakeVector4(0.30F, 0.85F, 0.45F, 1.0F);

			// Format playing time
			int now = static_cast<int>(helper->GetClientTime());
			int hrs = now / 3600;
			int mins = (now % 3600) / 60;
			int secs = now % 60;

			std::string time;
			if (hrs > 0)
				time = _Tr("Client", "{0}h {1}m {2}s", ToString(hrs), ToString(mins), ToString(secs));
			else if (mins > 0)
				time = _Tr("Client", "{0}m {1}s", ToString(mins), ToString(secs));
			else
				time = _Tr("Client", "{0}s", ToString(secs));

			std::string timeStr = _Tr("Client", "Session time: {0}", time);
			std::string serverName = helper->GetServerName();
			std::string serverStr = serverName.empty() ? "" : _Tr("Client", "Connected to: {0}", serverName);

			// Calculate card dimensions dynamically based on font metrics
			float cardX = 14.0F;
			float cardY = 14.0F;
			float cardW = 280.0F;

			float tW = headingFont.Measure(timeStr).x + 46.0F;
			if (tW > cardW) cardW = tW;
			float sH = 0.0F;
			if (!serverStr.empty()) {
				float sW = guiFont.Measure(serverStr).x + 46.0F;
				if (sW > cardW) cardW = sW;
				sH = guiFont.Measure(serverStr).y;
			}

			float timeH = headingFont.Measure(timeStr).y;
			float cardH = 12.0F + timeH + (!serverStr.empty() ? (8.0F + sH) : 0.0F) + 12.0F;

			// Floating glass card background & border in HD transparent black
			gui::ui::SetColorNP(r, MakeVector4(0.0F, 0.0F, 0.0F, 0.75F));
			r.DrawFilledRect(cardX, cardY, cardX + cardW, cardY + cardH);

			gui::ui::SetColorNP(r, MakeVector4(0.0F, 0.0F, 0.0F, 0.90F));
			r.DrawOutlinedRect(cardX, cardY, cardX + cardW, cardY + cardH);

			// Subtle top accent line (silver)
			gui::ui::SetColorNP(r, MakeVector4(0.40F, 0.40F, 0.40F, 0.60F));
			r.DrawLine(MakeVector2(cardX + 2.0F, cardY + 1.0F), MakeVector2(cardX + cardW - 2.0F, cardY + 1.0F), 1.5F);

			float curY = cardY + 12.0F;

			// 1. Session Time with Clock icon
			gui::DrawGuiIcon(r, gui::GuiIconType::Clock, MakeVector2(cardX + 18.0F, curY + timeH * 0.5F), cyan, 1.25F);
			headingFont.DrawShadow(timeStr, MakeVector2(cardX + 36.0F, curY), 1.0F, white, shadow);
			curY += timeH + 8.0F;

			// 2. Server Name with Server icon
			if (!serverStr.empty()) {
				gui::DrawGuiIcon(r, gui::GuiIconType::Server, MakeVector2(cardX + 18.0F, curY + sH * 0.5F), green, 1.15F);
				guiFont.DrawShadow(serverStr, MakeVector2(cardX + 36.0F, curY), 1.0F, MakeVector4(0.92F, 0.95F, 0.98F, 1.0F), shadow);
			}
		}
	} // namespace client
} // namespace spades
