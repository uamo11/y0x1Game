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

#include "LobbyScreen.h"

#include "MainScreenUI.h"
#include "SkinSelectScreen.h"
#include "ModWeaponSelectScreen.h"
#include <Client/Fonts.h>
#include <Client/IFont.h>
#include <Client/IRenderer.h>
#include <Core/Settings.h>
#include <Core/Strings.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/Settings/PreferenceView.h>
#include <Gui/UI/Widgets/DrawUtils.h>
#include <Gui/UpdateChecker.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace spades {
	namespace gui {
		using ui::SetColorNP;
		using ui::UIElement;
		using ui::UIManager;

		enum class LobbyIconType {
			Play,
			Skins,
			Mods,
			Settings,
			Quit
		};

		class LobbyIconButton : public ui::ButtonBase {
			LobbyIconType iconType;

		public:
			LobbyIconButton(UIManager* manager, LobbyIconType type)
			    : ui::ButtonBase(manager), iconType(type) {}

			void Render() override {
				client::IRenderer& r = GetManager().GetRenderer();
				Vector2 pos = GetScreenPosition();
				Vector2 sz = size;

				// Background color: sleek neutral transparent black
				Vector4 bg(0.0F, 0.0F, 0.0F, 0.55F);
				if (pressed && hover) {
					bg = MakeVector4(0.02F, 0.02F, 0.02F, 0.90F);
				} else if (hover) {
					bg = MakeVector4(0.12F, 0.12F, 0.12F, 0.80F);
				}

				SetColorNP(r, bg);
				r.DrawImage(nullptr, AABB2(pos.x, pos.y, sz.x, sz.y));

				// Border line: sharp 1px black border with silver highlight on hover
				Vector4 borderColor(0.0F, 0.0F, 0.0F, 0.90F);
				float borderW = 1.0F;
				if (hover) {
					borderColor = MakeVector4(0.70F, 0.70F, 0.70F, 0.85F);
				}

				SetColorNP(r, borderColor);
				r.DrawImage(nullptr, AABB2(pos.x, pos.y, sz.x, borderW));
				r.DrawImage(nullptr, AABB2(pos.x, pos.y + sz.y - borderW, sz.x, borderW));
				r.DrawImage(nullptr, AABB2(pos.x, pos.y, borderW, sz.y));
				r.DrawImage(nullptr, AABB2(pos.x + sz.x - borderW, pos.y, borderW, sz.y));

				// Icon color: crisp white / silver
				Vector4 iconCol(0.92F, 0.92F, 0.92F, 0.95F);
				if (hover) {
					iconCol = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
				}

				SetColorNP(r, iconCol);
				float cx = pos.x + sz.x * 0.5F;
				float cy = pos.y + sz.y * 0.5F;

				if (iconType == LobbyIconType::Play) {
					// Draw filled play triangle pointing right
					float playX = cx + 2.0F; // optical centering
					float leftX = playX - 10.0F;
					float rightX = playX + 12.0F;
					for (float x = leftX; x <= rightX; x += 1.0F) {
						float t = (x - leftX) / 22.0F;
						float halfH = 13.0F * (1.0F - t);
						r.DrawLine(MakeVector2(x, cy - halfH), MakeVector2(x, cy + halfH), 1.5F);
					}
				} else if (iconType == LobbyIconType::Skins) {
					// Draw Character / T-shirt skin silhouette
					float s = 1.6F;
					// Shoulders
					r.DrawLine(MakeVector2(cx - 7.0F * s, cy - 2.5F * s), MakeVector2(cx - 2.5F * s, cy - 5.5F * s), 2.0F * s);
					r.DrawLine(MakeVector2(cx + 2.5F * s, cy - 5.5F * s), MakeVector2(cx + 7.0F * s, cy - 2.5F * s), 2.0F * s);
					// V-neck collar
					r.DrawLine(MakeVector2(cx - 2.5F * s, cy - 5.5F * s), MakeVector2(cx, cy - 2.0F * s), 2.0F * s);
					r.DrawLine(MakeVector2(cx + 2.5F * s, cy - 5.5F * s), MakeVector2(cx, cy - 2.0F * s), 2.0F * s);
					// Sleeves
					r.DrawLine(MakeVector2(cx - 7.0F * s, cy - 2.5F * s), MakeVector2(cx - 8.5F * s, cy + 1.5F * s), 2.0F * s);
					r.DrawLine(MakeVector2(cx - 8.5F * s, cy + 1.5F * s), MakeVector2(cx - 5.5F * s, cy + 2.0F * s), 2.0F * s);
					r.DrawLine(MakeVector2(cx + 7.0F * s, cy - 2.5F * s), MakeVector2(cx + 8.5F * s, cy + 1.5F * s), 2.0F * s);
					r.DrawLine(MakeVector2(cx + 8.5F * s, cy + 1.5F * s), MakeVector2(cx + 5.5F * s, cy + 2.0F * s), 2.0F * s);
					// Torso sides & bottom hem
					r.DrawLine(MakeVector2(cx - 5.5F * s, cy + 2.0F * s), MakeVector2(cx - 5.5F * s, cy + 7.5F * s), 2.0F * s);
					r.DrawLine(MakeVector2(cx + 5.5F * s, cy + 2.0F * s), MakeVector2(cx + 5.5F * s, cy + 7.5F * s), 2.0F * s);
					r.DrawLine(MakeVector2(cx - 5.5F * s, cy + 7.5F * s), MakeVector2(cx + 5.5F * s, cy + 7.5F * s), 2.0F * s);
					// Head
					r.DrawImage(nullptr, AABB2(cx - 3.2F * s, cy - 10.5F * s, 6.4F * s, 4.5F * s));
				} else if (iconType == LobbyIconType::Mods) {
					// Draw Tactical Crosshairs / Target icon
					float rOuter = 10.0F;
					r.DrawLine(MakeVector2(cx - 13.0F, cy), MakeVector2(cx - 5.0F, cy), 2.2F);
					r.DrawLine(MakeVector2(cx + 5.0F, cy), MakeVector2(cx + 13.0F, cy), 2.2F);
					r.DrawLine(MakeVector2(cx, cy - 13.0F), MakeVector2(cx, cy - 5.0F), 2.2F);
					r.DrawLine(MakeVector2(cx, cy + 5.0F), MakeVector2(cx, cy + 13.0F), 2.2F);
					r.DrawLine(MakeVector2(cx - rOuter, cy - rOuter), MakeVector2(cx + rOuter, cy - rOuter), 1.5F);
					r.DrawLine(MakeVector2(cx + rOuter, cy - rOuter), MakeVector2(cx + rOuter, cy + rOuter), 1.5F);
					r.DrawLine(MakeVector2(cx + rOuter, cy + rOuter), MakeVector2(cx - rOuter, cy + rOuter), 1.5F);
					r.DrawLine(MakeVector2(cx - rOuter, cy + rOuter), MakeVector2(cx - rOuter, cy - rOuter), 1.5F);
					r.DrawFilledRect(cx - 1.5F, cy - 1.5F, cx + 1.5F, cy + 1.5F);
				} else if (iconType == LobbyIconType::Settings) {
					// Draw gear icon
					// 8 teeth spokes
					r.DrawLine(MakeVector2(cx - 13.0F, cy), MakeVector2(cx + 13.0F, cy), 4.5F);
					r.DrawLine(MakeVector2(cx, cy - 13.0F), MakeVector2(cx, cy + 13.0F), 4.5F);
					r.DrawLine(MakeVector2(cx - 9.0F, cy - 9.0F), MakeVector2(cx + 9.0F, cy + 9.0F), 4.5F);
					r.DrawLine(MakeVector2(cx - 9.0F, cy + 9.0F), MakeVector2(cx + 9.0F, cy - 9.0F), 4.5F);
					// Central solid hub
					r.DrawImage(nullptr, AABB2(cx - 7.0F, cy - 7.0F, 14.0F, 14.0F));
					// Inner cutout hole filled with button bg
					SetColorNP(r, bg);
					r.DrawImage(nullptr, AABB2(cx - 3.5F, cy - 3.5F, 7.0F, 7.0F));
				} else if (iconType == LobbyIconType::Quit) {
					// Draw bold 'X' cross
					r.DrawLine(MakeVector2(cx - 10.0F, cy - 10.0F), MakeVector2(cx + 10.0F, cy + 10.0F), 3.5F);
					r.DrawLine(MakeVector2(cx - 10.0F, cy + 10.0F), MakeVector2(cx + 10.0F, cy - 10.0F), 3.5F);
				}

				// Hover tooltip pill to the right of button
				client::IFont* font = GetFont();
				if (hover && font) {
					std::string tipText;
					Vector4 tipColor(1.0F, 1.0F, 1.0F, 1.0F);
					if (iconType == LobbyIconType::Play) {
						tipText = _Tr("MainScreen", "JUGAR");
						tipColor = MakeVector4(0.40F, 1.0F, 0.60F, 1.0F);
					} else if (iconType == LobbyIconType::Skins) {
						tipText = _Tr("MainScreen", "SKINS");
						tipColor = MakeVector4(0.92F, 0.65F, 1.0F, 1.0F);
					} else if (iconType == LobbyIconType::Mods) {
						tipText = _Tr("MainScreen", "MODS / ARMAS");
						tipColor = MakeVector4(0.35F, 0.85F, 1.0F, 1.0F);
					} else if (iconType == LobbyIconType::Settings) {
						tipText = _Tr("MainScreen", "AJUSTES");
						tipColor = MakeVector4(0.60F, 0.85F, 1.0F, 1.0F);
					} else if (iconType == LobbyIconType::Quit) {
						tipText = _Tr("MainScreen", "SALIR");
						tipColor = MakeVector4(1.0F, 0.45F, 0.45F, 1.0F);
					}

					if (!tipText.empty()) {
						Vector2 textSize = font->Measure(tipText);
						float tipW = textSize.x + 20.0F;
						float tipH = 28.0F;
						float tipX = pos.x + sz.x + 12.0F;
						float tipY = cy - tipH * 0.5F;

						SetColorNP(r, MakeVector4(0.04F, 0.06F, 0.10F, 0.90F));
						r.DrawImage(nullptr, AABB2(tipX, tipY, tipW, tipH));

						SetColorNP(r, borderColor);
						r.DrawImage(nullptr, AABB2(tipX, tipY, tipW, 1.0F));
						r.DrawImage(nullptr, AABB2(tipX, tipY + tipH - 1.0F, tipW, 1.0F));
						r.DrawImage(nullptr, AABB2(tipX, tipY, 1.0F, tipH));
						r.DrawImage(nullptr, AABB2(tipX + tipW - 1.0F, tipY, 1.0F, tipH));

						Vector4 shadowColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.8F);
						font->DrawShadow(tipText, MakeVector2(tipX + 10.0F, tipY + (tipH - textSize.y) * 0.5F), 1.0F,
						                tipColor, shadowColor);
					}
				}
			}
		};

		LobbyScreen::LobbyScreen(MainScreenUI* ui)
		    : UIElement(&ui->GetUIManager()), ui(ui) {
			UIManager* manager = &GetManager();
			SetFont(&ui->GetFontManager().GetGuiFont());

			SetBounds(manager->GetRootElement().GetBounds());

			// Play Button (icon only)
			{
				Handle<LobbyIconButton> btn =
				    Handle<LobbyIconButton>::New(manager, LobbyIconType::Play);
				btn->activated = [this](UIElement& s) { OnPlayPressed(s); };
				playButton = btn.GetPointerOrNull();
				AddChild(playButton);
			}

			// Skins Button (icon only)
			{
				Handle<LobbyIconButton> btn =
				    Handle<LobbyIconButton>::New(manager, LobbyIconType::Skins);
				btn->activated = [this](UIElement& s) { OnSkinsPressed(s); };
				skinsButton = btn.GetPointerOrNull();
				AddChild(skinsButton);
			}

			// Mods / Weapons Button
			{
				Handle<LobbyIconButton> btn =
				    Handle<LobbyIconButton>::New(manager, LobbyIconType::Mods);
				btn->activated = [this](UIElement& s) { OnModsPressed(s); };
				modsButton = btn.GetPointerOrNull();
				AddChild(modsButton);
			}

			// Settings Button (icon only)
			{
				Handle<LobbyIconButton> btn =
				    Handle<LobbyIconButton>::New(manager, LobbyIconType::Settings);
				btn->activated = [this](UIElement& s) { OnSettingsPressed(s); };
				settingsButton = btn.GetPointerOrNull();
				AddChild(settingsButton);
			}

			// Quit Button (icon only)
			{
				Handle<LobbyIconButton> btn =
				    Handle<LobbyIconButton>::New(manager, LobbyIconType::Quit);
				btn->activated = [this](UIElement& s) { OnQuitPressed(s); };
				quitButton = btn.GetPointerOrNull();
				AddChild(quitButton);
			}

			Layout();
		}

		void LobbyScreen::Layout() {
			float sw = GetManager().screenWidth;
			float sh = GetManager().screenHeight;
			SetBounds(AABB2(0.0F, 0.0F, sw, sh));

			float btnSize = 56.0F;
			float btnGap = 16.0F;
			float totalH = 5.0F * btnSize + 4.0F * btnGap;
			float startY = (sh - totalH) * 0.5F;
			if (startY < 45.0F)
				startY = 45.0F;

			float btnX = 64.0F;
			if (sw < 640.0F)
				btnX = 24.0F;

			if (playButton)
				playButton->SetBounds(AABB2(btnX, startY, btnSize, btnSize));

			if (skinsButton)
				skinsButton->SetBounds(AABB2(btnX, startY + 1.0F * (btnSize + btnGap), btnSize, btnSize));

			if (modsButton)
				modsButton->SetBounds(AABB2(btnX, startY + 2.0F * (btnSize + btnGap), btnSize, btnSize));

			if (settingsButton)
				settingsButton->SetBounds(AABB2(btnX, startY + 3.0F * (btnSize + btnGap), btnSize, btnSize));

			if (quitButton)
				quitButton->SetBounds(AABB2(btnX, startY + 4.0F * (btnSize + btnGap), btnSize, btnSize));
		}

		void LobbyScreen::OnPlayPressed(UIElement&) {
			ui->ShowServerBrowser();
		}

		void LobbyScreen::OnSkinsPressed(UIElement&) {
			Handle<SkinSelectScreen> screen =
			    Handle<SkinSelectScreen>::New(this, &ui->GetFontManager(), ui);
			screen->Run();
		}

		void LobbyScreen::OnModsPressed(UIElement&) {
			ui->ShowModsBrowser();
		}

		void LobbyScreen::OnSettingsPressed(UIElement&) {
			PreferenceViewOptions opt;
			opt.showOverlay = true;
			Handle<PreferenceView> pref =
			    Handle<PreferenceView>::New(this, opt, &ui->GetFontManager());
			pref->onDetailedRequested = [this](UIElement&) {
				ui->OpenDetailedSettings();
			};
			pref->onVentanaRequested = [this](UIElement&) {
				ui->OpenDetailedSettings();
			};
			pref->Run();
		}

		void LobbyScreen::OnQuitPressed(UIElement&) {
			ui->shouldExit = true;
		}

		void LobbyScreen::HotKey(const std::string& key) {
			if (IsEnabled() && visible) {
				if (key == "Enter") {
					OnPlayPressed(*this);
					return;
				} else if (key == "Escape") {
					OnQuitPressed(*this);
					return;
				}
			}
			UIElement::HotKey(key);
		}

		void LobbyScreen::Render() {
			Layout();

			client::IRenderer& r = GetManager().GetRenderer();
			Vector2 sz = size;

			float btnSize = 56.0F;
			float btnGap = 16.0F;
			float totalH = 4.0F * btnSize + 3.0F * btnGap;
			float startY = (sz.y - totalH) * 0.5F;
			if (startY < 60.0F)
				startY = 60.0F;

			float btnX = 64.0F;
			if (sz.x < 640.0F)
				btnX = 24.0F;

			UIElement::Render();

			// Small subtle version text in bottom-right corner of lobby
			client::IFont* font = GetFont();
			if (font) {
				std::string verText = _Tr("MainScreen", "Actualizado ") + ReleaseVersion::Current().ToString();
				Vector2 vSize = font->Measure(verText);
				float scale = 0.72F;
				Vector2 vPos = MakeVector2(sz.x - vSize.x * scale - 20.0F, sz.y - vSize.y * scale - 14.0F);
				Vector4 col = MakeVector4(0.50F, 0.85F, 0.55F, 0.70F);
				Vector4 shadow = MakeVector4(0.0F, 0.0F, 0.0F, 0.50F);
				font->DrawShadow(verText, vPos, scale, col, shadow);
			}
		}
	} // namespace gui
} // namespace spades
