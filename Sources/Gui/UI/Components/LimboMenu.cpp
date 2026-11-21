/*
 Copyright (c) 2026 Fran6nd, ZeroSpades developers.

 This file is part of ZeroSpades, a fork of OpenSpades.

 ZeroSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 ZeroSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with ZeroSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "LimboMenu.h"
#include "SoftwareCursor.h"
#include <Client/IRenderer.h>
#include <Client/IModel.h>
#include <Client/SceneDefinition.h>
#include <Client/Fonts.h>
#include <Client/IFont.h>
#include <Core/Strings.h>
#include <Core/Math.h>

namespace spades {
	namespace gui {
		namespace {
			/** Team id a team row stands for; matches `LimboView`'s 0/1/2 encoding. */
			int TeamIdOf(MenuItemType type) {
				switch (type) {
					case MenuItemType::Team1: return 0;
					case MenuItemType::Team2: return 1;
					default: return 2;
				}
			}
			WeaponType WeaponOf(MenuItemType type) {
				switch (type) {
					case MenuItemType::WeaponRifle: return RIFLE_WEAPON;
					case MenuItemType::WeaponSMG: return SMG_WEAPON;
					default: return SHOTGUN_WEAPON;
				}
			}
		} // namespace

		LimboMenu::LimboMenu(Handle<ILimboMenuHost> host, client::IRenderer& renderer,
							 client::FontManager& fontManager, SoftwareCursor& cursor)
		    : host(host), renderer(&renderer), fontManager(&fontManager), cursor(cursor) {}

		LimboMenu::~LimboMenu() {}

		void LimboMenu::Open() {
			if (!host)
				return;

			prevSelectedItem = -1;
			items.clear();

			RecalculateLayout();

			const AABB2 empty = AABB2(0.0F, 0.0F, 0.0F, 0.0F);
			items.push_back(MenuItem(MenuItemType::Team1, empty, host->GetTeamName(0)));
			items.push_back(MenuItem(MenuItemType::Team2, empty, host->GetTeamName(1)));
			items.push_back(MenuItem(MenuItemType::TeamSpectator, empty, _Tr("Client", "Spectator")));
			items.push_back(MenuItem(MenuItemType::WeaponRifle, empty, _Tr("Client", "Rifle")));
			items.push_back(MenuItem(MenuItemType::WeaponSMG, empty, _Tr("Client", "SMG")));
			items.push_back(MenuItem(MenuItemType::WeaponShotgun, empty, _Tr("Client", "Shotgun")));
			items.push_back(MenuItem(MenuItemType::Spawn, empty, _Tr("Client", "Spawn")));
			items.push_back(MenuItem(MenuItemType::Close, empty, "X"));

			// Rects, visibility and hover are always derived, never duplicated here:
			// the same passes that keep the menu right every frame also seed it, so
			// the frame it opens on draws exactly what any later frame would.
			RecalculateItemRects();
			UpdateItemVisibility();
			UpdateHover();

			// Last, so the menu is never observably active while half-built.
			isActive = true;
		}

		void LimboMenu::Close() {
			isActive = false;
			items.clear();
		}

		void LimboMenu::RecalculateLayout() {
			cachedLayout.screenWidth = renderer->ScreenWidth();
			cachedLayout.screenHeight = renderer->ScreenHeight();

			float maxContentsWidth = 860.0F;
			cachedLayout.contentsWidth = std::min(maxContentsWidth, cachedLayout.screenWidth - 16.0F);

			cachedLayout.left = (cachedLayout.screenWidth - cachedLayout.contentsWidth) * 0.5F;
			float panelH = 205.0F;
			cachedLayout.top = cachedLayout.screenHeight - (panelH + 15.0F);

			cachedLayout.menuWidth = 195.0F;
			cachedLayout.menuHeight = 34.0F;
			cachedLayout.rowHeight = 40.0F;

			cachedLayout.teamX = cachedLayout.left + 16.0F;
			cachedLayout.weapX = cachedLayout.left + 225.0F;
			cachedLayout.firstY = cachedLayout.top + 62.0F;

			// Right showcase frame for 3D weapon preview
			float showcaseX = cachedLayout.left + 435.0F;
			float showcaseW = cachedLayout.contentsWidth - (435.0F + 16.0F);
			float showcaseH = 185.0F;
			showcaseRect = AABB2(showcaseX, cachedLayout.top + 10.0F, showcaseW, showcaseH);

			cachedLayout.isValid = true;
		}

		void LimboMenu::EnsureLayout() {
			if (cachedLayout.isValid && cachedLayout.screenWidth == renderer->ScreenWidth() &&
				cachedLayout.screenHeight == renderer->ScreenHeight())
				return;

			RecalculateLayout();
			RecalculateItemRects();
		}

		void LimboMenu::UpdateItemVisibility() {
			const int selectedTeam = host->GetSelectedTeam();
			for (MenuItem& item : items) {
				switch (item.type) {
					case MenuItemType::WeaponRifle:
					case MenuItemType::WeaponSMG:
					case MenuItemType::WeaponShotgun:
						// No weapon to pick while spectating.
						item.visible = selectedTeam < 2;
						break;
					case MenuItemType::Close:
						// Nothing to go back to until the first spawn.
						item.visible = host->HasLocalPlayer();
						break;
					default: item.visible = true;
				}
			}
		}

		void LimboMenu::RecalculateItemRects() {
			for (size_t i = 0; i < items.size(); i++) {
				MenuItem& item = items[i];
				switch (item.type) {
					case MenuItemType::Team1:
						item.rect = AABB2(cachedLayout.teamX, cachedLayout.firstY, cachedLayout.menuWidth, cachedLayout.menuHeight);
						break;
					case MenuItemType::Team2:
						item.rect = AABB2(cachedLayout.teamX, cachedLayout.firstY + cachedLayout.rowHeight, cachedLayout.menuWidth, cachedLayout.menuHeight);
						break;
					case MenuItemType::TeamSpectator:
						item.rect = AABB2(cachedLayout.teamX, cachedLayout.firstY + cachedLayout.rowHeight * 2.0F, cachedLayout.menuWidth, cachedLayout.menuHeight);
						break;
					case MenuItemType::WeaponRifle:
						item.rect = AABB2(cachedLayout.weapX, cachedLayout.firstY, cachedLayout.menuWidth, cachedLayout.menuHeight);
						break;
					case MenuItemType::WeaponSMG:
						item.rect = AABB2(cachedLayout.weapX, cachedLayout.firstY + cachedLayout.rowHeight, cachedLayout.menuWidth, cachedLayout.menuHeight);
						break;
					case MenuItemType::WeaponShotgun:
						item.rect = AABB2(cachedLayout.weapX, cachedLayout.firstY + cachedLayout.rowHeight * 2.0F, cachedLayout.menuWidth, cachedLayout.menuHeight);
						break;
					case MenuItemType::Spawn:
						item.rect = AABB2(showcaseRect.min.x + showcaseRect.GetWidth() - 180.0F, showcaseRect.min.y + showcaseRect.GetHeight() - 36.0F, 172.0F, 30.0F);
						break;
					case MenuItemType::Close:
						item.rect = AABB2(cachedLayout.left + cachedLayout.contentsWidth - 28.0F, cachedLayout.top + 8.0F, 20.0F, 20.0F);
						break;
				}
			}
		}

		bool LimboMenu::KeyEvent(const std::string& key, bool down) {
			if (!isActive)
				return false;

			if (!down)
				return false;

			if (key == "LeftMouseButton") {
				// Clicks arrive during event dispatch, ahead of the frame's Update, so
				// a resize or an earlier click in the same batch could otherwise be
				// hit-tested against rects and visibility that no longer hold.
				EnsureLayout();
				UpdateItemVisibility();

				int idx = MenuButtonAt(cursor.GetPosition());
				if (idx >= 0 && idx < static_cast<int>(items.size())) {
					host->PlaySelectSound();
					HandleMenuItemSelection(items[idx].type);
				}
				return true;
			}

			// Keyboard shortcuts
			if (key == "1") {
				if (host->GetSelectedTeam() >= 2) {
					host->OnTeamSelected(0);
				} else {
					host->OnWeaponSelected(RIFLE_WEAPON);
					host->OnSpawnPressed();
				}
				host->PlaySelectSound();
				return true;
			} else if (key == "2") {
				if (host->GetSelectedTeam() >= 2) {
					host->OnTeamSelected(1);
				} else {
					host->OnWeaponSelected(SMG_WEAPON);
					host->OnSpawnPressed();
				}
				host->PlaySelectSound();
				return true;
			} else if (key == "3") {
				if (host->GetSelectedTeam() < 2) {
					host->OnWeaponSelected(SHOTGUN_WEAPON);
				}
				host->PlaySelectSound();
				host->OnSpawnPressed();
				return true;
			} else if (key == "Return" || key == "Enter" || key == "Space") {
				host->PlaySelectSound();
				host->OnSpawnPressed();
				return true;
			} else if (key == "Escape") {
				// Dismissing on Escape is this menu's own contract
				host->PlaySelectSound();
				host->OnClosePressed();
				return true;
			}

			return false;
		}

		void LimboMenu::TextInputEvent(const std::string& text) {}

		void LimboMenu::Render3DWeapon(const client::SceneDefinition& sceneDef) {
			if (!isActive)
				return;

			if (!rifleModel) rifleModel = renderer->RegisterModel("Models/Weapons/Rifle/Weapon.kv6");
			if (!smgModel) smgModel = renderer->RegisterModel("Models/Weapons/SMG/Weapon.kv6");
			if (!shotgunModel) shotgunModel = renderer->RegisterModel("Models/Weapons/Shotgun/Weapon.kv6");

			WeaponType selectedWeapon = host->GetSelectedWeapon();
			Handle<client::IModel> curModel;
			Vector3 lightColor;
			float weaponScale = 0.024F;

			switch (selectedWeapon) {
				case RIFLE_WEAPON:
					curModel = rifleModel;
					lightColor = MakeVector3(0.65F, 0.85F, 1.0F); // Crisp ice blue
					weaponScale = 0.022F;
					break;
				case SMG_WEAPON:
					curModel = smgModel;
					lightColor = MakeVector3(0.40F, 0.95F, 0.65F); // Emerald cyan
					weaponScale = 0.024F;
					break;
				case SHOTGUN_WEAPON:
					curModel = shotgunModel;
					lightColor = MakeVector3(1.0F, 0.70F, 0.35F); // Amber orange
					weaponScale = 0.023F;
					break;
				default:
					curModel = rifleModel;
					lightColor = MakeVector3(0.85F, 0.85F, 0.85F);
					break;
			}

			if (!curModel)
				return;

			float sw = renderer->ScreenWidth();
			float sh = renderer->ScreenHeight();

			float centerX = showcaseRect.min.x + showcaseRect.GetWidth() * 0.5F;
			float centerY = showcaseRect.min.y + 60.0F;

			float ndcX = (centerX / sw) * 2.0F - 1.0F;
			float ndcY = 1.0F - (centerY / sh) * 2.0F;

			float dist = 3.6F;
			float tanHalfFovX = (sceneDef.fovX > 0.01F) ? tanf(sceneDef.fovX * 0.5F) : (tanf(DEG2RAD(35.0F)) * (sw / sh));
			float tanHalfFovY = (sceneDef.fovY > 0.01F) ? tanf(sceneDef.fovY * 0.5F) : tanf(DEG2RAD(35.0F));

			float rightOffset = ndcX * tanHalfFovX * dist;
			float upOffset = ndcY * tanHalfFovY * dist;

			Vector3 orig = sceneDef.viewOrigin;
			Vector3 dir = sceneDef.viewAxis[2];
			Vector3 side = sceneDef.viewAxis[0];
			Vector3 up = sceneDef.viewAxis[1];

			Vector3 weaponPos = orig + dir * dist + side * rightOffset + up * upOffset;

			float bob = sinf(showcaseTime * 2.2F) * 0.025F;
			float pitch = sinf(showcaseTime * 1.6F) * 0.05F - 0.03F;
			Vector3 actualPos = weaponPos + up * bob;

			client::ModelRenderParam param;
			param.matrix = Matrix4::Translate(actualPos)
			             * Matrix4::Rotate(up, weaponYaw)
			             * Matrix4::Rotate(side, pitch)
			             * Matrix4::Scale(weaponScale);

			client::DynamicLightParam light;
			light.type = client::DynamicLightTypePoint;
			light.origin = actualPos + up * 0.75F - dir * 0.9F + side * 0.2F;
			light.radius = 4.8F;
			light.color = lightColor;
			renderer->AddLight(light);

			renderer->RenderModel(*curModel, param);
		}

		void LimboMenu::Draw() {
			if (!isActive)
				return;

			EnsureLayout();

			client::IFont& font = fontManager->GetGuiFont();
			client::IFont& headingFont = fontManager->GetHeadingFont();
			client::IFont& smallFont = fontManager->GetSmallFont();

			const float left = cachedLayout.left;
			const float top = cachedLayout.top;
			const float contentsWidth = cachedLayout.contentsWidth;
			const float height = 205.0F;

			const int selectedTeam = host->GetSelectedTeam();
			const WeaponType selectedWeapon = host->GetSelectedWeapon();

			// 1. Sleek glass panel background in HD translucent dark
			renderer->SetColorAlphaPremultiplied(MakeVector4(0.02F, 0.02F, 0.04F, 0.88F));
			renderer->DrawFilledRect(left, top, left + contentsWidth, top + height);

			// Border with subtle tactical glow
			renderer->SetColorAlphaPremultiplied(MakeVector4(0.18F, 0.24F, 0.35F, 0.90F));
			renderer->DrawOutlinedRect(left, top, left + contentsWidth, top + height);

			// Accent neon line on top edge
			renderer->SetColorAlphaPremultiplied(MakeVector4(0.35F, 0.75F, 1.0F, 0.95F));
			renderer->DrawLine(MakeVector2(left + 2.0F, top + 1.0F), MakeVector2(left + contentsWidth - 2.0F, top + 1.0F), 2.0F);

			Vector4 color = MakeVector4(1, 1, 1, 1);
			Vector4 shadowColor = MakeVector4(0, 0, 0, 0.55F);
			Vector4 hotkeyColor = MakeVector4(0.75F, 0.85F, 0.95F, 0.80F);

			// Header Title
			headingFont.DrawShadow(_Tr("Client", "DESPLIEGUE TÁCTICO & SELECCIÓN DE CLASE"), MakeVector2(left + 16.0F, top + 10.0F), 1.0F, color, shadowColor);
			smallFont.DrawShadow(_Tr("Client", "Selecciona tu bando y equipamiento táctico • [1, 2, 3] Selección rápida"), MakeVector2(left + 16.0F, top + 31.0F), 0.95F, hotkeyColor, shadowColor);

			// Section Column Titles
			font.DrawShadow(_Tr("Client", "1. EQUIPO"), MakeVector2(cachedLayout.teamX, top + 44.0F), 0.95F, MakeVector4(0.55F, 0.85F, 1.0F, 0.95F), shadowColor);
			if (selectedTeam < 2) {
				font.DrawShadow(_Tr("Client", "2. ARMA PRINCIPAL"), MakeVector2(cachedLayout.weapX, top + 44.0F), 0.95F, MakeVector4(0.55F, 0.85F, 1.0F, 0.95F), shadowColor);
			}

			// Render Team & Weapon Buttons
			for (const auto& item : items) {
				if (!item.visible)
					continue;

				bool selected = false;
				int index = 0;
				Vector4 accentCol = MakeVector4(0.4F, 0.6F, 0.8F, 1.0F);

				switch (item.type) {
					case MenuItemType::Team1:
					case MenuItemType::Team2:
					case MenuItemType::TeamSpectator: {
						int teamId = TeamIdOf(item.type);
						selected = (selectedTeam == teamId);
						index = (selectedTeam >= 2) ? (1 + teamId) : 0;
						if (teamId == 0) accentCol = MakeVector4(0.25F, 0.75F, 0.45F, 1.0F);      // Green Team
						else if (teamId == 1) accentCol = MakeVector4(0.25F, 0.55F, 0.95F, 1.0F); // Blue Team
						else accentCol = MakeVector4(0.65F, 0.70F, 0.80F, 1.0F);                  // Spectator
						break;
					}
					case MenuItemType::WeaponRifle:
					case MenuItemType::WeaponSMG:
					case MenuItemType::WeaponShotgun: {
						WeaponType weapon = WeaponOf(item.type);
						selected = (selectedWeapon == weapon);
						index = (selectedTeam < 2) ? (1 + (int)weapon) : 0;
						if (weapon == RIFLE_WEAPON) accentCol = MakeVector4(0.55F, 0.80F, 1.0F, 1.0F);
						else if (weapon == SMG_WEAPON) accentCol = MakeVector4(0.35F, 0.95F, 0.65F, 1.0F);
						else accentCol = MakeVector4(1.0F, 0.65F, 0.30F, 1.0F);
						break;
					}
					default: selected = false;
				}

				if (item.type == MenuItemType::Spawn) {
					// Tactical big Spawn button
					Vector4 bgCol = item.hover ? MakeVector4(0.20F, 0.55F, 0.35F, 0.95F)
					                           : MakeVector4(0.12F, 0.38F, 0.22F, 0.85F);
					renderer->SetColorAlphaPremultiplied(bgCol);
					renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

					Vector4 borderCol = item.hover ? MakeVector4(0.50F, 1.0F, 0.65F, 1.0F)
					                               : MakeVector4(0.30F, 0.85F, 0.45F, 0.90F);
					renderer->SetColorAlphaPremultiplied(borderCol);
					renderer->DrawOutlinedRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

					std::string spawnTxt = _Tr("Client", "DESPLEGARSE [ESPACIO]");
					Vector2 txtSz = font.Measure(spawnTxt);
					Vector2 txtPos = item.rect.min + MakeVector2((item.rect.GetWidth() - txtSz.x) * 0.5F, (item.rect.GetHeight() - txtSz.y) * 0.5F);
					font.DrawShadow(spawnTxt, txtPos, 1.0F, MakeVector4(1, 1, 1, 1), shadowColor);
					continue;
				} else if (item.type == MenuItemType::Close) {
					Vector4 bgCol = item.hover ? MakeVector4(0.6F, 0.15F, 0.15F, 0.90F)
					                           : MakeVector4(0.25F, 0.08F, 0.08F, 0.65F);
					renderer->SetColorAlphaPremultiplied(bgCol);
					renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);
					renderer->SetColorAlphaPremultiplied(MakeVector4(0.8F, 0.3F, 0.3F, 0.85F));
					renderer->DrawOutlinedRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

					Vector2 sz = font.Measure("X");
					Vector2 pos = item.rect.min + MakeVector2((item.rect.GetWidth() - sz.x) * 0.5F, (item.rect.GetHeight() - sz.y) * 0.5F);
					font.DrawShadow("X", pos, 1.0F, MakeVector4(1, 1, 1, 1), shadowColor);
					continue;
				}

				// Standard button card
				Vector4 fillColor = MakeVector4(0.06F, 0.07F, 0.10F, 0.70F);
				Vector4 borderCol = MakeVector4(0.18F, 0.22F, 0.30F, 0.75F);

				if (selected) {
					fillColor = MakeVector4(accentCol.x * 0.30F, accentCol.y * 0.30F, accentCol.z * 0.30F, 0.85F);
					borderCol = accentCol;
				} else if (item.hover) {
					fillColor = MakeVector4(0.14F, 0.16F, 0.22F, 0.85F);
					borderCol = MakeVector4(0.40F, 0.55F, 0.75F, 0.85F);
				}

				renderer->SetColorAlphaPremultiplied(fillColor);
				renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

				renderer->SetColorAlphaPremultiplied(borderCol);
				renderer->DrawOutlinedRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

				// Left colored indicator bar
				renderer->SetColorAlphaPremultiplied(accentCol);
				renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.min.x + (selected ? 4.0F : 2.5F), item.rect.max.y);

				// Button Label
				std::string str = item.text;
				if (item.type == MenuItemType::Team1) str = host->GetTeamName(0);
				else if (item.type == MenuItemType::Team2) str = host->GetTeamName(1);

				Vector2 size = font.Measure(str);
				Vector2 pos = MakeVector2(item.rect.min.x + 10.0F, item.rect.min.y + (item.rect.GetHeight() - size.y) * 0.5F);
				font.DrawShadow(str, pos, 1.0F, selected ? MakeVector4(1, 1, 1, 1) : MakeVector4(0.9F, 0.92F, 0.95F, 0.95F), shadowColor);

				// Hotkey hint [1], [2], [3]
				if (index > 0) {
					std::string hk = Format("[{0}]", index);
					Vector2 hkPos = MakeVector2(item.rect.max.x - font.Measure(hk).x - 8.0F, pos.y);
					font.DrawShadow(hk, hkPos, 1.0F, selected ? MakeVector4(1, 1, 1, 1) : hotkeyColor, shadowColor);
				}
			}

			// 2. 3D Showcase Viewport Box & Real-Time Stats (Right Column)
			if (selectedTeam < 2) {
				// Glass container for rotating 3D weapon
				renderer->SetColorAlphaPremultiplied(MakeVector4(0.01F, 0.015F, 0.03F, 0.75F));
				renderer->DrawFilledRect(showcaseRect.min.x, showcaseRect.min.y, showcaseRect.max.x, showcaseRect.max.y);

				renderer->SetColorAlphaPremultiplied(MakeVector4(0.20F, 0.35F, 0.50F, 0.85F));
				renderer->DrawOutlinedRect(showcaseRect.min.x, showcaseRect.min.y, showcaseRect.max.x, showcaseRect.max.y);

				// Reticle corners in cyan
				float rSz = 8.0F;
				renderer->SetColorAlphaPremultiplied(MakeVector4(0.35F, 0.85F, 1.0F, 0.90F));
				// Top-left
				renderer->DrawLine(MakeVector2(showcaseRect.min.x + 3.0F, showcaseRect.min.y + 3.0F), MakeVector2(showcaseRect.min.x + 3.0F + rSz, showcaseRect.min.y + 3.0F), 1.5F);
				renderer->DrawLine(MakeVector2(showcaseRect.min.x + 3.0F, showcaseRect.min.y + 3.0F), MakeVector2(showcaseRect.min.x + 3.0F, showcaseRect.min.y + 3.0F + rSz), 1.5F);
				// Top-right
				renderer->DrawLine(MakeVector2(showcaseRect.max.x - 3.0F, showcaseRect.min.y + 3.0F), MakeVector2(showcaseRect.max.x - 3.0F - rSz, showcaseRect.min.y + 3.0F), 1.5F);
				renderer->DrawLine(MakeVector2(showcaseRect.max.x - 3.0F, showcaseRect.min.y + 3.0F), MakeVector2(showcaseRect.max.x - 3.0F, showcaseRect.min.y + 3.0F + rSz), 1.5F);
				// Bottom-left
				renderer->DrawLine(MakeVector2(showcaseRect.min.x + 3.0F, showcaseRect.max.y - 3.0F), MakeVector2(showcaseRect.min.x + 3.0F + rSz, showcaseRect.max.y - 3.0F), 1.5F);
				renderer->DrawLine(MakeVector2(showcaseRect.min.x + 3.0F, showcaseRect.max.y - 3.0F), MakeVector2(showcaseRect.min.x + 3.0F, showcaseRect.max.y - 3.0F - rSz), 1.5F);
				// Bottom-right
				renderer->DrawLine(MakeVector2(showcaseRect.max.x - 3.0F, showcaseRect.max.y - 3.0F), MakeVector2(showcaseRect.max.x - 3.0F - rSz, showcaseRect.max.y - 3.0F), 1.5F);
				renderer->DrawLine(MakeVector2(showcaseRect.max.x - 3.0F, showcaseRect.max.y - 3.0F), MakeVector2(showcaseRect.max.x - 3.0F, showcaseRect.max.y - 3.0F - rSz), 1.5F);

				// Real-time blinking status indicator
				float blink = 0.55F + sinf(showcaseTime * 4.0F) * 0.40F;
				smallFont.DrawShadow("● VISTA PREVIA 3D EN TIEMPO REAL (FÍSICO)", MakeVector2(showcaseRect.min.x + 12.0F, showcaseRect.min.y + 6.0F), 0.88F, MakeVector4(0.35F, 1.0F, 0.55F, blink), shadowColor);

				// Weapon specifications and description below the 3D rotating model
				std::string wName = "RIFLE DE PRECISIÓN SEMI-AUTOMÁTICO";
				std::string wStats = "Daño: 85 • RPM: 30 • Precisión: 95% • Alcance: 98m • Movilidad: 65%";
				if (selectedWeapon == SMG_WEAPON) {
					wName = "SUBFUSIL SMG AUTOMÁTICO";
					wStats = "Daño: 42 • RPM: 92 • Precisión: 70% • Alcance: 60m • Movilidad: 90%";
				} else if (selectedWeapon == SHOTGUN_WEAPON) {
					wName = "ESCOPETA DE COMBATE TÁCTICA";
					wStats = "Daño: 96 • RPM: 40 • Precisión: 45% • Alcance: 40m • Movilidad: 75%";
				}

				font.DrawShadow(wName, MakeVector2(showcaseRect.min.x + 12.0F, showcaseRect.min.y + 110.0F), 1.0F, MakeVector4(1.0F, 0.95F, 0.75F, 1.0F), shadowColor);
				smallFont.DrawShadow(wStats, MakeVector2(showcaseRect.min.x + 12.0F, showcaseRect.min.y + 130.0F), 0.88F, MakeVector4(0.85F, 0.90F, 0.95F, 0.90F), shadowColor);
			}

			// draw cursor
			cursor.Draw();
		}

		void LimboMenu::Update(float dt) {
			if (!isActive)
				return;

			showcaseTime += dt;
			weaponYaw += dt * 1.5F; // smooth continuous physical 360 degree rotation

			// Hit-testing has to see the rects and visibility this frame will draw.
			EnsureLayout();
			UpdateItemVisibility();
			UpdateHover();
		}

		void LimboMenu::UpdateHover() {
			for (MenuItem& item : items)
				item.hover = false;

			int hoveredIndex = MenuButtonAt(cursor.GetPosition());
			if (hoveredIndex < 0) {
				prevSelectedItem = -1;
				return;
			}

			items[hoveredIndex].hover = true;
			if (hoveredIndex != prevSelectedItem) {
				host->PlayHoverSound();
				prevSelectedItem = hoveredIndex;
			}
		}

		void LimboMenu::MouseEvent(float x, float y) {
			if (isActive)
				cursor.SetPosition(MakeVector2(x, y));
		}

		int LimboMenu::MenuButtonAt(const Vector2& p) const {
			for (size_t i = 0; i < items.size(); i++) {
				const MenuItem& item = items[i];
				if (!item.visible)
					continue;
				if (item.rect && p) // half-open, as everywhere else in the UI
					return static_cast<int>(i);
			}
			return -1;
		}

		void LimboMenu::HandleMenuItemSelection(MenuItemType type) {
			switch (type) {
				case MenuItemType::Team1:
					host->OnTeamSelected(0);
					break;
				case MenuItemType::Team2:
					host->OnTeamSelected(1);
					break;
				case MenuItemType::TeamSpectator:
					host->OnTeamSelected(2);
					break;
				case MenuItemType::WeaponRifle:
					host->OnWeaponSelected(RIFLE_WEAPON);
					break;
				case MenuItemType::WeaponSMG:
					host->OnWeaponSelected(SMG_WEAPON);
					break;
				case MenuItemType::WeaponShotgun:
					host->OnWeaponSelected(SHOTGUN_WEAPON);
					break;
				case MenuItemType::Spawn:
					host->OnSpawnPressed();
					break;
				case MenuItemType::Close:
					host->OnClosePressed();
					break;
			}
		}

	} // namespace gui
} // namespace spades
