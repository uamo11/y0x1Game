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

#include "ModWeaponSelectScreen.h"
#include "MainScreenUI.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <Client/Fonts.h>
#include <Client/IAudioDevice.h>
#include <Client/IFont.h>
#include <Client/IRenderer.h>
#include <Core/FileManager.h>
#include <Core/Settings.h>
#include <Core/ShellApi.h>
#include <Core/Strings.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/Widgets/DrawUtils.h>

DEFINE_SPADES_SETTING(cg_mod_weapon_smg, "smg_mp5");
DEFINE_SPADES_SETTING(cg_mod_weapon_rifle, "rifle_classic");
DEFINE_SPADES_SETTING(cg_mod_weapon_shotgun, "shotgun_classic");
DEFINE_SPADES_SETTING(cg_mod_weapon_spade, "spade_military");
DEFINE_SPADES_SETTING(cg_mod_attachment_sight, "reflex");
DEFINE_SPADES_SETTING(cg_mod_audio_profile, "tactical");

namespace spades {
	namespace gui {
		using ui::Button;
		using ui::SetColorNP;
		using ui::UIElement;
		using ui::UIManager;

		/** Sidebar tab button for the Mod / Weapon Customizer */
		class ModTabButton : public ui::Button {
		public:
			bool isSelected = false;

			ModTabButton(UIManager* manager) : ui::Button(manager) {
				alignment = MakeVector2(0.0F, 0.5F);
			}

			void Render() override {
				client::IRenderer& r = GetManager().GetRenderer();
				Vector2 pos = GetScreenPosition();
				Vector2 sz = size;

				Vector4 bg = MakeVector4(0.07F, 0.10F, 0.15F, 0.70F);
				Vector4 textCol = MakeVector4(0.80F, 0.88F, 0.95F, 0.90F);

				if (isSelected) {
					bg = MakeVector4(0.14F, 0.28F, 0.48F, 0.95F);
					textCol = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
				} else if (hover) {
					bg = MakeVector4(0.10F, 0.16F, 0.25F, 0.85F);
					textCol = MakeVector4(0.95F, 0.98F, 1.0F, 1.0F);
				}

				SetColorNP(r, bg);
				r.DrawFilledRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				if (isSelected) {
					SetColorNP(r, MakeVector4(0.35F, 0.85F, 1.0F, 1.0F));
					r.DrawFilledRect(pos.x, pos.y, pos.x + 3.5F, pos.y + sz.y);
				} else {
					SetColorNP(r, MakeVector4(0.15F, 0.22F, 0.32F, 0.50F));
					r.DrawOutlinedRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);
				}

				client::IFont* font = GetFont();
				if (font) {
					Vector4 shadow = MakeVector4(0.0F, 0.0F, 0.0F, 0.65F);
					font->DrawShadow(caption, MakeVector2(pos.x + 12.0F, pos.y + (sz.y - font->Measure(caption).y) * 0.5F),
					                0.88F, textCol, shadow);
				}
			}
		};

		/** Subcategory tab button (Ametralladora, Rifle, Escopeta, Pala, Granada) */
		class WeaponSubCategoryBtn : public ui::Button {
		public:
			bool isSelected = false;

			WeaponSubCategoryBtn(UIManager* manager) : ui::Button(manager) {}

			void Render() override {
				client::IRenderer& r = GetManager().GetRenderer();
				Vector2 pos = GetScreenPosition();
				Vector2 sz = size;

				Vector4 bg = MakeVector4(0.07F, 0.10F, 0.16F, 0.75F);
				Vector4 border = MakeVector4(0.18F, 0.26F, 0.38F, 0.60F);
				Vector4 textCol = MakeVector4(0.75F, 0.85F, 0.95F, 0.85F);

				if (isSelected) {
					bg = MakeVector4(0.16F, 0.32F, 0.55F, 0.95F);
					border = MakeVector4(0.40F, 0.85F, 1.0F, 1.0F);
					textCol = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
				} else if (hover) {
					bg = MakeVector4(0.11F, 0.18F, 0.28F, 0.85F);
					border = MakeVector4(0.28F, 0.50F, 0.75F, 0.85F);
					textCol = MakeVector4(0.95F, 0.98F, 1.0F, 1.0F);
				}

				SetColorNP(r, bg);
				r.DrawFilledRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				SetColorNP(r, border);
				r.DrawOutlinedRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				if (isSelected) {
					SetColorNP(r, MakeVector4(0.40F, 0.85F, 1.0F, 1.0F));
					r.DrawFilledRect(pos.x, pos.y + sz.y - 2.5F, pos.x + sz.x, pos.y + sz.y);
				}

				client::IFont* font = GetFont();
				if (font) {
					Vector2 m = font->Measure(caption) * 0.75F;
					font->DrawShadow(caption, MakeVector2(pos.x + (sz.x - m.x) * 0.5F, pos.y + (sz.y - m.y) * 0.5F),
					                0.75F, textCol, MakeVector4(0.0F, 0.0F, 0.0F, 0.7F));
				}
			}
		};

		/** Selectable weapon option row with real-time indicators */
		class WeaponOptionRow : public ui::Button {
		public:
			WeaponOptionDef def;
			bool isSelected = false;
			bool isEquipped = false;

			WeaponOptionRow(UIManager* manager, const WeaponOptionDef& d, bool sel, bool eq)
			    : ui::Button(manager), def(d), isSelected(sel), isEquipped(eq) {}

			void Render() override {
				client::IRenderer& r = GetManager().GetRenderer();
				Vector2 pos = GetScreenPosition();
				Vector2 sz = size;

				Vector4 bg = MakeVector4(0.06F, 0.08F, 0.12F, 0.70F);
				Vector4 border = MakeVector4(0.18F, 0.24F, 0.36F, 0.60F);

				if (isSelected) {
					bg = MakeVector4(0.12F, 0.24F, 0.40F, 0.95F);
					border = MakeVector4(0.35F, 0.80F, 1.0F, 1.0F);
				} else if (hover) {
					bg = MakeVector4(0.09F, 0.15F, 0.24F, 0.85F);
					border = MakeVector4(0.26F, 0.48F, 0.72F, 0.85F);
				}

				SetColorNP(r, bg);
				r.DrawFilledRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				SetColorNP(r, border);
				r.DrawOutlinedRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				// Selection radio/check box
				float chkX = pos.x + 8.0F;
				float chkY = pos.y + (sz.y - 14.0F) * 0.5F;
				SetColorNP(r, MakeVector4(0.20F, 0.30F, 0.45F, 0.85F));
				r.DrawOutlinedRect(chkX, chkY, chkX + 14.0F, chkY + 14.0F);

				if (isSelected) {
					SetColorNP(r, MakeVector4(0.40F, 0.85F, 1.0F, 1.0F));
					r.DrawFilledRect(chkX + 3.0F, chkY + 3.0F, chkX + 11.0F, chkY + 11.0F);
				}

				// Swatch tag
				if (def.tagColor.w > 0.0F) {
					float tagW = 10.0F;
					float tagH = 12.0F;
					float tagX = chkX + 18.0F;
					float tagY = pos.y + (sz.y - tagH) * 0.5F;
					SetColorNP(r, def.tagColor);
					r.DrawFilledRect(tagX, tagY, tagX + tagW, tagY + tagH);
					SetColorNP(r, MakeVector4(1.0F, 1.0F, 1.0F, 0.60F));
					r.DrawOutlinedRect(tagX, tagY, tagX + tagW, tagY + tagH);
				}

				client::IFont* font = GetFont();
				if (font) {
					float textOffsetX = 44.0F;
					Vector4 titleCol = isSelected ? MakeVector4(1.0F, 1.0F, 1.0F, 1.0F)
					                              : MakeVector4(0.90F, 0.93F, 0.98F, 0.92F);
					Vector4 shadowCol = MakeVector4(0.0F, 0.0F, 0.0F, 0.70F);

					// Title
					font->DrawShadow(def.title, MakeVector2(pos.x + textOffsetX, pos.y + 5.0F), 0.85F,
					                titleCol, shadowCol);

					// Subtitle: Caliber + RPM
					std::string sub = def.caliber;
					if (def.rpm > 0)
						sub += " • " + std::to_string(def.rpm) + " RPM";
					Vector4 subCol = isSelected ? MakeVector4(0.55F, 0.82F, 1.0F, 0.90F)
					                            : MakeVector4(0.50F, 0.65F, 0.78F, 0.80F);
					font->DrawShadow(sub, MakeVector2(pos.x + textOffsetX, pos.y + 20.0F), 0.68F,
					                subCol, shadowCol);

					// Equipped tag on the right
					if (isEquipped) {
						float eqW = 68.0F;
						float eqH = 15.0F;
						float eqX = pos.x + sz.x - eqW - 6.0F;
						float eqY = pos.y + (sz.y - eqH) * 0.5F;

						SetColorNP(r, MakeVector4(0.12F, 0.45F, 0.25F, 0.90F));
						r.DrawFilledRect(eqX, eqY, eqX + eqW, eqY + eqH);
						SetColorNP(r, MakeVector4(0.35F, 0.90F, 0.45F, 0.95F));
						r.DrawOutlinedRect(eqX, eqY, eqX + eqW, eqY + eqH);

						std::string eqText = "EQUIPADA";
						Vector2 m = font->Measure(eqText) * 0.62F;
						font->DrawShadow(eqText, MakeVector2(eqX + (eqW - m.x) * 0.5F, eqY + (eqH - m.y) * 0.5F),
						                0.62F, MakeVector4(1.0F, 1.0F, 1.0F, 1.0F), shadowCol);
					}
				}
			}
		};

		ModWeaponSelectScreen::ModWeaponSelectScreen(ui::UIElement* owner, client::FontManager* fontManager,
		                                             MainScreenUI* mainScreenUI)
		    : UIElement(&owner->GetManager()), owner(owner), fontManager(fontManager),
		      mainScreenUI(mainScreenUI) {
			UIManager* manager = &GetManager();
			SetFont(&fontManager->GetGuiFont());
			SetBounds(manager->GetRootElement().GetBounds());

			float sw = manager->screenWidth;
			float sh = manager->screenHeight;

			float dlgW = std::min(840.0F, sw * 0.94F);
			float dlgH = std::min(580.0F, sh - 32.0F);
			float dlgX = (sw - dlgW) * 0.5F;
			float dlgY = (sh - dlgH) * 0.5F;

			float sidebarW = 125.0F;
			float tabH = 32.0F;
			float tabGap = 3.0F;

			// --- 1. Sidebar Tabs (Categorías Principales) ---
			{
				Handle<ModTabButton> btn = Handle<ModTabButton>::New(manager);
				tabWeapons = btn.GetPointerOrNull();
				tabWeapons->caption = _Tr("MainScreen", "Armas");
				tabWeapons->SetBounds(AABB2(dlgX + 8.0F, dlgY + 45.0F, sidebarW, tabH));
				tabWeapons->isSelected = true;
				tabWeapons->activated = [this](UIElement&) { SwitchMainCategory(0); };
				AddChild(tabWeapons);
			}
			{
				Handle<ModTabButton> btn = Handle<ModTabButton>::New(manager);
				tabAccessories = btn.GetPointerOrNull();
				tabAccessories->caption = _Tr("MainScreen", "Accesorios");
				tabAccessories->SetBounds(AABB2(dlgX + 8.0F, dlgY + 45.0F + (tabH + tabGap), sidebarW, tabH));
				tabAccessories->activated = [this](UIElement&) { SwitchMainCategory(1); };
				AddChild(tabAccessories);
			}
			{
				Handle<ModTabButton> btn = Handle<ModTabButton>::New(manager);
				tabAudio = btn.GetPointerOrNull();
				tabAudio->caption = _Tr("MainScreen", "Sonidos FX");
				tabAudio->SetBounds(AABB2(dlgX + 8.0F, dlgY + 45.0F + 2.0F * (tabH + tabGap), sidebarW, tabH));
				tabAudio->activated = [this](UIElement&) { SwitchMainCategory(2); };
				AddChild(tabAudio);
			}
			{
				Handle<ModTabButton> btn = Handle<ModTabButton>::New(manager);
				tabPaks = btn.GetPointerOrNull();
				tabPaks->caption = _Tr("MainScreen", "Paquetes .pak");
				tabPaks->SetBounds(AABB2(dlgX + 8.0F, dlgY + 45.0F + 3.0F * (tabH + tabGap), sidebarW, tabH));
				tabPaks->activated = [this](UIElement&) { SwitchMainCategory(3); };
				AddChild(tabPaks);
			}

			// --- 2. Middle Column: Subcategories & Weapon Rows ---
			float midX = dlgX + sidebarW + 16.0F;
			float midW = 275.0F;
			float subBtnH = 26.0F;
			float subBtnW = (midW - 12.0F) / 4.0F;

			// Subcategory panel container
			Handle<UIElement> subPnl = Handle<UIElement>::New(manager);
			subCategoryPanel = subPnl.GetPointerOrNull();
			subCategoryPanel->SetBounds(AABB2(midX, dlgY + 45.0F, midW, subBtnH));
			AddChild(subCategoryPanel);

			// Subcategory buttons
			{
				Handle<WeaponSubCategoryBtn> b = Handle<WeaponSubCategoryBtn>::New(manager);
				subSmgBtn = b.GetPointerOrNull();
				subSmgBtn->caption = _Tr("MainScreen", "SMG");
				subSmgBtn->SetBounds(AABB2(0.0F, 0.0F, subBtnW, subBtnH));
				subSmgBtn->isSelected = true;
				subSmgBtn->activated = [this](UIElement&) { SwitchWeaponSubCategory(0); };
				subCategoryPanel->AddChild(subSmgBtn);
			}
			{
				Handle<WeaponSubCategoryBtn> b = Handle<WeaponSubCategoryBtn>::New(manager);
				subRifleBtn = b.GetPointerOrNull();
				subRifleBtn->caption = _Tr("MainScreen", "Rifle");
				subRifleBtn->SetBounds(AABB2(subBtnW + 4.0F, 0.0F, subBtnW, subBtnH));
				subRifleBtn->activated = [this](UIElement&) { SwitchWeaponSubCategory(1); };
				subCategoryPanel->AddChild(subRifleBtn);
			}
			{
				Handle<WeaponSubCategoryBtn> b = Handle<WeaponSubCategoryBtn>::New(manager);
				subShotgunBtn = b.GetPointerOrNull();
				subShotgunBtn->caption = _Tr("MainScreen", "Escopeta");
				subShotgunBtn->SetBounds(AABB2(2.0F * (subBtnW + 4.0F), 0.0F, subBtnW, subBtnH));
				subShotgunBtn->activated = [this](UIElement&) { SwitchWeaponSubCategory(2); };
				subCategoryPanel->AddChild(subShotgunBtn);
			}
			{
				Handle<WeaponSubCategoryBtn> b = Handle<WeaponSubCategoryBtn>::New(manager);
				subSpadeBtn = b.GetPointerOrNull();
				subSpadeBtn->caption = _Tr("MainScreen", "Pala");
				subSpadeBtn->SetBounds(AABB2(3.0F * (subBtnW + 4.0F), 0.0F, subBtnW, subBtnH));
				subSpadeBtn->activated = [this](UIElement&) { SwitchWeaponSubCategory(3); };
				subCategoryPanel->AddChild(subSpadeBtn);
			}

			// Weapon items list panel
			Handle<UIElement> listPnl = Handle<UIElement>::New(manager);
			weaponListPanel = listPnl.GetPointerOrNull();
			weaponListPanel->SetBounds(AABB2(midX, dlgY + 80.0F, midW, dlgH - 95.0F));
			AddChild(weaponListPanel);

			// --- 3. Right Column Action Buttons ---
			float rightX = dlgX + sidebarW + 16.0F + midW + 16.0F;
			float rightW = dlgW - (sidebarW + 16.0F + midW + 28.0F);

			{
				Handle<ui::Button> btn = Handle<ui::Button>::New(manager);
				equipButton = btn.GetPointerOrNull();
				equipButton->caption = _Tr("MainScreen", "★ EQUIPAR ESTA ARMA");
				equipButton->SetBounds(AABB2(rightX, dlgY + dlgH - 44.0F, rightW * 0.50F - 4.0F, 34.0F));
				equipButton->activated = [this](UIElement&) { EquipSelectedWeapon(); };
				AddChild(equipButton);
			}
			{
				Handle<ui::Button> btn = Handle<ui::Button>::New(manager);
				testFireButton = btn.GetPointerOrNull();
				testFireButton->caption = _Tr("MainScreen", "▶ PROBAR DISPARO");
				testFireButton->SetBounds(AABB2(rightX + rightW * 0.50F, dlgY + dlgH - 44.0F, rightW * 0.28F - 4.0F, 34.0F));
				testFireButton->activated = [this](UIElement&) { PlayTestSound(); };
				AddChild(testFireButton);
			}
			{
				Handle<ui::Button> btn = Handle<ui::Button>::New(manager);
				closeButton = btn.GetPointerOrNull();
				closeButton->caption = _Tr("MainScreen", "CERRAR");
				closeButton->SetBounds(AABB2(rightX + rightW * 0.78F, dlgY + dlgH - 44.0F, rightW * 0.22F, 34.0F));
				closeButton->activated = [this](UIElement&) { Close(); };
				AddChild(closeButton);
			}

			// Load initial subcategory (SMG)
			SwitchWeaponSubCategory(0);
		}

		void ModWeaponSelectScreen::SwitchMainCategory(int index) {
			activeMainCategory = index;
			if (tabWeapons)
				tabWeapons->isSelected = (index == 0);
			if (tabAccessories)
				tabAccessories->isSelected = (index == 1);
			if (tabAudio)
				tabAudio->isSelected = (index == 2);
			if (tabPaks)
				tabPaks->isSelected = (index == 3);

			if (subCategoryPanel)
				subCategoryPanel->visible = (index == 0);
			if (weaponListPanel)
				weaponListPanel->visible = (index == 0);

			if (index == 0) {
				SwitchWeaponSubCategory(activeWeaponSubCategory);
			}
		}

		void ModWeaponSelectScreen::SwitchWeaponSubCategory(int sub) {
			activeWeaponSubCategory = sub;
			if (subSmgBtn)
				subSmgBtn->isSelected = (sub == 0);
			if (subRifleBtn)
				subRifleBtn->isSelected = (sub == 1);
			if (subShotgunBtn)
				subShotgunBtn->isSelected = (sub == 2);
			if (subSpadeBtn)
				subSpadeBtn->isSelected = (sub == 3);

			activeCategoryDefs.clear();

			if (sub == 0) {
				// --- SMG (Ametralladora) ---
				WeaponOptionDef d1;
				d1.id = "smg_classic";
				d1.title = "Subfusil SMG Clásico";
				d1.weaponTypeStr = "Subfusil Automático (Ametralladora)";
				d1.caliber = "9x19mm Parabellum";
				d1.ammo = "30 / 120 Tiros";
				d1.rpm = 545;
				d1.damageTorso = 29;
				d1.damageHead = 72;
				d1.accuracy = 0.75F;
				d1.rangeMeters = 42.0F;
				d1.mobility = 0.95F;
				d1.desc = "El subfusil clásico reglamentario de Ace of Spades. Cadencia equilibrada y retroceso suave, ideal para escaramuzas a media distancia.";
				d1.modelPath = "Models/Weapons/SMG/Weapon.kv6";
				d1.sightPath = "";
				d1.customColor = MakeVector4(0.80F, 0.80F, 0.80F, 1.0F);
				d1.tagColor = MakeVector4(0.30F, 0.50F, 0.75F, 1.0F);
				d1.soundPath = "Sounds/Weapons/SMG/V2Third1.opus";
				activeCategoryDefs.push_back(d1);

				WeaponOptionDef d2;
				d2.id = "smg_mp5";
				d2.title = "MP5 Táctica Spec-Ops";
				d2.weaponTypeStr = "Subfusil Táctico Especial (Ametralladora)";
				d2.caliber = "9x19mm NATO Parabellum";
				d2.ammo = "30 / 120 Tiros";
				d2.rpm = 720;
				d2.damageTorso = 31;
				d2.damageHead = 75;
				d2.accuracy = 0.88F;
				d2.rangeMeters = 50.0F;
				d2.mobility = 0.92F;
				d2.desc = "Subfusil alemán de alta precisión con mira reflex holográfica y cañón corto. Máxima eficacia en combate cerrado (CQB).";
				d2.modelPath = "Models/Weapons/SMG/Weapon.kv6";
				d2.sightPath = "Models/Weapons/SMG/SightReflex.kv6";
				d2.customColor = MakeVector4(0.20F, 0.22F, 0.25F, 1.0F);
				d2.tagColor = MakeVector4(0.15F, 0.40F, 0.85F, 1.0F);
				d2.soundPath = "Sounds/Weapons/SMG/V2Third2.opus";
				activeCategoryDefs.push_back(d2);

				WeaponOptionDef d3;
				d3.id = "smg_p90";
				d3.title = "P90 Bullpup Compacta";
				d3.weaponTypeStr = "Arma de Defensa Personal PDW (Ametralladora)";
				d3.caliber = "5.7x28mm Perforante";
				d3.ammo = "50 / 150 Tiros";
				d3.rpm = 850;
				d3.damageTorso = 26;
				d3.damageHead = 65;
				d3.accuracy = 0.80F;
				d3.rangeMeters = 45.0F;
				d3.mobility = 0.98F;
				d3.desc = "Diseño bullpup futurista con cargador horizontal translúcido de 50 cartuchos. Dispara a ráfagas devastadoras en asaltos veloces.";
				d3.modelPath = "Models/Weapons/SMG/WeaponNoMagazine.kv6";
				d3.sightPath = "Models/Weapons/SMG/SightFront.kv6";
				d3.customColor = MakeVector4(0.30F, 0.35F, 0.40F, 1.0F);
				d3.tagColor = MakeVector4(0.20F, 0.70F, 0.50F, 1.0F);
				d3.soundPath = "Sounds/Weapons/SMG/V2Third3.opus";
				activeCategoryDefs.push_back(d3);

				WeaponOptionDef d4;
				d4.id = "smg_tommy";
				d4.title = "Thompson M1928 Tommy";
				d4.weaponTypeStr = "Subfusil Pesado de Trinchera (Ametralladora)";
				d4.caliber = ".45 ACP Alto Impacto";
				d4.ammo = "50 / 100 Tiros";
				d4.rpm = 680;
				d4.damageTorso = 35;
				d4.damageHead = 85;
				d4.accuracy = 0.70F;
				d4.rangeMeters = 38.0F;
				d4.mobility = 0.86F;
				d4.desc = "El icónico subfusil con culata de nogal americano. Munición calibre .45 ACP de contundencia demoledora en combates a corta distancia.";
				d4.modelPath = "Models/Weapons/SMG/Weapon.kv6";
				d4.sightPath = "";
				d4.customColor = MakeVector4(0.55F, 0.35F, 0.20F, 1.0F);
				d4.tagColor = MakeVector4(0.70F, 0.40F, 0.15F, 1.0F);
				d4.soundPath = "Sounds/Weapons/SMG/V2Third4.opus";
				activeCategoryDefs.push_back(d4);

				WeaponOptionDef d5;
				d5.id = "smg_uzi";
				d5.title = "Micro Uzi Israeli Spec";
				d5.weaponTypeStr = "Pistola Ametralladora Automática";
				d5.caliber = "9mm High Velocity";
				d5.ammo = "32 / 128 Tiros";
				d5.rpm = 950;
				d5.damageTorso = 24;
				d5.damageHead = 60;
				d5.accuracy = 0.65F;
				d5.rangeMeters = 32.0F;
				d5.mobility = 1.0F;
				d5.desc = "Extremadamente compacta con cerrojo telescópico. Cadencia feroz de 950 disparos por minuto para dominar esquinas y túneles.";
				d5.modelPath = "Models/Weapons/SMG/Weapon.kv6";
				d5.sightPath = "";
				d5.customColor = MakeVector4(0.35F, 0.38F, 0.42F, 1.0F);
				d5.tagColor = MakeVector4(0.50F, 0.55F, 0.60F, 1.0F);
				d5.soundPath = "Sounds/Weapons/SMG/V2Third1.opus";
				activeCategoryDefs.push_back(d5);

				WeaponOptionDef d6;
				d6.id = "smg_gold";
				d6.title = "SMG Royale Dorada VIP";
				d6.weaponTypeStr = "Subfusil de Élite / Acabado Dorado";
				d6.caliber = "9mm Gold Imperial";
				d6.ammo = "30 / 120 Tiros";
				d6.rpm = 620;
				d6.damageTorso = 32;
				d6.damageHead = 80;
				d6.accuracy = 0.86F;
				d6.rangeMeters = 48.0F;
				d6.mobility = 0.94F;
				d6.desc = "Edición exclusiva para comandantes bañada en oro de 24 quilates con mira reflex grabada. Destello dorado al disparar en combate.";
				d6.modelPath = "Models/Weapons/SMG/Weapon.kv6";
				d6.sightPath = "Models/Weapons/SMG/SightReflex.kv6";
				d6.customColor = MakeVector4(1.0F, 0.84F, 0.20F, 1.0F);
				d6.tagColor = MakeVector4(1.0F, 0.84F, 0.15F, 1.0F);
				d6.soundPath = "Sounds/Weapons/SMG/V2Third2.opus";
				activeCategoryDefs.push_back(d6);
			} else if (sub == 1) {
				// --- Rifle ---
				WeaponOptionDef d1;
				d1.id = "rifle_classic";
				d1.title = "Rifle Semiauto M1";
				d1.weaponTypeStr = "Fusil de Asalto Semiautomático";
				d1.caliber = "7.62x51mm NATO";
				d1.ammo = "10 / 50 Tiros";
				d1.rpm = 240;
				d1.damageTorso = 52;
				d1.damageHead = 100;
				d1.accuracy = 0.96F;
				d1.rangeMeters = 75.0F;
				d1.mobility = 0.88F;
				d1.desc = "Fusil de servicio reglamentario de alta potencia. Capaz de abatir enemigos de un único disparo crítico certero a la cabeza.";
				d1.modelPath = "Models/Weapons/Rifle/Weapon.kv6";
				d1.customColor = MakeVector4(0.80F, 0.80F, 0.80F, 1.0F);
				d1.tagColor = MakeVector4(0.35F, 0.60F, 0.85F, 1.0F);
				d1.soundPath = "Sounds/Weapons/SMG/V2Third1.opus";
				activeCategoryDefs.push_back(d1);

				WeaponOptionDef d2;
				d2.id = "rifle_ak47";
				d2.title = "AK-47 Kalashnikov";
				d2.weaponTypeStr = "Fusil de Asalto Automático Pesado";
				d2.caliber = "7.62x39mm Soviético";
				d2.ammo = "30 / 90 Tiros";
				d2.rpm = 600;
				d2.damageTorso = 42;
				d2.damageHead = 100;
				d2.accuracy = 0.82F;
				d2.rangeMeters = 65.0F;
				d2.mobility = 0.85F;
				d2.desc = "Culata de madera con mecanismo de gas indestructible. Penetración contundente de bloques y defensas a medio y largo alcance.";
				d2.modelPath = "Models/Weapons/Rifle/Weapon.kv6";
				d2.customColor = MakeVector4(0.50F, 0.32F, 0.18F, 1.0F);
				d2.tagColor = MakeVector4(0.65F, 0.35F, 0.15F, 1.0F);
				d2.soundPath = "Sounds/Weapons/SMG/V2Third4.opus";
				activeCategoryDefs.push_back(d2);

				WeaponOptionDef d3;
				d3.id = "rifle_m4a1";
				d3.title = "M4A1 Carabina Táctica";
				d3.weaponTypeStr = "Carabina de Precisión Modular";
				d3.caliber = "5.56x45mm NATO";
				d3.ammo = "30 / 120 Tiros";
				d3.rpm = 720;
				d3.damageTorso = 36;
				d3.damageHead = 90;
				d3.accuracy = 0.92F;
				d3.rangeMeters = 68.0F;
				d3.mobility = 0.92F;
				d3.desc = "Carabina táctica con mira reflex integrada. Excelente control de ráfaga y dispersión mínima en combates abiertos.";
				d3.modelPath = "Models/Weapons/Rifle/Weapon.kv6";
				d3.sightPath = "Models/Weapons/Rifle/SightReflex.kv6";
				d3.customColor = MakeVector4(0.20F, 0.22F, 0.25F, 1.0F);
				d3.tagColor = MakeVector4(0.20F, 0.45F, 0.85F, 1.0F);
				d3.soundPath = "Sounds/Weapons/SMG/V2Third2.opus";
				activeCategoryDefs.push_back(d3);

				WeaponOptionDef d4;
				d4.id = "rifle_awp";
				d4.title = "AWP Francotirador Camo";
				d4.weaponTypeStr = "Rifle de Precisión Cerrojo Sniper";
				d4.caliber = ".338 Lapua Magnum";
				d4.ammo = "5 / 25 Tiros";
				d4.rpm = 45;
				d4.damageTorso = 85;
				d4.damageHead = 100;
				d4.accuracy = 0.99F;
				d4.rangeMeters = 95.0F;
				d4.mobility = 0.75F;
				d4.desc = "Calibre magnum devastador con camuflaje verde francotirador. Alcance extremo y precisión milimétrica.";
				d4.modelPath = "Models/Weapons/Rifle/Weapon.kv6";
				d4.customColor = MakeVector4(0.25F, 0.38F, 0.22F, 1.0F);
				d4.tagColor = MakeVector4(0.20F, 0.60F, 0.25F, 1.0F);
				d4.soundPath = "Sounds/Weapons/SMG/V2Third3.opus";
				activeCategoryDefs.push_back(d4);
			} else if (sub == 2) {
				// --- Escopeta (Shotgun) ---
				WeaponOptionDef d1;
				d1.id = "shotgun_classic";
				d1.title = "Escopeta Corredera 12G";
				d1.weaponTypeStr = "Escopeta de Bomba Estándar";
				d1.caliber = "Calibre 12 Postas";
				d1.ammo = "6 / 36 Cartuchos";
				d1.rpm = 80;
				d1.damageTorso = 90;
				d1.damageHead = 100;
				d1.accuracy = 0.60F;
				d1.rangeMeters = 24.0F;
				d1.mobility = 0.90F;
				d1.desc = "Escopeta tradicional de bombeo calibre 12. Devastadora a corta distancia, capaz de limpiar trincheras enteras.";
				d1.modelPath = "Models/Weapons/Shotgun/Weapon.kv6";
				d1.customColor = MakeVector4(0.80F, 0.80F, 0.80F, 1.0F);
				d1.tagColor = MakeVector4(0.85F, 0.35F, 0.20F, 1.0F);
				d1.soundPath = "Sounds/Weapons/SMG/V2Third1.opus";
				activeCategoryDefs.push_back(d1);

				WeaponOptionDef d2;
				d2.id = "shotgun_spas12";
				d2.title = "SPAS-12 Asalto Militar";
				d2.weaponTypeStr = "Escopeta Táctica de Combate";
				d2.caliber = "Calibre 12 Breaching";
				d2.ammo = "8 / 32 Cartuchos";
				d2.rpm = 120;
				d2.damageTorso = 85;
				d2.damageHead = 100;
				d2.accuracy = 0.68F;
				d2.rangeMeters = 28.0F;
				d2.mobility = 0.92F;
				d2.desc = "Escopeta táctica militar con mira reflex y cañón ventilado. Mayor cadencia y capacidad ampliada de cartuchos.";
				d2.modelPath = "Models/Weapons/Shotgun/Weapon.kv6";
				d2.sightPath = "Models/Weapons/Shotgun/SightReflex.kv6";
				d2.customColor = MakeVector4(0.18F, 0.20F, 0.22F, 1.0F);
				d2.tagColor = MakeVector4(0.20F, 0.25F, 0.35F, 1.0F);
				d2.soundPath = "Sounds/Weapons/SMG/V2Third2.opus";
				activeCategoryDefs.push_back(d2);

				WeaponOptionDef d3;
				d3.id = "shotgun_double";
				d3.title = "Recortada Doble Cañón";
				d3.weaponTypeStr = "Escopeta Recortada de Bolsillo";
				d3.caliber = "Calibre 12 Magnum";
				d3.ammo = "2 / 20 Cartuchos";
				d3.rpm = 200;
				d3.damageTorso = 100;
				d3.damageHead = 100;
				d3.accuracy = 0.45F;
				d3.rangeMeters = 16.0F;
				d3.mobility = 1.0F;
				d3.desc = "Doble cañón recortado para máxima movilidad y dispersión masiva a quemarropa.";
				d3.modelPath = "Models/Weapons/Shotgun/WeaponNoPump.kv6";
				d3.customColor = MakeVector4(0.48F, 0.28F, 0.16F, 1.0F);
				d3.tagColor = MakeVector4(0.60F, 0.30F, 0.15F, 1.0F);
				d3.soundPath = "Sounds/Weapons/SMG/V2Third4.opus";
				activeCategoryDefs.push_back(d3);
			} else if (sub == 3) {
				// --- Pala (Spade) ---
				WeaponOptionDef d1;
				d1.id = "spade_military";
				d1.title = "Pala Militar de Trinchera";
				d1.weaponTypeStr = "Herramienta de Zapa y Combate";
				d1.caliber = "Acero Templado Vóxel";
				d1.ammo = "Cuerpo a Cuerpo";
				d1.rpm = 150;
				d1.damageTorso = 35;
				d1.damageHead = 100;
				d1.accuracy = 1.0F;
				d1.rangeMeters = 2.5F;
				d1.mobility = 1.0F;
				d1.desc = "Herramienta estándar de excavación militar y golpe contundente cuerpo a cuerpo.";
				d1.modelPath = "Models/Weapons/Spade/Spade.kv6";
				d1.customColor = MakeVector4(0.80F, 0.80F, 0.80F, 1.0F);
				d1.tagColor = MakeVector4(0.40F, 0.60F, 0.30F, 1.0F);
				d1.soundPath = "Sounds/Weapons/SMG/V2Third1.opus";
				activeCategoryDefs.push_back(d1);

				WeaponOptionDef d2;
				d2.id = "spade_pickaxe";
				d2.title = "Pico Minero Clásico";
				d2.weaponTypeStr = "Herramienta de Demolición Pesada";
				d2.caliber = "Hierro Forjado";
				d2.ammo = "Demolición";
				d2.rpm = 130;
				d2.damageTorso = 40;
				d2.damageHead = 100;
				d2.accuracy = 1.0F;
				d2.rangeMeters = 2.5F;
				d2.mobility = 0.96F;
				d2.desc = "Pico clásico de minero para demolición acelerada de muros y búnkeres enemigos.";
				d2.modelPath = "Models/Weapons/Spade/Pickaxe.kv6";
				d2.customColor = MakeVector4(0.80F, 0.80F, 0.80F, 1.0F);
				d2.tagColor = MakeVector4(0.70F, 0.50F, 0.20F, 1.0F);
				d2.soundPath = "Sounds/Weapons/SMG/V2Third2.opus";
				activeCategoryDefs.push_back(d2);
			}

			// Read current equipped ID for this category
			std::string eqId = (sub == 0) ? std::string(cg_mod_weapon_smg)
			                : (sub == 1) ? std::string(cg_mod_weapon_rifle)
			                : (sub == 2) ? std::string(cg_mod_weapon_shotgun)
			                             : std::string(cg_mod_weapon_spade);

			if (eqId.empty())
				eqId = activeCategoryDefs[0].id;

			// Select the first one or currently equipped
			bool found = false;
			for (const auto& d : activeCategoryDefs) {
				if (d.id == eqId) {
					SelectWeapon(d.id);
					found = true;
					break;
				}
			}
			if (!found && !activeCategoryDefs.empty()) {
				SelectWeapon(activeCategoryDefs[0].id);
			}

			RebuildWeaponList();
		}

		void ModWeaponSelectScreen::RebuildWeaponList() {
			if (!weaponListPanel)
				return;

			// Clear previous rows
			for (auto* r : weaponRows) {
				if (r && r->GetParent())
					r->GetParent()->RemoveChild(r);
			}
			weaponRows.clear();

			UIManager* manager = &GetManager();
			float contentW = weaponListPanel->size.x;
			float rowH = 38.0F;
			float rowGap = 4.0F;

			std::string curEquipped = (activeWeaponSubCategory == 0) ? std::string(cg_mod_weapon_smg)
			                       : (activeWeaponSubCategory == 1) ? std::string(cg_mod_weapon_rifle)
			                       : (activeWeaponSubCategory == 2) ? std::string(cg_mod_weapon_shotgun)
			                                                        : std::string(cg_mod_weapon_spade);

			float y = 0.0F;
			for (const auto& d : activeCategoryDefs) {
				bool isSel = (d.id == selectedWeaponId);
				bool isEq = (d.id == curEquipped);

				Handle<WeaponOptionRow> row =
				    Handle<WeaponOptionRow>::New(manager, d, isSel, isEq);
				row->SetBounds(AABB2(0.0F, y, contentW, rowH));

				row->activated = [this, rowPtr = row.GetPointerOrNull()](UIElement&) {
					SelectWeapon(rowPtr->def.id);
				};

				weaponRows.push_back(row.GetPointerOrNull());
				weaponListPanel->AddChild(row.GetPointerOrNull());
				y += rowH + rowGap;
			}
		}

		void ModWeaponSelectScreen::SelectWeapon(const std::string& id) {
			selectedWeaponId = id;
			for (const auto& d : activeCategoryDefs) {
				if (d.id == id) {
					currentDef = d;
					break;
				}
			}

			// Update row selections
			for (auto* r : weaponRows) {
				if (r) {
					r->isSelected = (r->def.id == id);
				}
			}

			// Update equip button text
			std::string curEquipped = (activeWeaponSubCategory == 0) ? std::string(cg_mod_weapon_smg)
			                       : (activeWeaponSubCategory == 1) ? std::string(cg_mod_weapon_rifle)
			                       : (activeWeaponSubCategory == 2) ? std::string(cg_mod_weapon_shotgun)
			                                                        : std::string(cg_mod_weapon_spade);

			if (equipButton) {
				if (id == curEquipped) {
					equipButton->caption = _Tr("MainScreen", "✓ ARMA EQUIPADA & ACTIVA");
				} else {
					equipButton->caption = _Tr("MainScreen", "★ EQUIPAR ESTA ARMA");
				}
			}

			UpdatePreviewModels();
		}

		void ModWeaponSelectScreen::EquipSelectedWeapon() {
			if (activeWeaponSubCategory == 0)
				cg_mod_weapon_smg = selectedWeaponId;
			else if (activeWeaponSubCategory == 1)
				cg_mod_weapon_rifle = selectedWeaponId;
			else if (activeWeaponSubCategory == 2)
				cg_mod_weapon_shotgun = selectedWeaponId;
			else if (activeWeaponSubCategory == 3)
				cg_mod_weapon_spade = selectedWeaponId;

			Settings::GetInstance()->Flush();

			// Play tactile switch sound
			try {
				client::IAudioDevice& audio = GetManager().GetAudioDevice();
				Handle<client::IAudioChunk> c = audio.RegisterSound("Sounds/Weapons/SwitchLocal.opus");
				if (c)
					audio.PlayLocal(c.GetPointerOrNull(), client::AudioParam());
			} catch (...) {}

			// Update row tags
			for (auto* r : weaponRows) {
				if (r) {
					r->isEquipped = (r->def.id == selectedWeaponId);
				}
			}

			if (equipButton)
				equipButton->caption = _Tr("MainScreen", "✓ ARMA EQUIPADA & ACTIVA");
		}

		void ModWeaponSelectScreen::PlayTestSound() {
			try {
				client::IAudioDevice& audio = GetManager().GetAudioDevice();
				std::string sPath = currentDef.soundPath;
				if (sPath.empty())
					sPath = "Sounds/Weapons/SMG/V2Third1.opus";

				Handle<client::IAudioChunk> c = audio.RegisterSound(sPath.c_str());
				if (c) {
					client::AudioParam param;
					param.volume = 0.85F;
					audio.PlayLocal(c.GetPointerOrNull(), param);
				}
			} catch (...) {}
		}

		void ModWeaponSelectScreen::UpdatePreviewModels() {
			client::IRenderer& r = GetManager().GetRenderer();

			if (loadedModelPath != currentDef.modelPath && !currentDef.modelPath.empty()) {
				try {
					previewWeaponModel = r.RegisterModel(currentDef.modelPath.c_str());
					loadedModelPath = currentDef.modelPath;
				} catch (...) {
					previewWeaponModel.Set(nullptr, false);
				}
			}

			if (loadedSightPath != currentDef.sightPath) {
				if (!currentDef.sightPath.empty()) {
					try {
						previewSightModel = r.RegisterModel(currentDef.sightPath.c_str());
						loadedSightPath = currentDef.sightPath;
					} catch (...) {
						previewSightModel.Set(nullptr, false);
					}
				} else {
					previewSightModel.Set(nullptr, false);
					loadedSightPath = "";
				}
			}
		}

		void ModWeaponSelectScreen::Render3DWeapon(const client::SceneDefinition& sceneDef, float dt) {
			client::IRenderer& r = GetManager().GetRenderer();
			if (!previewWeaponModel)
				return;

			showcaseTime += dt;
			weaponYaw += dt * 1.5F; // smooth continuous 360 rotation

			float sw = GetManager().screenWidth;
			float sh = GetManager().screenHeight;

			float dlgW = std::min(840.0F, sw * 0.94F);
			float dlgH = std::min(580.0F, sh - 32.0F);
			float dlgX = (sw - dlgW) * 0.5F;
			float dlgY = (sh - dlgH) * 0.5F;

			float sidebarW = 125.0F;
			float midW = 275.0F;

			float rightX = dlgX + sidebarW + 16.0F + midW + 16.0F;
			float rightW = dlgW - (sidebarW + 16.0F + midW + 28.0F);

			float showcaseX = rightX;
			float showcaseY = dlgY + 45.0F;
			float showcaseW = rightW;
			float showcaseH = 205.0F;

			// Project center of showcase box into 3D camera space
			float centerX = showcaseX + showcaseW * 0.5F;
			float centerY = showcaseY + showcaseH * 0.5F;

			float ndcX = (centerX / sw) * 2.0F - 1.0F;
			float ndcY = 1.0F - (centerY / sh) * 2.0F;

			float dist = 3.6F;
			float tanHalfFov = tanf(DEG2RAD(15.0F)); // 30 deg vertical FOV / 2
			float aspect = sw / sh;

			float rightOffset = ndcX * tanHalfFov * aspect * dist;
			float upOffset = ndcY * tanHalfFov * dist;

			Vector3 orig = sceneDef.viewOrigin;
			Vector3 dir = sceneDef.viewAxis[2];  // camera forward
			Vector3 side = sceneDef.viewAxis[0]; // camera right
			Vector3 up = sceneDef.viewAxis[1];   // camera up

			Vector3 weaponPos = orig + dir * dist + side * rightOffset + up * upOffset;

			// Floating bob and subtle tilt
			float bob = sinf(showcaseTime * 2.2F) * 0.025F;
			float pitch = sinf(showcaseTime * 1.6F) * 0.05F - 0.03F;

			// Add dynamic showcase light
			{
				client::DynamicLightParam light;
				light.type = client::DynamicLightTypePoint;
				light.origin = weaponPos + up * 0.75F - dir * 0.9F + side * 0.2F;
				light.radius = 4.8F;
				if (selectedWeaponId.find("gold") != std::string::npos) {
					light.color = MakeVector3(1.0F, 0.88F, 0.35F); // Rich gold
				} else {
					light.color = MakeVector3(0.85F, 0.95F, 1.0F);  // High-tech crisp cyan-white
				}
				r.AddLight(light);
			}

			float scale = (activeWeaponSubCategory == 3) ? 0.075F : 0.062F;

			Matrix4 mat = Matrix4::Translate(weaponPos + up * bob)
			    * Matrix4::Rotate(MakeVector3(0, 0, 1), weaponYaw)
			    * Matrix4::Rotate(MakeVector3(1, 0, 0), pitch)
			    * Matrix4::Rotate(MakeVector3(0, 1, 0), 0.18F)
			    * Matrix4::Scale(scale)
			    * Matrix4::Scale(-1, -1, 1);

			client::ModelRenderParam param;
			param.matrix = mat;
			if (currentDef.customColor.w > 0.0F) {
				param.customColor = MakeVector3(currentDef.customColor.x, currentDef.customColor.y, currentDef.customColor.z);
			}
			r.RenderModel(*previewWeaponModel, param);

			if (previewSightModel) {
				client::ModelRenderParam sp;
				sp.matrix = mat * Matrix4::Translate(0.0F, 0.0F, -0.15F);
				r.RenderModel(*previewSightModel, sp);
			}
		}

		void ModWeaponSelectScreen::Run() {
			owner->enable = false;
			owner->GetParent()->AddChild(this);
			if (mainScreenUI)
				mainScreenUI->SetActiveWeaponScreen(this);
		}

		void ModWeaponSelectScreen::Close() {
			if (mainScreenUI)
				mainScreenUI->SetActiveWeaponScreen(nullptr);
			owner->enable = true;
			if (GetParent())
				GetParent()->RemoveChild(this);
		}

		void ModWeaponSelectScreen::HotKey(const std::string& key) {
			if (IsEnabled() && (key == "Escape" || key == "Back")) {
				Close();
				return;
			}
			UIElement::HotKey(key);
		}

		void ModWeaponSelectScreen::Render() {
			client::IRenderer& r = GetManager().GetRenderer();
			float sw = GetManager().screenWidth;
			float sh = GetManager().screenHeight;

			float dlgW = std::min(840.0F, sw * 0.94F);
			float dlgH = std::min(580.0F, sh - 32.0F);
			float dlgX = (sw - dlgW) * 0.5F;
			float dlgY = (sh - dlgH) * 0.5F;

			// Dimmed background overlay
			SetColorNP(r, MakeVector4(0.0F, 0.0F, 0.0F, 0.65F));
			r.DrawFilledRect(0.0F, 0.0F, sw, sh);

			// Main dialog container
			SetColorNP(r, MakeVector4(0.04F, 0.06F, 0.09F, 0.97F));
			r.DrawFilledRect(dlgX, dlgY, dlgX + dlgW, dlgY + dlgH);

			// Dialog border
			SetColorNP(r, MakeVector4(0.20F, 0.35F, 0.55F, 0.85F));
			r.DrawOutlinedRect(dlgX, dlgY, dlgX + dlgW, dlgY + dlgH);

			// Top glowing neon bar
			SetColorNP(r, MakeVector4(0.25F, 0.70F, 1.0F, 1.0F));
			r.DrawFilledRect(dlgX, dlgY, dlgX + dlgW, dlgY + 3.0F);

			// Vertical column separators
			float sidebarW = 125.0F;
			float midW = 275.0F;
			float sep1X = dlgX + sidebarW + 8.0F;
			float sep2X = dlgX + sidebarW + 16.0F + midW + 8.0F;

			SetColorNP(r, MakeVector4(0.15F, 0.22F, 0.32F, 0.60F));
			r.DrawFilledRect(sep1X, dlgY + 40.0F, sep1X + 1.5F, dlgY + dlgH - 12.0F);
			r.DrawFilledRect(sep2X, dlgY + 40.0F, sep2X + 1.5F, dlgY + dlgH - 12.0F);

			client::IFont* font = fontManager ? &fontManager->GetGuiFont() : nullptr;
			Vector4 shadowCol = MakeVector4(0.0F, 0.0F, 0.0F, 0.75F);

			if (font) {
				// Window title
				std::string title = _Tr("MainScreen", "ARMERÍA & MODS — PERSONALIZACIÓN EN TIEMPO REAL");
				font->DrawShadow(title, MakeVector2(dlgX + 16.0F, dlgY + 12.0F), 1.05F,
				                MakeVector4(0.35F, 0.85F, 1.0F, 1.0F), shadowCol);

				std::string sub = _Tr("MainScreen", "Categorías: Armas • Subfusil SMG • Previsualización 3D y estadísticas en vivo");
				font->DrawShadow(sub, MakeVector2(dlgX + 16.0F, dlgY + 28.0F), 0.72F,
				                MakeVector4(0.60F, 0.72F, 0.85F, 0.80F), shadowCol);
			}

			// --- Right Column: Showcase & Live Stats HUD ---
			float rightX = dlgX + sidebarW + 16.0F + midW + 16.0F;
			float rightW = dlgW - (sidebarW + 16.0F + midW + 28.0F);

			float showcaseX = rightX;
			float showcaseY = dlgY + 45.0F;
			float showcaseW = rightW;
			float showcaseH = 205.0F;

			// 3D Showcase viewport container
			SetColorNP(r, MakeVector4(0.03F, 0.05F, 0.08F, 0.85F));
			r.DrawFilledRect(showcaseX, showcaseY, showcaseX + showcaseW, showcaseY + showcaseH);

			SetColorNP(r, MakeVector4(0.22F, 0.40F, 0.65F, 0.75F));
			r.DrawOutlinedRect(showcaseX, showcaseY, showcaseX + showcaseW, showcaseY + showcaseH);

			// Holographic corner ticks
			float tickLen = 8.0F;
			SetColorNP(r, MakeVector4(0.40F, 0.85F, 1.0F, 0.95F));
			r.DrawFilledRect(showcaseX, showcaseY, showcaseX + tickLen, showcaseY + 2.0F);
			r.DrawFilledRect(showcaseX, showcaseY, showcaseX + 2.0F, showcaseY + tickLen);
			r.DrawFilledRect(showcaseX + showcaseW - tickLen, showcaseY, showcaseX + showcaseW, showcaseY + 2.0F);
			r.DrawFilledRect(showcaseX + showcaseW - 2.0F, showcaseY, showcaseX + showcaseW, showcaseY + tickLen);
			r.DrawFilledRect(showcaseX, showcaseY + showcaseH - 2.0F, showcaseX + tickLen, showcaseY + showcaseH);
			r.DrawFilledRect(showcaseX, showcaseY + showcaseH - tickLen, showcaseX + 2.0F, showcaseY + showcaseH);
			r.DrawFilledRect(showcaseX + showcaseW - tickLen, showcaseY + showcaseH - 2.0F, showcaseX + showcaseW, showcaseY + showcaseH);
			r.DrawFilledRect(showcaseX + showcaseW - 2.0F, showcaseY + showcaseH - tickLen, showcaseX + showcaseW, showcaseY + showcaseH);

			if (font) {
				// Pulsing live badge in top-left of showcase
				float pulse = sinf(showcaseTime * 4.0F) * 0.3F + 0.7F;
				SetColorNP(r, MakeVector4(0.20F, 0.90F, 0.35F, pulse));
				r.DrawFilledRect(showcaseX + 10.0F, showcaseY + 9.0F, showcaseX + 16.0F, showcaseY + 15.0F);

				std::string liveTag = "EN VIVO (3D SHOWCASE)";
				font->DrawShadow(liveTag, MakeVector2(showcaseX + 22.0F, showcaseY + 7.0F), 0.70F,
				                MakeVector4(0.35F, 0.95F, 0.45F, 0.95F), shadowCol);

				std::string orbTag = "[ VISTA ORBITAL 360° ]";
				Vector2 orbSz = font->Measure(orbTag) * 0.65F;
				font->DrawShadow(orbTag, MakeVector2(showcaseX + showcaseW - orbSz.x - 8.0F, showcaseY + 7.0F), 0.65F,
				                MakeVector4(0.40F, 0.80F, 1.0F, 0.85F), shadowCol);

				// Weapon Title & Type Header
				float infoY = showcaseY + showcaseH + 10.0F;
				font->DrawShadow(currentDef.title, MakeVector2(rightX, infoY), 1.15F,
				                MakeVector4(1.0F, 1.0F, 1.0F, 1.0F), shadowCol);

				std::string typeLine = currentDef.weaponTypeStr + " • " + currentDef.caliber;
				font->DrawShadow(typeLine, MakeVector2(rightX, infoY + 20.0F), 0.74F,
				                MakeVector4(0.35F, 0.85F, 1.0F, 0.95F), shadowCol);

				// --- Live Statistics HUD Bars ---
				float statY = infoY + 38.0F;
				float statW = rightW;

				auto drawStatBar = [&](float yPos, const std::string& label, const std::string& valStr, float ratio, const Vector4& fillCol) {
					font->DrawShadow(label, MakeVector2(rightX, yPos), 0.70F,
					                MakeVector4(0.70F, 0.80F, 0.92F, 0.85F), shadowCol);
					Vector2 vSz = font->Measure(valStr) * 0.70F;
					font->DrawShadow(valStr, MakeVector2(rightX + statW - vSz.x, yPos), 0.70F,
					                MakeVector4(0.95F, 0.98F, 1.0F, 0.95F), shadowCol);

					float barY = yPos + 12.0F;
					SetColorNP(r, MakeVector4(0.06F, 0.09F, 0.14F, 0.85F));
					r.DrawFilledRect(rightX, barY, rightX + statW, barY + 6.0F);
					SetColorNP(r, MakeVector4(0.18F, 0.25F, 0.36F, 0.60F));
					r.DrawOutlinedRect(rightX, barY, rightX + statW, barY + 6.0F);

					float fillW = std::max(2.0F, statW * Clamp(ratio, 0.0F, 1.0F));
					SetColorNP(r, fillCol);
					r.DrawFilledRect(rightX + 1.0F, barY + 1.0F, rightX + fillW - 1.0F, barY + 5.0F);
				};

				float rpmRatio = currentDef.rpm / 1000.0F;
				drawStatBar(statY, "Cadencia de Fuego (RPM)", std::to_string(currentDef.rpm) + " RPM",
				            rpmRatio, MakeVector4(0.35F, 0.75F, 1.0F, 1.0F));

				std::string dmgStr = std::to_string(currentDef.damageTorso) + " HP (Cuerpo) / " + std::to_string(currentDef.damageHead) + " HP (Cabeza)";
				drawStatBar(statY + 22.0F, "Daño por Impacto", dmgStr,
				            currentDef.damageTorso / 80.0F, MakeVector4(1.0F, 0.40F, 0.35F, 1.0F));

				drawStatBar(statY + 44.0F, "Precisión & Control", std::to_string(int(currentDef.accuracy * 100.0F)) + "%",
				            currentDef.accuracy, MakeVector4(0.40F, 0.88F, 0.50F, 1.0F));

				drawStatBar(statY + 66.0F, "Alcance Efectivo", std::to_string(int(currentDef.rangeMeters)) + " Metros",
				            currentDef.rangeMeters / 80.0F, MakeVector4(1.0F, 0.75F, 0.25F, 1.0F));

				drawStatBar(statY + 88.0F, "Movilidad & Manejo", std::to_string(int(currentDef.mobility * 100.0F)) + "%",
				            currentDef.mobility, MakeVector4(0.70F, 0.50F, 1.0F, 1.0F));

				// Weapon Lore / Tactical Note Box
				float noteY = statY + 114.0F;
				float noteH = 46.0F;
				SetColorNP(r, MakeVector4(0.06F, 0.09F, 0.14F, 0.70F));
				r.DrawFilledRect(rightX, noteY, rightX + statW, noteY + noteH);
				SetColorNP(r, MakeVector4(0.18F, 0.26F, 0.38F, 0.50F));
				r.DrawOutlinedRect(rightX, noteY, rightX + statW, noteY + noteH);

				font->DrawShadow(currentDef.desc, MakeVector2(rightX + 8.0F, noteY + 7.0F), 0.70F,
				                MakeVector4(0.75F, 0.85F, 0.95F, 0.90F), shadowCol);
			}

			// Render child elements (buttons, subcategory bar, list)
			UIElement::Render();
		}
	} // namespace gui
} // namespace spades
