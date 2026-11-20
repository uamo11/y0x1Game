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

#pragma once

#include <string>
#include <vector>

#include <Gui/UI/MainScreen/DemoListView.h>
#include <Gui/UI/MainScreen/ModListView.h>
#include <Gui/UI/MainScreen/ServerListView.h>
#include <Gui/UI/Widgets/Button.h>
#include <Gui/UI/Widgets/DropDownList.h>
#include <Gui/UI/Widgets/Field.h>
#include <Gui/UI/Widgets/GuiIcons.h>
#include <Gui/UI/Widgets/Label.h>
#include <Gui/UI/Widgets/TabStrip.h>

namespace spades {
	namespace gui {
		class MainScreenUI;
		class MainScreenHelper;
		class ModsScreenHelper;

		/** A push button with a procedural GUI icon and optional caption. */
		class MenuIconButton : public ui::Button {
		public:
			GuiIconType iconType = GuiIconType::None;
			bool iconOnly = false;

			MenuIconButton(ui::UIManager* manager, GuiIconType type = GuiIconType::None, bool iconOnly = false)
			    : ui::Button(manager), iconType(type), iconOnly(iconOnly) {
				if (!iconOnly)
					alignment = MakeVector2(0.0F, 0.5F);
			}
			void Render() override;
		};

		/** A `SimpleButton` decorated with the refresh arrow icon. */
		class RefreshButton : public ui::SimpleButton {
		public:
			RefreshButton(ui::UIManager* manager) : ui::SimpleButton(manager) {}
			void Render() override;
		};

		/** A toggle `SimpleButton` used for protocol selection/filtering. */
		class ProtocolButton : public ui::SimpleButton {
		public:
			ProtocolButton(ui::UIManager* manager) : ui::SimpleButton(manager) { toggle = true; }
		};

		/** Transparent container panel used for tab switching. */
		class TabPanel : public ui::UIElement {
		public:
			TabPanel(ui::UIManager* manager) : ui::UIElement(manager) {}
		};

		/** Modal dialog for renaming a demo file. */
		class RenameScreen : public ui::UIElement {
			ui::UIElement* owner; // weak
			ui::Field* nameField; // weak; owned as a child

			void OnConfirm(ui::UIElement& sender);
			void OnCancel(ui::UIElement& sender);

		public:
			ui::EventHandler closed;
			bool result = false;
			std::string newName;

			RenameScreen(ui::UIElement* owner, const std::string& currentName);

			void Close();
			void Run();
			void HotKey(const std::string& key) override;
		};

		/**
		 * The part of the main menu that survives a rebuild. Everything else either
		 * lives in config (the quick-connect address, the protocol choice) or in the
		 * helper (the server list), and comes back on its own.
		 */
		struct MainScreenMainMenuState {
			int activeTabIndex = 0;
		};

		enum class ServerVersionFilter {
			All = 0,
			v075,
			v076,
			vY
		};

		/** The main menu: server browser, demo browser, mod manager, footer. */
		class MainScreenMainMenu : public ui::UIElement {
			MainScreenUI* ui;         // weak
			MainScreenHelper* helper; // weak

			ui::Field* addressField = nullptr;
			MenuIconButton* connectButton = nullptr;
			MenuIconButton* toggleIpButton = nullptr;
			MenuIconButton* skinsButton = nullptr;
			MenuIconButton* modsButton = nullptr;
			ui::Label* goldNoticeLabel = nullptr;
			bool showDirectConnect = false;

			void UpdateDirectConnectVisibility();

			ui::Button* versionFilterButton = nullptr;
			ServerVersionFilter currentVersionFilter = ServerVersionFilter::All;
			void OpenVersionDropdown();

			MenuIconButton* searchFilterButton = nullptr;
			bool showSearchFilter = false;
			void UpdateSearchFilterVisibility();

			ui::Label* filterLabel = nullptr;
			ui::Button* filterFullButton = nullptr;
			ui::Field* filterField = nullptr;

			ui::ListView* serverList = nullptr;
			MainScreenServerListLoadingView* loadingView = nullptr;
			MainScreenServerListErrorView* errorView = nullptr;
			bool loading = false, loaded = false;

			Handle<ServerListModel> currentServerListModel;
			int serverListUpdateTimer = 5;
			std::string selectedMapName;

			TabPanel* serverPanel = nullptr;
			TabPanel* worldsPanel = nullptr;

			// Worlds tab state
			ui::ListView* worldsList = nullptr;
			std::string selectedWorldMap = "LAbeach.vxl";
			void LoadWorldList();
			void WorldListItemActivated(const std::string& mapName);
			void WorldListItemDoubleClicked(const std::string& mapName);
			void PlaySelectedWorld();

			// Demo tab state
			TabPanel* demoPanel = nullptr;
			ui::ListView* demoList = nullptr;
			Handle<DemoListModel> currentDemoListModel;
			std::string selectedDemoPath;
			ui::Field* demoNameField;
			ui::Button* demoPlayButton;
			ui::Button* demoDeleteButton;
			ui::Button* demoRenameButton;

			// Demo list column widths (pixels)
			float demoDateColWidth;
			float demoModeColWidth;
			float demoMapColWidth;
			float demoSizeColWidth;
			float demoContentsWidth;

			// Mods tab state
			Handle<ModsScreenHelper> modsHelper;
			TabPanel* modsPanel;
			ui::ListView* modsList;
			ui::Button* modsDownloadButton;
			ui::Button* modsApplyButton;
			ui::Button* modsResetButton;
			ui::Label* modsStatusLabel;
			ModsProgressBar* modsProgressBar;
			bool modsDownloading = false;
			bool modsDirty = false; // enabled set changed this session; restart to apply
			float modsCheckColWidth;
			float modsOrderColWidth;
			float modsTagColWidth;
			float modsNameColWidth;
			float modsAuthorColWidth;
			float modsSizeColWidth;

			void OnTabChanged(ui::UIElement& sender);
			int EnabledIndex(const std::vector<std::string>& enabled, const std::string& name);
			void UpdateModsStatus();
			void OnDownloadModsPressed(ui::UIElement& sender);
			void OnDownloadConfirmed(ui::UIElement& sender);
			void OnApplyModsPressed(ui::UIElement& sender);
			void OnResetModsPressed(ui::UIElement& sender);
			void OnResetConfirmed(ui::UIElement& sender);
			void OnModToggle(const std::string& modName);
			void CheckModsRefresh();

			void DemoListItemActivated(const std::string& filename);
			void DemoListItemDoubleClicked(const std::string& filename);
			void PlaySelectedDemo();
			void OnPlayDemoPressed(ui::UIElement& sender);
			void OnDeleteDemoPressed(ui::UIElement& sender);
			void OnDeleteConfirmClosed(ui::UIElement& sender);
			void OnRenameDemoPressed(ui::UIElement& sender);
			void OnRenameScreenClosed(ui::UIElement& sender);

			void ServerListItemActivated(MainScreenServerItem& item);
			void ServerListItemDoubleClicked(MainScreenServerItem& item);
			void ServerListItemRightClicked(MainScreenServerItem& item);

			void SortServerList(int keyId);
			void UpdateServerList(bool refresh = true);
			void CheckServerList();

			void OnAddressChanged(ui::UIElement& sender);
			void SetProtocolVersion(int ver);

			void OnSkinsPressed(ui::UIElement& sender);
			void OnModsPressed(ui::UIElement& sender);
			void Connect();

		public:
			MainScreenMainMenu(MainScreenUI* ui);

			void LoadServerList();
			void LoadDemoList();
			void LoadModList();

			MainScreenMainMenuState GetState();
			void SetState(const MainScreenMainMenuState& state);

			void ReturnToLobby();

			void HotKey(const std::string& key) override;
			void Render() override;
		};
	} // namespace gui
} // namespace spades
