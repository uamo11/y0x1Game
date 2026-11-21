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

			// Center selection panel: 460px wide, 168px high
			cachedLayout.contentsWidth = std::min(460.0F, cachedLayout.screenWidth - 16.0F);
			cachedLayout.left = (cachedLayout.screenWidth - cachedLayout.contentsWidth) * 0.5F;
			float panelH = 168.0F;
			cachedLayout.top = cachedLayout.screenHeight - (panelH + 20.0F);

			cachedLayout.menuWidth = (cachedLayout.contentsWidth - 36.0F) * 0.5F;
			cachedLayout.menuHeight = 28.0F;
			cachedLayout.rowHeight = 33.0F;

			cachedLayout.teamX = cachedLayout.left + 14.0F;
			cachedLayout.weapX = cachedLayout.left + 14.0F + cachedLayout.menuWidth + 8.0F;
			cachedLayout.firstY = cachedLayout.top + 50.0F;

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
						item.rect = AABB2(cachedLayout.left + 14.0F, cachedLayout.top + 130.0F, cachedLayout.contentsWidth - 28.0F, 28.0F);
						break;
					case MenuItemType::Close:
						item.rect = AABB2(cachedLayout.left + cachedLayout.contentsWidth - 26.0F, cachedLayout.top + 6.0F, 20.0F, 20.0F);
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

		void LimboMenu::Render3DScene(const client::SceneDefinition& sceneDef) {
			if (!isActive)
				return;

			if (!rifleModel) rifleModel = renderer->RegisterModel("Models/Weapons/Rifle/Weapon.kv6");
			if (!smgModel) smgModel = renderer->RegisterModel("Models/Weapons/SMG/Weapon.kv6");
			if (!shotgunModel) shotgunModel = renderer->RegisterModel("Models/Weapons/Shotgun/Weapon.kv6");
			if (!playerTorsoModel) playerTorsoModel = renderer->RegisterModel("Models/Player/Torso.kv6");
			if (!playerHeadModel) playerHeadModel = renderer->RegisterModel("Models/Player/Head.kv6");
			if (!playerLegLeftModel) playerLegLeftModel = renderer->RegisterModel("Models/Player/Leg.kv6");
			if (!playerLegRightModel) playerLegRightModel = renderer->RegisterModel("Models/Player/LegRight.kv6");
			if (!playerArmsModel) playerArmsModel = renderer->RegisterModel("Models/Player/Arms.kv6");

			float sw = renderer->ScreenWidth();
			float sh = renderer->ScreenHeight();

			float tanHalfFovX = (sceneDef.fovX > 0.01F) ? tanf(sceneDef.fovX * 0.5F) : (tanf(DEG2RAD(35.0F)) * (sw / sh));
			float tanHalfFovY = (sceneDef.fovY > 0.01F) ? tanf(sceneDef.fovY * 0.5F) : tanf(DEG2RAD(35.0F));

			Vector3 orig = sceneDef.viewOrigin;
			Vector3 dir = sceneDef.viewAxis[2];
			Vector3 side = sceneDef.viewAxis[0];
			Vector3 up = sceneDef.viewAxis[1];

			int selectedTeam = host->GetSelectedTeam();
			WeaponType selectedWeapon = host->GetSelectedWeapon();

			// 1. LEFT SIDE: 3D Player Character (Soldier with Team Skin)
			// Perfectly upright, facing the player with slight 3/4 turn towards center
			{
				float charNdcX = -0.60F;
				float charNdcY = -0.52F;
				float charDist = 3.6F;

				float charRight = charNdcX * tanHalfFovX * charDist;
				float charUp = charNdcY * tanHalfFovY * charDist;
				Vector3 charPos = orig + dir * charDist + side * charRight + up * charUp;

				Vector3 teamColor = MakeVector3(0.18F, 0.50F, 0.95F); // Blue uniform
				if (selectedTeam == 1) {
					teamColor = MakeVector3(0.95F, 0.22F, 0.22F); // Red uniform
				} else if (selectedTeam >= 2) {
					teamColor = MakeVector3(0.60F, 0.65F, 0.70F); // Spectator / Neutral
				}

				client::DynamicLightParam charLight;
				charLight.type = client::DynamicLightTypePoint;
				charLight.origin = charPos + up * 1.5F - dir * 1.0F - side * 0.3F;
				charLight.radius = 6.0F;
				charLight.color = teamColor;
				renderer->AddLight(charLight);

				float breath = sinf(showcaseTime * 2.2F);
				float pitch = 0.04F + breath * 0.02F;
				float armPitch = -0.45F + breath * 0.02F;

				// Facing towards camera (-dir) and turned slightly right towards center (+side)
				Vector3 facing = (-dir * 0.88F + side * 0.45F).Normalize();
				Vector3 rgt = Vector3::Cross(facing, up).Normalize();

				// Basis: -rgt * (-1 from scaler) = +rgt (Right)
				//        -facing * (-1 from scaler) = +facing (Front, towards player)
				//        -up * (+1 from scaler) = -up (Down, so head at -1.0 is UP!)
				Matrix4 lower = Matrix4::FromAxis(-rgt, -facing, -up, charPos);
				Matrix4 scaler = Matrix4::Scale(0.080F) * Matrix4::Scale(-1, -1, 1);

				client::ModelRenderParam charParam;
				charParam.depthHack = true;
				charParam.customColor = teamColor;

				if (playerLegLeftModel) {
					charParam.matrix = lower * Matrix4::Translate(-0.25F, 0.0F, -0.1F) * scaler;
					renderer->RenderModel(*playerLegLeftModel, charParam);
				}
				if (playerLegRightModel) {
					charParam.matrix = lower * Matrix4::Translate(0.25F, 0.0F, -0.1F) * scaler;
					renderer->RenderModel(*playerLegRightModel, charParam);
				}
				Matrix4 torso = lower * Matrix4::Translate(0.0F, 0.0F, -1.0F + breath * 0.015F)
				              * Matrix4::Rotate(MakeVector3(1, 0, 0), breath * 0.02F);
				charParam.matrix = torso * scaler;
				if (playerTorsoModel)
					renderer->RenderModel(*playerTorsoModel, charParam);

				if (playerHeadModel) {
					charParam.matrix = torso * Matrix4::Translate(0.0F, 0.0F, 0.0F)
					                 * Matrix4::Rotate(MakeVector3(1, 0, 0), pitch) * scaler;
					renderer->RenderModel(*playerHeadModel, charParam);
				}
				if (playerArmsModel) {
					charParam.matrix = torso * Matrix4::Translate(0.0F, 0.0F, 0.1F)
					                 * Matrix4::Rotate(MakeVector3(1, 0, 0), armPitch) * scaler;
					renderer->RenderModel(*playerArmsModel, charParam);
				}
			}

			// 2. RIGHT SIDE: 3D Weapon Model Physically Rotating 360°
			// Completely upright: scope/sights on TOP, grip/magazine on BOTTOM
			if (selectedTeam < 2) {
				Handle<client::IModel> curModel;
				Vector3 lightColor;
				float weaponScale = 0.025F;

				switch (selectedWeapon) {
					case RIFLE_WEAPON:
						curModel = rifleModel;
						lightColor = MakeVector3(0.65F, 0.85F, 1.0F); // Crisp ice blue
						weaponScale = 0.023F;
						break;
					case SMG_WEAPON:
						curModel = smgModel;
						lightColor = MakeVector3(0.40F, 0.95F, 0.65F); // Emerald cyan
						weaponScale = 0.025F;
						break;
					case SHOTGUN_WEAPON:
						curModel = shotgunModel;
						lightColor = MakeVector3(1.0F, 0.70F, 0.35F); // Amber orange
						weaponScale = 0.024F;
						break;
					default:
						curModel = rifleModel;
						lightColor = MakeVector3(0.85F, 0.85F, 0.85F);
						break;
				}

				if (curModel) {
					float weapNdcX = 0.60F;
					float weapNdcY = -0.52F;
					float weapDist = 3.6F;

					float weapRight = weapNdcX * tanHalfFovX * weapDist;
					float weapUp = weapNdcY * tanHalfFovY * weapDist;
					Vector3 weapPos = orig + dir * weapDist + side * weapRight + up * weapUp;

					float bob = sinf(showcaseTime * 2.2F) * 0.02F;
					Vector3 actualPos = weapPos + up * bob;

					client::DynamicLightParam weapLight;
					weapLight.type = client::DynamicLightTypePoint;
					weapLight.origin = actualPos + up * 0.75F - dir * 0.9F + side * 0.2F;
					weapLight.radius = 5.0F;
					weapLight.color = lightColor;
					renderer->AddLight(weapLight);

					// Smooth 360 physical rotation around Screen UP
					Vector3 rotFwd = (side * cosf(weaponYaw) - dir * sinf(weaponYaw)).Normalize();
					Vector3 rotRight = (side * sinf(weaponYaw) + dir * cosf(weaponYaw)).Normalize();

					// In KV6 weapon:
					// +Y = Barrel (forward)
					// +Z = Grip / Magazine (downward)
					// -Z = Sight / Scope / Rail (upward)
					// +X = Weapon Right Side
					Vector3 weapBarrel = (rotFwd + up * 0.14F).Normalize();
					Vector3 weapDown = (-up + rotFwd * 0.14F).Normalize();
					Vector3 weapRight = rotRight;

					Matrix4 weapMatrix = Matrix4::FromAxis(weapRight, weapBarrel, weapDown, actualPos);

					client::ModelRenderParam weapParam;
					weapParam.depthHack = true;
					weapParam.matrix = weapMatrix * Matrix4::Scale(weaponScale);

					renderer->RenderModel(*curModel, weapParam);
				}
			}
		}

		void LimboMenu::Draw() {
			if (!isActive)
				return;

			EnsureLayout();

			client::IFont& font = fontManager->GetGuiFont();
			client::IFont& smallFont = fontManager->GetSmallFont();

			const float sw = cachedLayout.screenWidth;
			const float sh = cachedLayout.screenHeight;
			const float left = cachedLayout.left;
			const float top = cachedLayout.top;
			const float contentsWidth = cachedLayout.contentsWidth;
			const float height = 168.0F;

			const int selectedTeam = host->GetSelectedTeam();
			const WeaponType selectedWeapon = host->GetSelectedWeapon();

			Vector4 shadowColor = MakeVector4(0, 0, 0, 0.65F);
			Vector4 hotkeyColor = MakeVector4(0.75F, 0.85F, 0.95F, 0.80F);

			// Center selection card (clean, semi-transparent background)
			renderer->SetColorAlphaPremultiplied(MakeVector4(0.02F, 0.03F, 0.05F, 0.82F));
			renderer->DrawFilledRect(left, top, left + contentsWidth, top + height);

			renderer->SetColorAlphaPremultiplied(MakeVector4(0.20F, 0.30F, 0.45F, 0.85F));
			renderer->DrawOutlinedRect(left, top, left + contentsWidth, top + height);

			// Top neon line
			renderer->SetColorAlphaPremultiplied(MakeVector4(0.35F, 0.75F, 1.0F, 0.95F));
			renderer->DrawLine(MakeVector2(left + 2.0F, top + 1.0F), MakeVector2(left + contentsWidth - 2.0F, top + 1.0F), 2.0F);

			// Center Title (Clean, NO overlap!)
			font.DrawShadow(_Tr("Client", "SELECCIONAR EQUIPO Y ARMA"), MakeVector2(left + 14.0F, top + 10.0F), 1.0F, MakeVector4(1, 1, 1, 1), shadowColor);

			// Section Column Titles
			smallFont.DrawShadow(_Tr("Client", "1. EQUIPO"), MakeVector2(cachedLayout.teamX, top + 32.0F), 0.95F, MakeVector4(0.55F, 0.85F, 1.0F, 0.95F), shadowColor);
			if (selectedTeam < 2) {
				smallFont.DrawShadow(_Tr("Client", "2. ARMA PRINCIPAL"), MakeVector2(cachedLayout.weapX, top + 32.0F), 0.95F, MakeVector4(0.55F, 0.85F, 1.0F, 0.95F), shadowColor);
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
						if (teamId == 0) accentCol = MakeVector4(0.25F, 0.55F, 0.95F, 1.0F);      // Blue Team
						else if (teamId == 1) accentCol = MakeVector4(0.95F, 0.30F, 0.30F, 1.0F); // Red Team
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
					Vector4 bgCol = item.hover ? MakeVector4(0.18F, 0.50F, 0.32F, 0.95F)
					                           : MakeVector4(0.10F, 0.32F, 0.20F, 0.85F);
					renderer->SetColorAlphaPremultiplied(bgCol);
					renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

					Vector4 borderCol = item.hover ? MakeVector4(0.45F, 0.95F, 0.60F, 1.0F)
					                               : MakeVector4(0.25F, 0.75F, 0.40F, 0.90F);
					renderer->SetColorAlphaPremultiplied(borderCol);
					renderer->DrawOutlinedRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

					std::string spawnTxt = _Tr("Client", "DESPLEGARSE [ESPACIO]");
					Vector2 txtSz = font.Measure(spawnTxt);
					Vector2 txtPos = item.rect.min + MakeVector2((item.rect.GetWidth() - txtSz.x) * 0.5F, (item.rect.GetHeight() - txtSz.y) * 0.5F);
					font.DrawShadow(spawnTxt, txtPos, 1.0F, MakeVector4(1, 1, 1, 1), shadowColor);
					continue;
				} else if (item.type == MenuItemType::Close) {
					Vector4 bgCol = item.hover ? MakeVector4(0.5F, 0.15F, 0.15F, 0.90F)
					                           : MakeVector4(0.20F, 0.08F, 0.08F, 0.65F);
					renderer->SetColorAlphaPremultiplied(bgCol);
					renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);
					renderer->SetColorAlphaPremultiplied(MakeVector4(0.8F, 0.3F, 0.3F, 0.85F));
					renderer->DrawOutlinedRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

					Vector2 sz = smallFont.Measure("X");
					Vector2 pos = item.rect.min + MakeVector2((item.rect.GetWidth() - sz.x) * 0.5F, (item.rect.GetHeight() - sz.y) * 0.5F);
					smallFont.DrawShadow("X", pos, 1.0F, MakeVector4(1, 1, 1, 1), shadowColor);
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
				renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.min.x + (selected ? 3.5F : 2.0F), item.rect.max.y);

				// Button Label
				std::string str = item.text;
				if (item.type == MenuItemType::Team1) str = host->GetTeamName(0);
				else if (item.type == MenuItemType::Team2) str = host->GetTeamName(1);

				Vector2 size = smallFont.Measure(str);
				Vector2 pos = MakeVector2(item.rect.min.x + 8.0F, item.rect.min.y + (item.rect.GetHeight() - size.y) * 0.5F);
				smallFont.DrawShadow(str, pos, 1.0F, selected ? MakeVector4(1, 1, 1, 1) : MakeVector4(0.9F, 0.92F, 0.95F, 0.95F), shadowColor);

				// Hotkey hint [1], [2], [3]
				if (index > 0) {
					std::string hk = Format("[{0}]", index);
					Vector2 hkPos = MakeVector2(item.rect.max.x - smallFont.Measure(hk).x - 6.0F, pos.y);
					smallFont.DrawShadow(hk, hkPos, 1.0F, selected ? MakeVector4(1, 1, 1, 1) : hotkeyColor, shadowColor);
				}
			}

			// Under the 3D character (Left side): clean glowing centered label
			float charColX = sw * 0.20F;
			float labelY = top + height - 20.0F;
			std::string charText = "MODO ESPECTADOR";
			Vector4 charColor = MakeVector4(0.7F, 0.75F, 0.8F, 1.0F);
			if (selectedTeam == 0) {
				charText = "SOLDADO AZUL";
				charColor = MakeVector4(0.35F, 0.75F, 1.0F, 1.0F);
			} else if (selectedTeam == 1) {
				charText = "SOLDADO ROJO";
				charColor = MakeVector4(1.0F, 0.35F, 0.35F, 1.0F);
			}
			Vector2 charTextSz = font.Measure(charText);
			font.DrawShadow(charText, MakeVector2(charColX - charTextSz.x * 0.5F, labelY), 1.0F, charColor, shadowColor);

			// Under the 3D weapon (Right side): clean glowing centered label
			if (selectedTeam < 2) {
				float weapColX = sw * 0.80F;
				std::string weapText = "FUSIL DE PRECISIÓN";
				Vector4 weapColor = MakeVector4(0.65F, 0.85F, 1.0F, 1.0F);
				if (selectedWeapon == SMG_WEAPON) {
					weapText = "SUBFUSIL SMG";
					weapColor = MakeVector4(0.40F, 0.95F, 0.65F, 1.0F);
				} else if (selectedWeapon == SHOTGUN_WEAPON) {
					weapText = "ESCOPETA DE COMBATE";
					weapColor = MakeVector4(1.0F, 0.70F, 0.35F, 1.0F);
				}
				Vector2 weapTextSz = font.Measure(weapText);
				font.DrawShadow(weapText, MakeVector2(weapColX - weapTextSz.x * 0.5F, labelY), 1.0F, weapColor, shadowColor);
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
