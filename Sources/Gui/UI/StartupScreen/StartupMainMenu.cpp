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

#include "StartupMainMenu.h"
#include "StartupScreenUI.h"
#include <Client/Fonts.h>
#include <Client/IFont.h>
#include <Core/Strings.h>
#include <Gui/StartupScreenHelper.h>
#include <Gui/SDLRunner.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/Widgets/GuiIcons.h>
#include <Gui/UI/Widgets/TabStrip.h>
#include <SDL.h>

namespace spades {
	namespace gui {
		SPADES_SETTING(r_fullscreen);

		using ui::Button;
		using ui::CheckBox;
		using ui::SimpleButton;
		using ui::LinkButton;
		using ui::SimpleTabStrip;
		using ui::UIElement;
		using ui::UIManager;

		class SetupActionButton : public ui::Button {
		public:
			GuiIconType iconType = GuiIconType::None;
			Vector4 iconColor = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);

			SetupActionButton(ui::UIManager* manager, GuiIconType icon, const Vector4& iconCol)
			    : ui::Button(manager), iconType(icon), iconColor(iconCol) {
				alignment = MakeVector2(0.0F, 0.5F);
			}

			void Render() override {
				client::IRenderer& r = GetManager().GetRenderer();
				Vector2 pos = GetScreenPosition();
				Vector2 sz = size;

				Vector4 bgCol = MakeVector4(0.0F, 0.0F, 0.0F, 0.65F);
				Vector4 borderCol = MakeVector4(0.0F, 0.0F, 0.0F, 0.90F);
				Vector4 textCol = MakeVector4(0.95F, 0.95F, 0.95F, 1.0F);

				if (pressed || toggled) {
					bgCol = MakeVector4(0.12F, 0.45F, 0.75F, 0.75F);
					borderCol = MakeVector4(0.35F, 0.80F, 1.0F, 1.0F);
				} else if (hover) {
					bgCol = MakeVector4(0.08F, 0.22F, 0.38F, 0.70F);
					borderCol = MakeVector4(0.40F, 0.85F, 1.0F, 0.90F);
				}

				ui::SetColorNP(r, bgCol);
				r.DrawFilledRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				ui::SetColorNP(r, borderCol);
				r.DrawOutlinedRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				if (hover || toggled) {
					ui::SetColorNP(r, MakeVector4(0.40F, 0.85F, 1.0F, 1.0F));
					r.DrawFilledRect(pos.x + 1, pos.y + 1, pos.x + 3, pos.y + sz.y - 1);
				}

				if (iconType != GuiIconType::None) {
					Vector4 ic = hover ? MakeVector4(1.0F, 1.0F, 1.0F, 1.0F) : iconColor;
					DrawGuiIcon(r, iconType, MakeVector2(pos.x + 18.0F, pos.y + sz.y * 0.5F), ic, 1.15F);
				}

				client::IFont* font = GetFont();
				if (!font)
					return;

				float textStartX = (iconType != GuiIconType::None) ? 36.0F : 12.0F;
				Vector2 txtPos = MakeVector2(pos.x + textStartX,
				                             pos.y + (sz.y - font->Measure(caption).y) * 0.5F);
				font->DrawShadow(caption, txtPos, 1.0F, textCol, MakeVector4(0.0F, 0.0F, 0.0F, 0.4F));

				if (!hotKeyText.empty()) {
					Vector2 hkSize = font->Measure(hotKeyText);
					Vector2 hkPos = MakeVector2(pos.x + sz.x - hkSize.x - 10.0F,
					                            pos.y + (sz.y - hkSize.y) * 0.5F);
					font->DrawShadow(hotKeyText, hkPos, 1.0F,
					                 MakeVector4(textCol.x, textCol.y, textCol.z, 0.5F),
					                 MakeVector4(0.0F, 0.0F, 0.0F, 0.3F));
				}
			}
		};

		StartupScreenMainMenu::StartupScreenMainMenu(StartupScreenUI* ui)
		    : UIElement(&ui->GetUIManager()),
		      ui(ui),
		      helper(&ui->GetHelper()),
		      cl_showStartupWindow("cl_showStartupWindow", nullptr) {
			UIManager* manager = &GetManager();
			SetFont(&ui->GetFontManager().GetGuiFont());

			float sw = manager->screenWidth;
			float sh = manager->screenHeight;

			// Atrás button [Esc] on left
			{
				Handle<SetupActionButton> button =
				    Handle<SetupActionButton>::New(manager, GuiIconType::Back,
				                                   MakeVector4(0.92F, 0.50F, 0.50F, 1.0F));
				button->caption = _Tr("StartupScreen", "Atrás");
				button->hotKeyText = _Tr("Client", "[Esc]");
				button->SetBounds(AABB2(sw - 330.0F, 16.0F, 145.0F, 34.0F));
				button->activated = [this](UIElement&) { this->ui->shouldExit = true; };
				AddChild(button.GetPointerOrNull());
			}
			// Aplicar button [Enter] on right
			{
				Handle<SetupActionButton> button =
				    Handle<SetupActionButton>::New(manager, GuiIconType::Check,
				                                   MakeVector4(0.35F, 0.90F, 0.55F, 1.0F));
				button->caption = _Tr("StartupScreen", "Aplicar");
				button->hotKeyText = _Tr("Client", "[Enter]");
				button->SetBounds(AABB2(sw - 170.0F, 16.0F, 150.0F, 34.0F));
				button->activated = [this](UIElement&) { Start(); };
				AddChild(button.GetPointerOrNull());
			}
			float capSize = GetFont()->Measure(_Tr("StartupScreen", "Saltar esta pantalla la proxima vez")).x;
			{
				Handle<CheckBox> button = Handle<CheckBox>::New(manager);
				button->caption = _Tr("StartupScreen", "Saltar esta pantalla la proxima vez");
				button->SetBounds(AABB2(sw - capSize - 30.0F, 72.0F, capSize + 24.0F, 20.0F));
				button->visible = false;
				AddChild(button.GetPointerOrNull());
				bypassStartupWindowCheck = button.GetPointerOrNull();
				button->activated = [this](UIElement& s) {
					OnBypassStartupWindowCheckChanged(s);
				};
			}

			AABB2 clientArea(10.0F, 100.0F, sw - 20.0F, sh - 110.0F);
			Vector2 clientSize = clientArea.max - clientArea.min;

			Handle<StartupScreenGraphicsTab> graphicsTabH =
			    Handle<StartupScreenGraphicsTab>::New(ui, clientSize);
			Handle<StartupScreenAudioTab> audioTabH =
			    Handle<StartupScreenAudioTab>::New(ui, clientSize);
			Handle<StartupScreenGenericTab> genericTabH =
			    Handle<StartupScreenGenericTab>::New(ui, clientSize);
			Handle<StartupScreenProxyTab> proxyTabH =
			    Handle<StartupScreenProxyTab>::New(ui, clientSize);
			Handle<StartupScreenSystemInfoTab> profileTabH =
			    Handle<StartupScreenSystemInfoTab>::New(ui, clientSize);
			Handle<StartupScreenAdvancedTab> advancedTabH =
			    Handle<StartupScreenAdvancedTab>::New(ui, clientSize);
			graphicsTabH->SetBounds(clientArea);
			audioTabH->SetBounds(clientArea);
			genericTabH->SetBounds(clientArea);
			proxyTabH->SetBounds(clientArea);
			profileTabH->SetBounds(clientArea);
			advancedTabH->SetBounds(clientArea);
			AddChild(graphicsTabH.GetPointerOrNull());
			AddChild(audioTabH.GetPointerOrNull());
			AddChild(genericTabH.GetPointerOrNull());
			AddChild(proxyTabH.GetPointerOrNull());
			AddChild(profileTabH.GetPointerOrNull());
			AddChild(advancedTabH.GetPointerOrNull());
			audioTabH->visible = false;
			genericTabH->visible = false;
			proxyTabH->visible = false;
			profileTabH->visible = false;
			advancedTabH->visible = false;

			graphicsTab = graphicsTabH.GetPointerOrNull();
			audioTab = audioTabH.GetPointerOrNull();
			genericTab = genericTabH.GetPointerOrNull();
			proxyTab = proxyTabH.GetPointerOrNull();
			systemInfoTab = profileTabH.GetPointerOrNull();
			advancedTab = advancedTabH.GetPointerOrNull();

			{
				Handle<SimpleTabStrip> tabStrip = Handle<SimpleTabStrip>::New(manager);
				AddChild(tabStrip.GetPointerOrNull());
				tabStrip->SetBounds(AABB2(10.0F, 68.0F, sw - 20.0F, 28.0F));
				tabStrip->AddItem(_Tr("StartupScreen", "Graphics"), graphicsTab);
				tabStrip->AddItem(_Tr("StartupScreen", "Audio"), audioTab);
				tabStrip->AddItem(_Tr("StartupScreen", "Generic"), genericTab);
				tabStrip->AddItem(_Tr("StartupScreen", "Proxy"), proxyTab);
				tabStrip->AddItem(_Tr("StartupScreen", "System Info"), systemInfoTab);
				tabStrip->AddItem(_Tr("StartupScreen", "Advanced"), advancedTab);
				tabStrip->changed = [this](UIElement& s) { OnTabChanged(s); };
			}

			LoadConfig();
		}

		void StartupScreenMainMenu::OnWebsitePressed(UIElement&) {
			if (helper->OpenLinkInBrowser("https://zerospades.github.io/"))
				return;

			std::string msg =
				_Tr("StartupScreen", "An unknown error has occurred while opening this url.");
			Handle<AlertScreen> al = Handle<AlertScreen>::New(GetParent(), msg, 100.0F);
			al->Run();
		}

		void StartupScreenMainMenu::OnTabChanged(UIElement&) {
			bool v = advancedTab->visible;
			if (advancedTabVisible && !v)
				LoadConfig();
			advancedTabVisible = v;
		}

		void StartupScreenMainMenu::LoadConfig() {
			switch (static_cast<int>(cl_showStartupWindow)) {
				case -1: bypassStartupWindowCheck->toggled = false; break;
				case 0: bypassStartupWindowCheck->toggled = true; break;
				default: bypassStartupWindowCheck->toggled = false; break;
			}

			graphicsTab->LoadConfig();
			audioTab->LoadConfig();
			genericTab->LoadConfig();
			proxyTab->LoadConfig();
			advancedTab->LoadConfig();
		}

		StartupScreenMainMenuState StartupScreenMainMenu::GetState() {
			StartupScreenMainMenuState state;
			if (graphicsTab->visible) {
				state.activeTabIndex = 0;
			} else if (audioTab->visible) {
				state.activeTabIndex = 1;
			} else if (genericTab->visible) {
				state.activeTabIndex = 2;
			} else if (proxyTab->visible) {
				state.activeTabIndex = 3;
			} else if (systemInfoTab->visible) {
				state.activeTabIndex = 4;
			} else if (advancedTab->visible) {
				state.activeTabIndex = 5;
			}
			return state;
		}

		void StartupScreenMainMenu::SetState(const StartupScreenMainMenuState& state) {
			graphicsTab->visible = state.activeTabIndex == 0;
			audioTab->visible = state.activeTabIndex == 1;
			genericTab->visible = state.activeTabIndex == 2;
			proxyTab->visible = state.activeTabIndex == 3;
			systemInfoTab->visible = state.activeTabIndex == 4;
			advancedTab->visible = state.activeTabIndex == 5;
		}

		void StartupScreenMainMenu::OnBypassStartupWindowCheckChanged(UIElement&) {
			cl_showStartupWindow = (bypassStartupWindowCheck->toggled ? 0 : 1);
		}

		void StartupScreenMainMenu::Start() {
			helper->Start();
			SDLRunner::ApplyWindowVideoMode();
			ui->shouldExit = true; // exit from the startup screen to return to the game
		}

		void StartupScreenMainMenu::HotKey(const std::string& key) {
			if (IsEnabled() && key == "Enter") {
				Start();
			} else if (IsEnabled() && key == "Escape") {
				ui->shouldExit = true;
			} else {
				UIElement::HotKey(key);
			}
		}
	} // namespace gui
} // namespace spades
