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

#pragma once

#include <string>
#include <vector>

#include <Core/Math.h>
#include <Gui/UI/Framework/UIElement.h>
#include <Gui/UI/Widgets/Button.h>
#include <Gui/UI/Widgets/GuiIcons.h>
#include <Gui/UI/Widgets/Label.h>

namespace spades {
	namespace client {
		class FontManager;
		class IModel;
		struct SceneDefinition;
	}
	namespace gui {
		class MainScreenUI;
		class ModTabButton;
		class WeaponOptionRow;
		class WeaponSubCategoryBtn;

		struct WeaponOptionDef {
			std::string id;
			std::string title;
			std::string weaponTypeStr; // e.g. "Subfusil Automático (Ametralladora)"
			std::string caliber;       // e.g. "9x19mm Parabellum"
			std::string ammo;          // e.g. "30 / 120 Tiros"
			int rpm = 0;               // e.g. 720
			int damageTorso = 0;       // e.g. 31
			int damageHead = 0;        // e.g. 75
			float accuracy = 0.0F;     // 0.0 to 1.0
			float rangeMeters = 0.0F;  // e.g. 50.0F
			float mobility = 0.0F;     // 0.0 to 1.0
			std::string desc;
			std::string modelPath;     // e.g. "Models/Weapons/SMG/Weapon.kv6"
			std::string sightPath;     // e.g. "Models/Weapons/SMG/SightReflex.kv6"
			Vector4 customColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.0F);
			Vector4 tagColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.0F);
			std::string soundPath;
		};

		/**
		 * High-performance real-time Weapon Customization & Mod Selection Screen.
		 * Features category sidebar (Armas, Accesorios, Sonidos, Paquetes .pak),
		 * subcategories (Ametralladora, Rifle, Escopeta, Pala, Granada),
		 * weapon variant picker, real-time 3D rotating model showcase, and live combat statistics.
		 */
		class ModWeaponSelectScreen : public ui::UIElement {
			ui::UIElement* owner;             // weak
			client::FontManager* fontManager; // weak
			MainScreenUI* mainScreenUI;       // weak

			int activeMainCategory = 0;       // 0 = Armas, 1 = Accesorios, 2 = Sonidos FX, 3 = Paquetes .pak
			int activeWeaponSubCategory = 0;  // 0 = Ametralladora (SMG), 1 = Rifle, 2 = Escopeta, 3 = Pala, 4 = Granada

			std::string selectedWeaponId;
			WeaponOptionDef currentDef;

			float showcaseTime = 0.0F;
			float weaponYaw = 0.0F;

			// 3D Model Handles
			Handle<client::IModel> previewWeaponModel;
			Handle<client::IModel> previewSightModel;
			std::string loadedModelPath;
			std::string loadedSightPath;

			// Sidebar tabs
			ModTabButton* tabWeapons = nullptr;
			ModTabButton* tabAccessories = nullptr;
			ModTabButton* tabAudio = nullptr;
			ModTabButton* tabPaks = nullptr;

			// Weapon Subcategory buttons
			WeaponSubCategoryBtn* subSmgBtn = nullptr;
			WeaponSubCategoryBtn* subRifleBtn = nullptr;
			WeaponSubCategoryBtn* subShotgunBtn = nullptr;
			WeaponSubCategoryBtn* subSpadeBtn = nullptr;
			WeaponSubCategoryBtn* subGrenadeBtn = nullptr;

			// Action buttons
			ui::Button* equipButton = nullptr;
			ui::Button* testFireButton = nullptr;
			ui::Button* closeButton = nullptr;

			// Content panels
			ui::UIElement* subCategoryPanel = nullptr;
			ui::UIElement* weaponListPanel = nullptr;
			ui::UIElement* accessoriesPanel = nullptr;
			ui::UIElement* audioPanel = nullptr;
			ui::UIElement* paksPanel = nullptr;

			std::vector<WeaponOptionRow*> weaponRows;
			std::vector<WeaponOptionDef> activeCategoryDefs;

			void SwitchMainCategory(int index);
			void SwitchWeaponSubCategory(int sub);
			void SelectWeapon(const std::string& id);
			void EquipSelectedWeapon();
			void PlayTestSound();
			void RebuildWeaponList();
			void UpdatePreviewModels();

		public:
			ModWeaponSelectScreen(ui::UIElement* owner, client::FontManager* fontManager,
			                      MainScreenUI* mainScreenUI = nullptr);

			void Run();
			void Close();

			void Render3DWeapon(const client::SceneDefinition& sceneDef, float dt);
			void HotKey(const std::string& key) override;
			void Render() override;
		};
	} // namespace gui
} // namespace spades
