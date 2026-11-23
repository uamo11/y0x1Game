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

#include <algorithm>
#include <cctype>
#include "LimboMenu.h"
#include "SoftwareCursor.h"
#include <Client/IRenderer.h>
#include <Client/IModel.h>
#include <Client/SceneDefinition.h>
#include <Client/Fonts.h>
#include <Client/IFont.h>
#include <Core/FileManager.h>
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

			std::string ToLowerCopy(std::string s) {
				for (char& c : s)
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				return s;
			}

			bool ShouldSkipLimboAccessoryModel(const std::string& lowerPath) {
				return lowerPath.find("sight") != std::string::npos ||
				       lowerPath.find("scopedot") != std::string::npos ||
				       lowerPath.find("casing") != std::string::npos ||
				       lowerPath.find("shell") != std::string::npos ||
				       lowerPath.find("tracer") != std::string::npos ||
				       lowerPath.find("singlevoxel") != std::string::npos ||
				       lowerPath.find("bullet") != std::string::npos ||
				       lowerPath.find("hand") != std::string::npos;
			}

			std::vector<std::string> ExtractQuotedKv6Paths(const std::string& scriptText) {
				std::vector<std::string> paths;
				size_t pos = 0;
				while (pos < scriptText.size()) {
					size_t q1 = scriptText.find('"', pos);
					if (q1 == std::string::npos)
						break;
					size_t q2 = scriptText.find('"', q1 + 1);
					if (q2 == std::string::npos)
						break;
					std::string lit = scriptText.substr(q1 + 1, q2 - q1 - 1);
					std::string lower = ToLowerCopy(lit);
					if (lower.find("models/weapons/") != std::string::npos &&
					    lower.size() >= 4 &&
					    lower.compare(lower.size() - 4, 4, ".kv6") == 0 &&
					    !ShouldSkipLimboAccessoryModel(lower)) {
						if (std::find(paths.begin(), paths.end(), lit) == paths.end())
							paths.push_back(lit);
					}
					pos = q2 + 1;
				}
				return paths;
			}

			std::vector<Handle<client::IModel>> LoadActiveWeaponModels(client::IRenderer* renderer,
			                                                            const char* weaponDir,
			                                                            const char* fallbackPath) {
				std::vector<Handle<client::IModel>> out;
				std::vector<std::string> modelPaths;

				try {
					std::string tpPath = std::string("Scripts/Skin/") + weaponDir + "/ThirdPerson.as";
					if (FileManager::FileExists(tpPath.c_str())) {
						std::string text = FileManager::ReadAllBytes(tpPath.c_str());
						modelPaths = ExtractQuotedKv6Paths(text);
					}
				} catch (...) {
				}

				bool onlyFallback = (modelPaths.size() == 1 &&
				                     ToLowerCopy(modelPaths[0]) == ToLowerCopy(fallbackPath));
				if (modelPaths.empty() || onlyFallback) {
					try {
						std::string viewPath = std::string("Scripts/Skin/") + weaponDir + "/View.as";
						if (FileManager::FileExists(viewPath.c_str())) {
							std::string text = FileManager::ReadAllBytes(viewPath.c_str());
							std::vector<std::string> viewPaths = ExtractQuotedKv6Paths(text);
							std::vector<std::string> customPaths;
							for (const std::string& p : viewPaths) {
								std::string l = ToLowerCopy(p);
								if (l.find("weaponnomagazine.kv6") != std::string::npos ||
								    l.find("weaponnopump.kv6") != std::string::npos ||
								    l.find("magazine.kv6") != std::string::npos ||
								    l.find("pump.kv6") != std::string::npos)
									continue;
								customPaths.push_back(p);
							}
							if (!customPaths.empty())
								modelPaths = std::move(customPaths);
						}
					} catch (...) {
					}
				}

				for (const std::string& p : modelPaths) {
					try {
						Handle<client::IModel> m = renderer->RegisterModel(p.c_str());
						if (m)
							out.push_back(m);
					} catch (...) {
					}
				}

				if (out.empty()) {
					try {
						Handle<client::IModel> m = renderer->RegisterModel(fallbackPath);
						if (m)
							out.push_back(m);
					} catch (...) {
					}
				}
				return out;
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

			// Center selection panel: 460px wide, 204px high (clean spacing, no button overlap)
			cachedLayout.contentsWidth = std::min(460.0F, cachedLayout.screenWidth - 16.0F);
			cachedLayout.left = (cachedLayout.screenWidth - cachedLayout.contentsWidth) * 0.5F;
			float panelH = 204.0F;
			cachedLayout.top = cachedLayout.screenHeight - (panelH + 18.0F);

			cachedLayout.menuWidth = (cachedLayout.contentsWidth - 36.0F) * 0.5F;
			cachedLayout.menuHeight = 28.0F;
			cachedLayout.rowHeight = 34.0F;

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
						item.rect = AABB2(cachedLayout.left + 14.0F, cachedLayout.top + 158.0F, cachedLayout.contentsWidth - 28.0F, 32.0F);
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

			if (!weaponModelsLoaded) {
				rifleModels = LoadActiveWeaponModels(renderer.GetPointerOrNull(), "Rifle", "Models/Weapons/Rifle/Weapon.kv6");
				smgModels = LoadActiveWeaponModels(renderer.GetPointerOrNull(), "SMG", "Models/Weapons/SMG/Weapon.kv6");
				shotgunModels = LoadActiveWeaponModels(renderer.GetPointerOrNull(), "Shotgun", "Models/Weapons/Shotgun/Weapon.kv6");
				weaponModelsLoaded = true;
			}
			if (!playerTorsoModel) playerTorsoModel = renderer->RegisterModel("Models/Player/Torso.kv6");
			if (!playerHeadModel) playerHeadModel = renderer->RegisterModel("Models/Player/Head.kv6");
			if (!playerLegLeftModel) playerLegLeftModel = renderer->RegisterModel("Models/Player/Leg.kv6");
			if (!playerLegRightModel) playerLegRightModel = renderer->RegisterModel("Models/Player/LegRight.kv6");
			if (!playerArmsModel) playerArmsModel = renderer->RegisterModel("Models/Player/Arms.kv6");

			float sw = renderer->ScreenWidth();
			float sh = renderer->ScreenHeight();

			float tanHalfFovX = (sceneDef.fovX > 0.01F) ? tanf(sceneDef.fovX * 0.5F) : (tanf(DEG2RAD(35.0F)) * (sw / sh));
			float tanHalfFovY = (sceneDef.fovY > 0.01F) ? tanf(sceneDef.fovY * 0.5F) : tanf(DEG2RAD(35.0F));
			float fovScale = std::max(0.25F, tanHalfFovY / 0.57735F);

			Vector3 orig = sceneDef.viewOrigin;
			Vector3 dir = sceneDef.viewAxis[2];
			Vector3 side = sceneDef.viewAxis[0];
			Vector3 up = sceneDef.viewAxis[1];

			int selectedTeam = host->GetSelectedTeam();
			WeaponType selectedWeapon = host->GetSelectedWeapon();

			IntVector3 srvCol = host->GetTeamColor(selectedTeam);
			Vector3 teamColor = MakeVector3(srvCol.x / 255.0F, srvCol.y / 255.0F, srvCol.z / 255.0F);

			// 1. LEFT SIDE: 3D Player Character with exact server team color
			{
				float charNdcX = -0.60F;
				float charNdcY = -0.18F;
				float charDist = 4.6F / fovScale;

				float charRight = charNdcX * tanHalfFovX * charDist;
				float charUp = charNdcY * tanHalfFovY * charDist;
				Vector3 charPos = orig + dir * charDist + side * charRight + up * charUp;

				// Tiny neutral fill light right in front of preview character (does not bleed onto map walls)
				client::DynamicLightParam charLight;
				charLight.type = client::DynamicLightTypePoint;
				charLight.origin = charPos + up * 0.8F - dir * 1.1F + side * 0.2F;
				charLight.radius = 1.5F;
				charLight.color = MakeVector3(0.45F, 0.45F, 0.45F);
				renderer->AddLight(charLight);

				float breath = sinf(showcaseTime * 2.2F);
				float pitch = 0.03F + breath * 0.015F;
				float armPitch = -0.25F + breath * 0.02F;

				// Facing towards camera (-dir) and turned slightly right towards center (+side)
				Vector3 facing = (-dir * 0.88F + side * 0.45F).Normalize();
				Vector3 rgt = Vector3::Cross(facing, up).Normalize();

				// Right-handed orthonormal basis (det = +1)
				Matrix4 lower = Matrix4::FromAxis(rgt, -facing, -up, charPos);
				Matrix4 scaler = Matrix4::Scale(0.085F) * Matrix4::Scale(-1, -1, 1);

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
				              * Matrix4::Rotate(MakeVector3(1, 0, 0), breath * 0.015F);
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

			// 2. RIGHT SIDE: Active Mod 3D Weapon Model Physically Rotating 360° (Auto-Normalized BoundingBox)
			if (selectedTeam < 2) {
				const std::vector<Handle<client::IModel>>* curModels = &rifleModels;
				if (selectedWeapon == SMG_WEAPON) curModels = &smgModels;
				else if (selectedWeapon == SHOTGUN_WEAPON) curModels = &shotgunModels;

				if (!curModels->empty() && (*curModels)[0]) {
					AABB3 bb = (*curModels)[0]->GetBoundingBox();
					for (size_t i = 1; i < curModels->size(); ++i) {
						if (!(*curModels)[i]) continue;
						AABB3 partBB = (*curModels)[i]->GetBoundingBox();
						bb.min.x = std::min(bb.min.x, partBB.min.x);
						bb.min.y = std::min(bb.min.y, partBB.min.y);
						bb.min.z = std::min(bb.min.z, partBB.min.z);
						bb.max.x = std::max(bb.max.x, partBB.max.x);
						bb.max.y = std::max(bb.max.y, partBB.max.y);
						bb.max.z = std::max(bb.max.z, partBB.max.z);
					}
					Vector3 bbCenter = (bb.min + bb.max) * 0.5F;
					float maxExtent = std::max({bb.GetWidth(), bb.GetHeight(), bb.GetDepth(), 1.0F});
					float targetSize = 1.15F;
					float autoScale = targetSize / maxExtent;

					float weapNdcX = 0.60F;
					float weapNdcY = 0.02F;
					float weapDist = 3.2F / fovScale;

					float weapRight = weapNdcX * tanHalfFovX * weapDist;
					float weapUp = weapNdcY * tanHalfFovY * weapDist;
					Vector3 weapPos = orig + dir * weapDist + side * weapRight + up * weapUp;

					float bob = sinf(showcaseTime * 2.2F) * 0.02F;
					Vector3 actualPos = weapPos + up * bob;

					client::DynamicLightParam weapLight;
					weapLight.type = client::DynamicLightTypePoint;
					weapLight.origin = actualPos + up * 0.6F - dir * 0.9F + side * 0.1F;
					weapLight.radius = 1.4F;
					weapLight.color = MakeVector3(0.45F, 0.45F, 0.45F);
					renderer->AddLight(weapLight);

					// Smooth 360 physical rotation around Screen UP with orthonormal RHS basis (det = +1)
					Vector3 rotFwd = (side * cosf(weaponYaw) - dir * sinf(weaponYaw)).Normalize();
					Vector3 weapBarrel = (rotFwd + up * 0.12F).Normalize();
					Vector3 weapRightVec = Vector3::Cross(weapBarrel, -up).Normalize();
					Vector3 weapDown = Vector3::Cross(weapRightVec, weapBarrel).Normalize();

					Matrix4 weapMatrix = Matrix4::FromAxis(weapRightVec, weapBarrel, weapDown, actualPos);

					client::ModelRenderParam weapParam;
					weapParam.depthHack = true;
					weapParam.customColor = teamColor;
					weapParam.matrix = weapMatrix * Matrix4::Scale(autoScale) * Matrix4::Translate(-bbCenter);

					for (const auto& part : *curModels) {
						if (part)
							renderer->RenderModel(*part, weapParam);
					}
				}
			}
		}

		void LimboMenu::Draw() {
			if (!isActive)
				return;

			EnsureLayout();

			client::IFont& font = fontManager->GetGuiFont();

			const float sw = cachedLayout.screenWidth;
			const float sh = cachedLayout.screenHeight;
			const float left = cachedLayout.left;
			const float top = cachedLayout.top;
			const float contentsWidth = cachedLayout.contentsWidth;
			const float height = 204.0F;

			const int selectedTeam = host->GetSelectedTeam();
			const WeaponType selectedWeapon = host->GetSelectedWeapon();

			Vector4 textColor = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
			Vector4 shadowColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.55F);
			Vector4 hotkeyColor = MakeVector4(1.0F, 1.0F, 1.0F, 0.60F);

			// Native neutral translucent panel
			renderer->SetColorAlphaPremultiplied(MakeVector4(0.0F, 0.0F, 0.0F, 0.62F));
			renderer->DrawFilledRect(left, top, left + contentsWidth, top + height);

			renderer->SetColorAlphaPremultiplied(MakeVector4(1.0F, 1.0F, 1.0F, 0.14F));
			renderer->DrawOutlinedRect(left, top, left + contentsWidth, top + height);

			// Native crisp titles at 1.0x scale using GuiFont
			font.DrawShadow(_Tr("Client", "Select Team:"), MakeVector2(cachedLayout.teamX, top + 16.0F), 1.0F, textColor, shadowColor);
			if (selectedTeam < 2) {
				font.DrawShadow(_Tr("Client", "Select Weapon:"), MakeVector2(cachedLayout.weapX, top + 16.0F), 1.0F, textColor, shadowColor);
			}

			// Render Team & Weapon Buttons in native OpenSpades UI style
			for (const auto& item : items) {
				if (!item.visible)
					continue;

				bool selected = false;
				int index = 0;
				bool hasTeamBar = false;
				Vector4 teamBarCol = MakeVector4(0.65F, 0.65F, 0.65F, 1.0F);

				switch (item.type) {
					case MenuItemType::Team1:
					case MenuItemType::Team2:
					case MenuItemType::TeamSpectator: {
						int teamId = TeamIdOf(item.type);
						selected = (selectedTeam == teamId);
						index = (selectedTeam >= 2) ? (1 + teamId) : 0;
						if (teamId < 2) {
							IntVector3 tc = host->GetTeamColor(teamId);
							teamBarCol = MakeVector4(tc.x / 255.0F, tc.y / 255.0F, tc.z / 255.0F, 1.0F);
							hasTeamBar = true;
						}
						break;
					}
					case MenuItemType::WeaponRifle:
					case MenuItemType::WeaponSMG:
					case MenuItemType::WeaponShotgun: {
						WeaponType weapon = WeaponOf(item.type);
						selected = (selectedWeapon == weapon);
						index = (selectedTeam < 2) ? (1 + (int)weapon) : 0;
						break;
					}
					default: selected = false;
				}

				if (item.type == MenuItemType::Spawn) {
					Vector4 bgCol = item.hover ? MakeVector4(0.22F, 0.42F, 0.26F, 0.85F)
					                           : MakeVector4(0.14F, 0.28F, 0.17F, 0.72F);
					renderer->SetColorAlphaPremultiplied(bgCol);
					renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

					renderer->SetColorAlphaPremultiplied(MakeVector4(0.0F, 0.0F, 0.0F, 0.75F));
					renderer->DrawOutlinedRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

					std::string spawnTxt = _Tr("Client", "Spawn");
					Vector2 txtSz = font.Measure(spawnTxt);
					Vector2 txtPos = item.rect.min + MakeVector2((item.rect.GetWidth() - txtSz.x) * 0.5F, (item.rect.GetHeight() - txtSz.y) * 0.5F);
					font.DrawShadow(spawnTxt, txtPos, 1.0F, textColor, shadowColor);
					continue;
				} else if (item.type == MenuItemType::Close) {
					Vector4 bgCol = item.hover ? MakeVector4(0.45F, 0.20F, 0.20F, 0.85F)
					                           : MakeVector4(0.20F, 0.20F, 0.20F, 0.65F);
					renderer->SetColorAlphaPremultiplied(bgCol);
					renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);
					renderer->SetColorAlphaPremultiplied(MakeVector4(0.0F, 0.0F, 0.0F, 0.75F));
					renderer->DrawOutlinedRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

					Vector2 sz = font.Measure("X");
					Vector2 pos = item.rect.min + MakeVector2((item.rect.GetWidth() - sz.x) * 0.5F, (item.rect.GetHeight() - sz.y) * 0.5F);
					font.DrawShadow("X", pos, 1.0F, textColor, shadowColor);
					continue;
				}

				Vector4 fillColor = MakeVector4(0.18F, 0.18F, 0.18F, 0.65F);
				if (selected)
					fillColor = MakeVector4(0.50F, 0.50F, 0.50F, 0.88F);
				else if (item.hover)
					fillColor = MakeVector4(0.32F, 0.32F, 0.32F, 0.78F);

				renderer->SetColorAlphaPremultiplied(fillColor);
				renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

				renderer->SetColorAlphaPremultiplied(MakeVector4(0.0F, 0.0F, 0.0F, 0.75F));
				renderer->DrawOutlinedRect(item.rect.min.x, item.rect.min.y, item.rect.max.x, item.rect.max.y);

				if (hasTeamBar) {
					renderer->SetColorAlphaPremultiplied(teamBarCol);
					renderer->DrawFilledRect(item.rect.min.x, item.rect.min.y, item.rect.min.x + 3.0F, item.rect.max.y);
				}

				std::string str = item.text;
				if (item.type == MenuItemType::Team1) str = host->GetTeamName(0);
				else if (item.type == MenuItemType::Team2) str = host->GetTeamName(1);

				Vector2 size = font.Measure(str);
				Vector2 pos = MakeVector2(item.rect.min.x + 10.0F, item.rect.min.y + (item.rect.GetHeight() - size.y) * 0.5F);
				font.DrawShadow(str, pos, 1.0F, textColor, shadowColor);

				if (index > 0) {
					std::string hk = Format("[{0}]", index);
					Vector2 hkPos = MakeVector2(item.rect.max.x - font.Measure(hk).x - 8.0F, pos.y);
					font.DrawShadow(hk, hkPos, 1.0F, selected ? textColor : hotkeyColor, shadowColor);
				}
			}

			// Under the 3D character (Left side): actual server team name in brightened server team color
			float charColX = sw * 0.19F;
			float labelY = std::min(sh - 34.0F, top + height - 24.0F);
			std::string charText = _Tr("Client", "Spectator");
			Vector4 charColor = MakeVector4(0.85F, 0.85F, 0.85F, 1.0F);
			if (selectedTeam < 2) {
				charText = host->GetTeamName(selectedTeam);
				IntVector3 tc = host->GetTeamColor(selectedTeam);
				Vector3 brightened = Mix(MakeVector3(tc.x / 255.0F, tc.y / 255.0F, tc.z / 255.0F),
				                         MakeVector3(1.0F, 1.0F, 1.0F), 0.25F);
				charColor = MakeVector4(brightened.x, brightened.y, brightened.z, 1.0F);
			}
			Vector2 charTextSz = font.Measure(charText);
			font.DrawShadow(charText, MakeVector2(charColX - charTextSz.x * 0.5F, labelY), 1.0F, charColor, shadowColor);

			// Under the 3D weapon (Right side): crisp weapon name
			if (selectedTeam < 2) {
				float weapColX = sw * 0.81F;
				std::string weapText = _Tr("Client", "Rifle");
				if (selectedWeapon == SMG_WEAPON) {
					weapText = "SMG";
				} else if (selectedWeapon == SHOTGUN_WEAPON) {
					weapText = _Tr("Client", "Shotgun");
				}
				Vector2 weapTextSz = font.Measure(weapText);
				font.DrawShadow(weapText, MakeVector2(weapColX - weapTextSz.x * 0.5F, labelY), 1.0F, textColor, shadowColor);
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
