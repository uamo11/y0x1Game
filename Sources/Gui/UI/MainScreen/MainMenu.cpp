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

#include <algorithm>
#include <cctype>

#include "MainMenu.h"
#include "MainScreenUI.h"
#include "SkinSelectScreen.h"
#include "ModWeaponSelectScreen.h"
#include <Client/Fonts.h>
#include <Client/IImage.h>
#include <Client/IRenderer.h>
#include <Core/FileManager.h>
#include <Core/Settings.h>
#include <Core/ShellApi.h>
#include <Core/Strings.h>
#include <Gui/MainScreenHelper.h>
#include <Gui/ModsScreenHelper.h>
#include <Gui/UI/Components/UpdateNotice.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/Settings/PreferenceView.h>
#include <Gui/UI/Widgets/DrawUtils.h>
#include <Gui/UI/Widgets/Label.h>
#include <Gui/UI/Widgets/MessageBox.h>

DEFINE_SPADES_SETTING(cg_protocolVersion, "3");
DEFINE_SPADES_SETTING(cg_lastQuickConnectHost, "127.0.0.1");
DEFINE_SPADES_SETTING(cg_serverlistSort, "16385");

namespace spades {
	namespace gui {
		using ui::Button;
		using ui::Field;
		using ui::Label;
		using ui::ListView;
		using ui::ListViewModel;
		using ui::SetColorNP;
		using ui::SimpleTabStrip;
		using ui::UIElement;
		using ui::UIManager;

		namespace {
			bool StringContainsCaseInsensitive(std::string text, std::string pattern) {
				for (char& c : text)
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				for (char& c : pattern)
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				return text.find(pattern) != std::string::npos;
			}
		} // namespace

		// -- MenuIconButton --

		void MenuIconButton::Render() {
			client::IRenderer& r = GetManager().GetRenderer();
			Vector2 pos = GetScreenPosition();
			Vector2 sz = size;

			Vector4 color = MakeVector4(0.0F, 0.0F, 0.0F, 0.60F);
			Vector4 iconCol = MakeVector4(0.92F, 0.92F, 0.92F, 0.95F);

			if (IsEnabled()) {
				if (toggled || (pressed && hover)) {
					color = MakeVector4(0.04F, 0.04F, 0.04F, 0.90F);
					iconCol = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
				} else if (hover) {
					color = MakeVector4(0.12F, 0.12F, 0.12F, 0.80F);
					iconCol = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
				}
			} else {
				color.w *= 0.5F;
				iconCol.w *= 0.4F;
			}

			SetColorNP(r, color);
			r.DrawFilledRect(pos.x + 1, pos.y + 1, pos.x + sz.x - 1, pos.y + sz.y - 1);

			Vector4 borderCol = (hover || toggled) ? MakeVector4(0.70F, 0.70F, 0.70F, 0.85F)
			                                       : MakeVector4(0.0F, 0.0F, 0.0F, 0.90F);
			SetColorNP(r, borderCol);
			r.DrawOutlinedRect(pos.x + 1, pos.y + 1, pos.x + sz.x - 1, pos.y + sz.y - 1);

			if (iconOnly) {
				if (iconType != GuiIconType::None) {
					DrawGuiIcon(r, iconType, pos + sz * 0.5F, iconCol, 1.1F);
				}
				return;
			}

			if (iconType != GuiIconType::None) {
				DrawGuiIcon(r, iconType, MakeVector2(pos.x + 15.0F, pos.y + sz.y * 0.5F), iconCol, 1.1F);
			}

			client::IFont* font = GetFont();
			if (!font)
				return;

			float textStartX = (iconType != GuiIconType::None) ? 28.0F : 8.0F;
			Vector2 txtPos = MakeVector2(pos.x + textStartX, pos.y + (sz.y - font->Measure(caption).y) * 0.5F);
			font->DrawShadow(caption, txtPos, 1.0F,
			                 MakeVector4(1.0F, 1.0F, 1.0F, IsEnabled() ? 1.0F : 0.5F),
			                 MakeVector4(0.0F, 0.0F, 0.0F, 0.3F));

			if (!hotKeyText.empty()) {
				Vector2 hkSize = font->Measure(hotKeyText);
				Vector2 hkPos = MakeVector2(pos.x + sz.x - hkSize.x - 8.0F, pos.y + (sz.y - hkSize.y) * 0.5F);
				font->DrawShadow(hotKeyText, hkPos, 1.0F,
				                 MakeVector4(1.0F, 1.0F, 1.0F, IsEnabled() ? 0.6F : 0.3F),
				                 MakeVector4(0.0F, 0.0F, 0.0F, 0.2F));
			}
		}

		// -- RefreshButton --

		void RefreshButton::Render() {
			SimpleButton::Render();

			client::IRenderer& r = GetManager().GetRenderer();
			Vector2 pos = GetScreenPosition();
			Vector2 sz = size;
			Handle<client::IImage> img = r.RegisterImage("Gfx/UI/Refresh.png");
			r.DrawImage(img, pos + (sz - MakeVector2(16.0F, 16.0F)) * 0.5F);
		}

		// -- RenameScreen --

		RenameScreen::RenameScreen(UIElement* owner, const std::string& currentName)
		    : UIElement(&owner->GetManager()), owner(owner) {
			SetFont(GetManager().GetRootElement().GetFont());
			SetBounds(owner->GetBounds());

			UIManager* manager = &GetManager();
			float sw = manager->screenWidth;
			float sh = manager->screenHeight;
			float w = std::min(sw - 16.0F, 500.0F);
			float h = 160.0F;
			float x = (sw - w) * 0.5F;
			float y = (sh - h) * 0.5F;

			// draw full background
			{
				Handle<Label> overlay = Handle<Label>::New(manager);
				overlay->backgroundColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.7F);
				overlay->SetBounds(AABB2(0.0F, 0.0F, sw, sh));
				AddChild(overlay.GetPointerOrNull());
			}

			// Background
			{
				Handle<Label> bg = Handle<Label>::New(manager);
				bg->backgroundColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.9F);
				bg->SetBounds(AABB2(0.0F, y - 13.0F, size.x, h + 27.0F));
				AddChild(bg.GetPointerOrNull());
			}
			{
				Handle<Label> label = Handle<Label>::New(manager);
				label->text = _Tr("MainScreen", "Rename Demo");
				label->SetBounds(AABB2(x, y, w, 30.0F));
				label->alignment = MakeVector2(0.0F, 0.5F);
				AddChild(label.GetPointerOrNull());
			}
			{
				Handle<Field> field = Handle<Field>::New(manager);
				nameField = field.GetPointerOrNull();
				nameField->SetBounds(AABB2(x, y + 40.0F, w, 30.0F));
				nameField->SetText(currentName);
				nameField->SelectAll();
				AddChild(nameField);
			}
			{
				Handle<Button> btn = Handle<Button>::New(manager);
				btn->caption = _Tr("MainScreen", "Rename");
				btn->SetBounds(AABB2(x + w - 310.0F, y + 130.0F, 150.0F, 30.0F));
				btn->activated = [this](UIElement& s) { OnConfirm(s); };
				AddChild(btn.GetPointerOrNull());
			}
			{
				Handle<Button> btn = Handle<Button>::New(manager);
				btn->caption = _Tr("MainScreen", "Cancel");
				btn->SetBounds(AABB2(x + w - 150.0F, y + 130.0F, 150.0F, 30.0F));
				btn->activated = [this](UIElement& s) { OnCancel(s); };
				AddChild(btn.GetPointerOrNull());
			}
		}

		void RenameScreen::OnConfirm(UIElement&) {
			newName = nameField->GetText();
			result = true;
			Close();
		}

		void RenameScreen::OnCancel(UIElement&) {
			result = false;
			Close();
		}

		void RenameScreen::Close() {
			// keep ourselves alive while handlers observing the close run
			Handle<RenameScreen> keepAlive(this);
			owner->enable = true;
			GetParent()->RemoveChild(this);
			if (closed)
				closed(*this);
		}

		void RenameScreen::Run() {
			owner->enable = false;
			owner->GetParent()->AddChild(this);
			GetManager().SetActiveElement(nameField);
		}

		void RenameScreen::HotKey(const std::string& key) {
			if (IsEnabled() && key == "Enter") {
				OnConfirm(*this);
			} else if (IsEnabled() && key == "Escape") {
				OnCancel(*this);
			} else {
				UIElement::HotKey(key);
			}
		}

		struct WorldItemData {
			std::string filename;
			std::string displayName;
			std::string description;
			std::string tag;
			std::string sizeText;
		};

		class WorldListItem : public ui::ButtonBase {
			WorldItemData data;

		public:
			bool selected = false;

			WorldListItem(ui::UIManager* manager, WorldItemData data)
			    : ButtonBase(manager), data(std::move(data)) {}

			void Render() override {
				client::IRenderer& r = GetManager().GetRenderer();
				Vector2 pos = GetScreenPosition();
				Vector2 sz = size;

				Vector4 bgCol = MakeVector4(0.03F, 0.03F, 0.03F, 0.65F);
				Vector4 borderCol = MakeVector4(0.0F, 0.0F, 0.0F, 0.85F);

				if (selected) {
					bgCol = MakeVector4(0.12F, 0.16F, 0.22F, 0.90F);
					borderCol = MakeVector4(0.85F, 0.70F, 0.20F, 0.85F);
				} else if (hover) {
					bgCol = MakeVector4(0.10F, 0.10F, 0.10F, 0.80F);
					borderCol = MakeVector4(0.50F, 0.50F, 0.50F, 0.70F);
				}

				SetColorNP(r, bgCol);
				r.DrawFilledRect(pos.x + 1, pos.y + 1, pos.x + sz.x - 1, pos.y + sz.y - 1);

				SetColorNP(r, borderCol);
				r.DrawOutlinedRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				client::IFont* font = GetFont();
				if (!font)
					return;

				Vector4 titleCol = selected ? MakeVector4(1.0F, 0.95F, 0.60F, 1.0F)
				                            : MakeVector4(0.95F, 0.95F, 0.95F, 1.0F);
				font->DrawShadow(data.displayName, pos + MakeVector2(12.0F, 5.0F), 1.0F, titleCol,
				                 MakeVector4(0.0F, 0.0F, 0.0F, 0.6F));

				font->DrawShadow(data.description, pos + MakeVector2(14.0F, 24.0F), 0.85F,
				                 MakeVector4(0.70F, 0.75F, 0.80F, 0.90F),
				                 MakeVector4(0.0F, 0.0F, 0.0F, 0.5F));

				Vector4 tagCol = (data.filename == "LAbeach.vxl")
				                     ? MakeVector4(1.0F, 0.80F, 0.20F, 1.0F)
				                     : MakeVector4(0.40F, 0.85F, 1.0F, 0.90F);
				Vector2 tagSz = font->Measure(data.tag);
				font->DrawShadow(data.tag, pos + MakeVector2(sz.x - tagSz.x - 16.0F, 8.0F), 0.90F,
				                 tagCol, MakeVector4(0.0F, 0.0F, 0.0F, 0.6F));

				if (!data.sizeText.empty()) {
					Vector2 sizeSz = font->Measure(data.sizeText);
					font->DrawShadow(data.sizeText,
					                 pos + MakeVector2(sz.x - sizeSz.x * 0.85F - 16.0F, 24.0F),
					                 0.85F, MakeVector4(0.60F, 0.60F, 0.60F, 0.85F),
					                 MakeVector4(0.0F, 0.0F, 0.0F, 0.5F));
				}
			}
		};

		class WorldListModel : public ui::ListViewModel {
			ui::UIManager* manager;
			std::vector<WorldItemData> list;
			std::string selectedFilename;

		public:
			std::function<void(const std::string&)> itemActivated;
			std::function<void(const std::string&)> itemDoubleClicked;

			WorldListModel(ui::UIManager* manager, std::vector<WorldItemData> list,
			               const std::string& selected)
			    : manager(manager), list(std::move(list)), selectedFilename(selected) {}

			int GetNumRows() override { return static_cast<int>(list.size()); }

			Handle<ui::UIElement> CreateElement(int row) override {
				if (row < 0 || row >= static_cast<int>(list.size()))
					return Handle<ui::UIElement>();
				Handle<WorldListItem> item = Handle<WorldListItem>::New(manager, list[row]);
				item->selected = (list[row].filename == selectedFilename);
				item->activated = [this, row](ui::UIElement&) {
					if (itemActivated)
						itemActivated(list[row].filename);
				};
				item->doubleClicked = [this, row](ui::UIElement&) {
					if (itemDoubleClicked)
						itemDoubleClicked(list[row].filename);
				};
				return std::move(item).Cast<ui::UIElement>();
			}
		};

		// -- MainScreenMainMenu --

		MainScreenMainMenu::MainScreenMainMenu(MainScreenUI* ui)
		    : UIElement(&ui->GetUIManager()), ui(ui), helper(&ui->GetHelper()) {
			SetFont(&ui->GetFontManager().GetGuiFont());

			UIManager* manager = &GetManager();

			demoDateColWidth = 130.0F;
			demoModeColWidth = 125.0F;
			demoMapColWidth = 185.0F;
			demoSizeColWidth = 70.0F;

			modsCheckColWidth = 60.0F;
			modsOrderColWidth = 90.0F;
			modsTagColWidth = 56.0F;
			modsNameColWidth = 200.0F;
			modsAuthorColWidth = 150.0F;
			modsSizeColWidth = 100.0F;
			modsHelper = Handle<ModsScreenHelper>::New();

			float sw = manager->screenWidth;
			float sh = manager->screenHeight;

			float contentsWidth = sw - 16.0F;
			float maxContentsWidth = 750.0F;
			if (contentsWidth > maxContentsWidth)
				contentsWidth = maxContentsWidth;
			demoContentsWidth = contentsWidth;

			float contentsLeft = (sw - contentsWidth) * 0.5F;
			float footerPos = sh - 50.0F;
			float tabStripPos = 232.0F;
			float headerPos = 260.0F;
			float headerHeight = 24.0F;
			float listPos = headerPos + headerHeight;

			// adjust based on screen width
			float scaleF = std::min(sw / maxContentsWidth, 1.0F);
			float slotsOffsetX = 300.0F * scaleF;
			float mapNameOffsetX = 370.0F * scaleF;
			float gameModeOffsetX = 520.0F * scaleF;
			float protocolOffsetX = 640.0F * scaleF;
			float pingOffsetX = 680.0F * scaleF;

			// --- Tab strip (always visible) ---
			{
				Handle<TabPanel> serverPanelH = Handle<TabPanel>::New(manager);
				serverPanel = serverPanelH.GetPointerOrNull();
				serverPanel->SetBounds(AABB2(0.0F, 0.0F, sw, sh));

				Handle<TabPanel> worldsPanelH = Handle<TabPanel>::New(manager);
				worldsPanel = worldsPanelH.GetPointerOrNull();
				worldsPanel->SetBounds(AABB2(0.0F, 0.0F, sw, sh));
				worldsPanel->visible = false;

				Handle<TabPanel> demoPanelH = Handle<TabPanel>::New(manager);
				demoPanel = demoPanelH.GetPointerOrNull();
				demoPanel->SetBounds(AABB2(0.0F, 0.0F, sw, sh));
				demoPanel->visible = false;

				Handle<TabPanel> modsPanelH = Handle<TabPanel>::New(manager);
				modsPanel = modsPanelH.GetPointerOrNull();
				modsPanel->SetBounds(AABB2(0.0F, 0.0F, sw, sh));
				modsPanel->visible = false;

				// --- Server panel contents ---
				// Toggle IP Button (an icon that deploys the IP view)
				{
					Handle<MenuIconButton> button =
					    Handle<MenuIconButton>::New(manager, GuiIconType::AddServer, true);
					toggleIpButton = button.GetPointerOrNull();
					toggleIpButton->toggle = true;
					toggleIpButton->SetBounds(AABB2(contentsLeft, 200.0F, 34.0F, 30.0F));
					toggleIpButton->activated = [this](UIElement&) {
						showDirectConnect = !showDirectConnect;
						if (toggleIpButton) {
							toggleIpButton->iconType =
							    showDirectConnect ? GuiIconType::Minus : GuiIconType::AddServer;
						}
						UpdateDirectConnectVisibility();
						if (showDirectConnect && addressField) {
							GetManager().SetActiveElement(addressField);
						}
					};
					serverPanel->AddChild(toggleIpButton);
				}

				// Gold season / Most played server event notice
				{
					Handle<Label> label = Handle<Label>::New(manager);
					goldNoticeLabel = label.GetPointerOrNull();
					goldNoticeLabel->SetBounds(
					    AABB2(contentsLeft + 40.0F, 200.0F, contentsWidth - 235.0F, 30.0F));
					goldNoticeLabel->alignment = MakeVector2(0.0F, 0.5F);
					goldNoticeLabel->backgroundColor = MakeVector4(0.18F, 0.14F, 0.04F, 0.65F);
					goldNoticeLabel->outlineColor = MakeVector4(0.85F, 0.70F, 0.20F, 0.60F);
					goldNoticeLabel->textColor = MakeVector4(1.0F, 0.88F, 0.35F, 1.0F);
					goldNoticeLabel->shadowColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.5F);
					goldNoticeLabel->text = _Tr(
					    "MainScreen",
					    " [★] El server más jugado tras 1 año recibirá fondo dorado por 3 meses en enero");
					goldNoticeLabel->visible = false; // Only shown when Versión Y is selected!
					serverPanel->AddChild(goldNoticeLabel);
				}

				// Direct IP address field (hidden until deployed!)
				{
					Handle<Field> field = Handle<Field>::New(manager);
					addressField = field.GetPointerOrNull();
					addressField->SetBounds(
					    AABB2(contentsLeft + 40.0F, 200.0F, contentsWidth - 335.0F, 30.0F));
					addressField->placeholder = _Tr("MainScreen", "IP del Servidor (ej. 127.0.0.1:32887)");
					addressField->SetText(cg_lastQuickConnectHost);
					addressField->changed = [this](UIElement& s) { OnAddressChanged(s); };
					addressField->visible = false;
					serverPanel->AddChild(addressField);
				}

				// Connect button (hidden until deployed!)
				{
					Handle<MenuIconButton> button =
					    Handle<MenuIconButton>::New(manager, GuiIconType::Connect, false);
					connectButton = button.GetPointerOrNull();
					connectButton->caption = _Tr("MainScreen", "Conectar");
					connectButton->hotKeyText = _Tr("Client", "[Enter]");
					connectButton->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 290.0F, 200.0F, 140.0F, 30.0F));
					connectButton->activated = [this](UIElement&) { Connect(); };
					connectButton->visible = false;
					serverPanel->AddChild(connectButton);
				}

				// Search filter button (toggles filter bar on touch)
				{
					Handle<MenuIconButton> button =
					    Handle<MenuIconButton>::New(manager, GuiIconType::Search, true);
					searchFilterButton = button.GetPointerOrNull();
					searchFilterButton->toggle = true;
					searchFilterButton->toggled = false;
					searchFilterButton->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 190.0F, 200.0F, 34.0F, 30.0F));
					searchFilterButton->activated = [this](UIElement&) {
						showSearchFilter = !showSearchFilter;
						UpdateSearchFilterVisibility();
						if (showSearchFilter && filterField) {
							GetManager().SetActiveElement(filterField);
						}
					};
					serverPanel->AddChild(searchFilterButton);
				}

				// Refresh button
				{
					Handle<RefreshButton> button = Handle<RefreshButton>::New(manager);
					button->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 150.0F, 200.0F, 30.0F, 30.0F));
					button->activated = [this](UIElement&) { LoadServerList(); };
					serverPanel->AddChild(button.GetPointerOrNull());
				}

				// Version selector dropdown button (Todos -> Versión Y / 0.76 / 0.75)
				{
					Handle<Button> button = Handle<Button>::New(manager);
					versionFilterButton = button.GetPointerOrNull();
					versionFilterButton->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 115.0F, 200.0F, 115.0F, 30.0F));
					versionFilterButton->caption = _Tr("MainScreen", "Todos");
					versionFilterButton->activated = [this](UIElement&) { OpenVersionDropdown(); };
					serverPanel->AddChild(versionFilterButton);
				}
				{
					Handle<ListView> list = Handle<ListView>::New(manager);
					serverList = list.GetPointerOrNull();
					serverList->SetBounds(
					    AABB2(contentsLeft, listPos, contentsWidth, footerPos - listPos - 8.0F));
					serverPanel->AddChild(serverList);
				}
				{
					Handle<ServerListHeader> header = Handle<ServerListHeader>::New(manager);
					header->SetBounds(AABB2(contentsLeft, headerPos, slotsOffsetX, headerHeight));
					header->text = _Tr("MainScreen", "Server Name");
					header->activated = [this](UIElement&) { SortServerList(2); };
					serverPanel->AddChild(header.GetPointerOrNull());
				}
				{
					Handle<ServerListHeader> header = Handle<ServerListHeader>::New(manager, GuiIconType::Players);
					header->SetBounds(AABB2(contentsLeft + slotsOffsetX, headerPos,
					                        mapNameOffsetX - slotsOffsetX, headerHeight));
					header->text = _Tr("MainScreen", "Slots");
					header->alignment = MakeVector2(0.5F, 0.5F);
					header->activated = [this](UIElement&) { SortServerList(1); };
					serverPanel->AddChild(header.GetPointerOrNull());
				}
				{
					Handle<ServerListHeader> header = Handle<ServerListHeader>::New(manager);
					header->SetBounds(AABB2(contentsLeft + mapNameOffsetX, headerPos,
					                        gameModeOffsetX - mapNameOffsetX, headerHeight));
					header->text = _Tr("MainScreen", "Map Name");
					header->activated = [this](UIElement&) { SortServerList(3); };
					serverPanel->AddChild(header.GetPointerOrNull());
				}
				{
					Handle<ServerListHeader> header = Handle<ServerListHeader>::New(manager);
					header->SetBounds(AABB2(contentsLeft + gameModeOffsetX, headerPos,
					                        protocolOffsetX - gameModeOffsetX, headerHeight));
					header->text = _Tr("MainScreen", "Game Mode");
					header->alignment = MakeVector2(0.5F, 0.5F);
					header->activated = [this](UIElement&) { SortServerList(4); };
					serverPanel->AddChild(header.GetPointerOrNull());
				}
				{
					Handle<ServerListHeader> header = Handle<ServerListHeader>::New(manager);
					header->SetBounds(AABB2(contentsLeft + protocolOffsetX, headerPos,
					                        pingOffsetX - protocolOffsetX, headerHeight));
					header->text = _Tr("MainScreen", "Ver.");
					header->alignment = MakeVector2(0.5F, 0.5F);
					header->activated = [this](UIElement&) { SortServerList(5); };
					serverPanel->AddChild(header.GetPointerOrNull());
				}
				{
					Handle<ServerListHeader> header =
					    Handle<ServerListHeader>::New(manager, GuiIconType::Ping);
					header->SetBounds(AABB2(contentsLeft + pingOffsetX, headerPos,
					                        contentsWidth - pingOffsetX, headerHeight));
					header->text = _Tr("MainScreen", "Ping");
					header->alignment = MakeVector2(0.5F, 0.5F);
					header->activated = [this](UIElement&) { SortServerList(0); };
					serverPanel->AddChild(header.GetPointerOrNull());
				}
				{
					Handle<MainScreenServerListLoadingView> view =
					    Handle<MainScreenServerListLoadingView>::New(manager);
					loadingView = view.GetPointerOrNull();
					loadingView->SetBounds(AABB2(contentsLeft, listPos, contentsWidth, 100.0F));
					loadingView->visible = false;
					serverPanel->AddChild(loadingView);
				}
				{
					Handle<MainScreenServerListErrorView> view =
					    Handle<MainScreenServerListErrorView>::New(manager);
					errorView = view.GetPointerOrNull();
					errorView->SetBounds(AABB2(contentsLeft, listPos, contentsWidth, 100.0F));
					errorView->visible = false;
					serverPanel->AddChild(errorView);
				}

				// Server search & filter bar (hidden by default, toggled with search button)
				float filterY = footerPos - 36.0F;
				{
					Handle<Label> label = Handle<Label>::New(manager);
					filterLabel = label.GetPointerOrNull();
					filterLabel->text = _Tr("MainScreen", "Buscar:");
					filterLabel->SetBounds(AABB2(contentsLeft, filterY, 60.0F, 26.0F));
					filterLabel->alignment = MakeVector2(0.0F, 0.5F);
					filterLabel->visible = false;
					serverPanel->AddChild(filterLabel);
				}

				{
					Handle<Field> field = Handle<Field>::New(manager);
					filterField = field.GetPointerOrNull();
					filterField->SetBounds(AABB2(contentsLeft + 65.0F, filterY, 210.0F, 26.0F));
					filterField->placeholder = _Tr("MainScreen", "Nombre, mapa o modo...");
					filterField->changed = [this](UIElement&) { UpdateServerList(); };
					filterField->visible = false;
					serverPanel->AddChild(filterField);
				}

				{
					Handle<ProtocolButton> button = Handle<ProtocolButton>::New(manager);
					filterFullButton = button.GetPointerOrNull();
					filterFullButton->SetBounds(
					    AABB2(contentsLeft + 285.0F, filterY, 95.0F, 26.0F));
					filterFullButton->caption = _Tr("MainScreen", "Ocultar Llenos");
					filterFullButton->activated = [this](UIElement&) { UpdateServerList(); };
					filterFullButton->toggle = true;
					filterFullButton->visible = false;
					serverPanel->AddChild(filterFullButton);
				}

				// --- Worlds panel contents (Mundos) ---
				{
					Handle<Label> titleLabel = Handle<Label>::New(manager);
					titleLabel->text = _Tr("MainScreen", "MUNDOS GUARDADOS (.VXL)");
					titleLabel->textColor = MakeVector4(0.95F, 0.95F, 1.0F, 1.0F);
					titleLabel->alignment = MakeVector2(0.0F, 0.5F);
					titleLabel->SetBounds(AABB2(contentsLeft, 200.0F, 240.0F, 30.0F));
					worldsPanel->AddChild(titleLabel.GetPointerOrNull());

					Handle<Button> openFolderBtn = Handle<Button>::New(manager);
					openFolderBtn->caption = _Tr("MainScreen", "Abrir Carpeta");
					openFolderBtn->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 310.0F, 200.0F, 140.0F, 30.0F));
					openFolderBtn->activated = [](UIElement&) {
#ifdef _WIN32
						ShowDirectoryInShell("Resources\\Maps");
#else
						ShowDirectoryInShell("Resources/Maps");
#endif
					};
					worldsPanel->AddChild(openFolderBtn.GetPointerOrNull());

					Handle<Button> playWorldBtn = Handle<Button>::New(manager);
					playWorldBtn->caption = _Tr("MainScreen", "Entrar al Mundo");
					playWorldBtn->hotKeyText = _Tr("Client", "[Enter]");
					playWorldBtn->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 160.0F, 200.0F, 160.0F, 30.0F));
					playWorldBtn->activated = [this](UIElement&) { PlaySelectedWorld(); };
					worldsPanel->AddChild(playWorldBtn.GetPointerOrNull());

					Handle<DemoListHeader> header1 = Handle<DemoListHeader>::New(manager);
					header1->SetBounds(AABB2(contentsLeft, headerPos, contentsWidth - 140.0F, headerHeight));
					header1->text = _Tr("MainScreen", "Mundo / Archivo");
					worldsPanel->AddChild(header1.GetPointerOrNull());

					Handle<DemoListHeader> header2 = Handle<DemoListHeader>::New(manager);
					header2->SetBounds(AABB2(contentsLeft + contentsWidth - 140.0F, headerPos, 140.0F, headerHeight));
					header2->text = _Tr("MainScreen", "Estado");
					worldsPanel->AddChild(header2.GetPointerOrNull());

					Handle<ListView> list = Handle<ListView>::New(manager);
					worldsList = list.GetPointerOrNull();
					worldsList->rowHeight = 44.0F;
					worldsList->SetBounds(
					    AABB2(contentsLeft, listPos, contentsWidth, footerPos - listPos - 8.0F));
					worldsPanel->AddChild(worldsList);
				}

				// --- Demo panel contents ---
				{
					// Selected demo name display (mirrors the server address field)
					Handle<Field> field = Handle<Field>::New(manager);
					demoNameField = field.GetPointerOrNull();
					demoNameField->SetBounds(
					    AABB2(contentsLeft, 200.0F, contentsWidth - 320.0F, 30.0F));
					demoNameField->placeholder = _Tr("MainScreen", "Select a demo");
					demoNameField->acceptsFocus = false;
					demoNameField->isMouseInteractive = false;
					demoPanel->AddChild(demoNameField);
				}
				{
					Handle<Button> button = Handle<Button>::New(manager);
					demoDeleteButton = button.GetPointerOrNull();
					demoDeleteButton->caption = _Tr("MainScreen", "Delete");
					demoDeleteButton->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 320.0F, 200.0F, 80.0F, 30.0F));
					demoDeleteButton->activated = [this](UIElement& s) { OnDeleteDemoPressed(s); };
					demoPanel->AddChild(demoDeleteButton);
				}
				{
					Handle<Button> button = Handle<Button>::New(manager);
					demoRenameButton = button.GetPointerOrNull();
					demoRenameButton->caption = _Tr("MainScreen", "Rename");
					demoRenameButton->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 240.0F, 200.0F, 90.0F, 30.0F));
					demoRenameButton->activated = [this](UIElement& s) { OnRenameDemoPressed(s); };
					demoPanel->AddChild(demoRenameButton);
				}
				{
					Handle<Button> button = Handle<Button>::New(manager);
					demoPlayButton = button.GetPointerOrNull();
					demoPlayButton->caption = _Tr("MainScreen", "Play");
					demoPlayButton->hotKeyText = _Tr("Client", "[Enter]");
					demoPlayButton->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 150.0F, 200.0F, 150.0F, 30.0F));
					demoPlayButton->activated = [this](UIElement& s) { OnPlayDemoPressed(s); };
					demoPanel->AddChild(demoPlayButton);
				}
				{
					// Column order: Server | Map | Mode | Timestamp | Size
					// Server is variable-width; the four rightmost columns are fixed.
					float fixedRight =
					    demoMapColWidth + demoModeColWidth + demoDateColWidth + demoSizeColWidth;
					float serverColWidth = contentsWidth - fixedRight;
					float x = contentsLeft;
					{
						Handle<DemoListHeader> header = Handle<DemoListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, serverColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Server");
						demoPanel->AddChild(header.GetPointerOrNull());
						x += serverColWidth;
					}
					{
						Handle<DemoListHeader> header = Handle<DemoListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, demoMapColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Map");
						demoPanel->AddChild(header.GetPointerOrNull());
						x += demoMapColWidth;
					}
					{
						Handle<DemoListHeader> header = Handle<DemoListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, demoModeColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Game Mode");
						demoPanel->AddChild(header.GetPointerOrNull());
						x += demoModeColWidth;
					}
					{
						Handle<DemoListHeader> header = Handle<DemoListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, demoDateColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Timestamp");
						demoPanel->AddChild(header.GetPointerOrNull());
						x += demoDateColWidth;
					}
					{
						Handle<DemoListHeader> header = Handle<DemoListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, demoSizeColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Size");
						demoPanel->AddChild(header.GetPointerOrNull());
					}
				}
				{
					Handle<ListView> list = Handle<ListView>::New(manager);
					demoList = list.GetPointerOrNull();
					demoList->SetBounds(
					    AABB2(contentsLeft, listPos, contentsWidth, footerPos - listPos - 14.0F));
					demoPanel->AddChild(demoList);
				}

				// --- Mods panel contents ---
				{
					Handle<Button> button = Handle<Button>::New(manager);
					modsDownloadButton = button.GetPointerOrNull();
					modsDownloadButton->caption = _Tr("MainScreen", "Download official mods...");
					modsDownloadButton->SetBounds(AABB2(contentsLeft, 200.0F, 240.0F, 30.0F));
					modsDownloadButton->activated = [this](UIElement& s) {
						OnDownloadModsPressed(s);
					};
					modsPanel->AddChild(modsDownloadButton);
				}
				{
					Handle<Button> button = Handle<Button>::New(manager);
					modsResetButton = button.GetPointerOrNull();
					modsResetButton->caption = _Tr("MainScreen", "Disable all");
					modsResetButton->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 300.0F, 200.0F, 130.0F, 30.0F));
					modsResetButton->activated = [this](UIElement& s) { OnResetModsPressed(s); };
					modsPanel->AddChild(modsResetButton);
				}
				{
					Handle<Button> button = Handle<Button>::New(manager);
					modsApplyButton = button.GetPointerOrNull();
					modsApplyButton->caption = _Tr("MainScreen", "Apply changes");
					modsApplyButton->SetBounds(
					    AABB2(contentsLeft + contentsWidth - 160.0F, 200.0F, 160.0F, 30.0F));
					modsApplyButton->activated = [this](UIElement& s) { OnApplyModsPressed(s); };
					modsPanel->AddChild(modsApplyButton);
				}
				{
					Handle<Label> label = Handle<Label>::New(manager);
					modsStatusLabel = label.GetPointerOrNull();
					modsStatusLabel->SetBounds(
					    AABB2(contentsLeft, footerPos - 40.0F, contentsWidth, 24.0F));
					modsStatusLabel->text = "";
					modsPanel->AddChild(modsStatusLabel);
				}
				{
					Handle<ModsProgressBar> bar = Handle<ModsProgressBar>::New(manager);
					modsProgressBar = bar.GetPointerOrNull();
					modsProgressBar->SetBounds(
					    AABB2(contentsLeft, footerPos - 12.0F, contentsWidth, 6.0F));
					modsProgressBar->visible = false;
					modsPanel->AddChild(modsProgressBar);
				}
				{
					float x = contentsLeft;
					{
						Handle<ModListHeader> header = Handle<ModListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, modsCheckColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Status");
						modsPanel->AddChild(header.GetPointerOrNull());
						x += modsCheckColWidth;
					}
					{
						Handle<ModListHeader> header = Handle<ModListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, modsOrderColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Apply Order");
						modsPanel->AddChild(header.GetPointerOrNull());
						x += modsOrderColWidth;
					}
					{
						Handle<ModListHeader> header = Handle<ModListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, modsTagColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Type");
						modsPanel->AddChild(header.GetPointerOrNull());
						x += modsTagColWidth;
					}
					{
						Handle<ModListHeader> header = Handle<ModListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, modsNameColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Mod");
						modsPanel->AddChild(header.GetPointerOrNull());
						x += modsNameColWidth;
					}
					{
						Handle<ModListHeader> header = Handle<ModListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, modsAuthorColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Author");
						modsPanel->AddChild(header.GetPointerOrNull());
						x += modsAuthorColWidth;
					}
					{
						Handle<ModListHeader> header = Handle<ModListHeader>::New(manager);
						header->SetBounds(AABB2(x, headerPos, modsSizeColWidth, headerHeight));
						header->text = _Tr("MainScreen", "Size");
						modsPanel->AddChild(header.GetPointerOrNull());
					}
				}
				{
					Handle<ListView> list = Handle<ListView>::New(manager);
					modsList = list.GetPointerOrNull();
					modsList->SetBounds(
					    AABB2(contentsLeft, listPos, contentsWidth, footerPos - listPos - 44.0F));
					modsPanel->AddChild(modsList);
				}

				AddChild(serverPanel);
				AddChild(worldsPanel);
				AddChild(demoPanel);
				AddChild(modsPanel);

				// Tab strip
				{
					Handle<SimpleTabStrip> tabStrip = Handle<SimpleTabStrip>::New(manager);
					tabStrip->SetBounds(AABB2(contentsLeft, tabStripPos, contentsWidth, 24.0F));
					AddChild(tabStrip.GetPointerOrNull());
					tabStrip->AddItem(_Tr("MainScreen", "Servidores"), serverPanel);
					// Place Mods directly next to Servidores
					if (!helper->IsTryingMod())
						tabStrip->AddItem(_Tr("MainScreen", "Mods"), modsPanel);
					tabStrip->AddItem(_Tr("MainScreen", "Mundos"), worldsPanel);
					tabStrip->AddItem(_Tr("MainScreen", "Demos"), demoPanel);
					tabStrip->changed = [this](UIElement& s) { OnTabChanged(s); };
				}

				// Coming back from an apply-triggered relaunch — open the Mods tab.
				if (helper->ShouldOpenModsTab()) {
					serverPanel->visible = false;
					if (worldsPanel)
						worldsPanel->visible = false;
					demoPanel->visible = false;
					modsPanel->visible = true;
				}
			}

			// Footer buttons (always visible, outside panels)
			{
				Handle<MenuIconButton> button =
				    Handle<MenuIconButton>::New(manager, GuiIconType::Lobby);
				button->caption = _Tr("MainScreen", "Lobby");
				button->hotKeyText = _Tr("Client", "[Esc]");
				button->SetBounds(AABB2(contentsLeft, footerPos, 110.0F, 30.0F));
				button->activated = [this](UIElement&) { ReturnToLobby(); };
				AddChild(button.GetPointerOrNull());
			}
			{
				Handle<MenuIconButton> button =
				    Handle<MenuIconButton>::New(manager, GuiIconType::Skin);
				skinsButton = button.GetPointerOrNull();
				skinsButton->caption = _Tr("MainScreen", "Skins");
				skinsButton->SetBounds(AABB2(contentsLeft + 115.0F, footerPos, 110.0F, 30.0F));
				skinsButton->activated = [this](UIElement& s) { OnSkinsPressed(s); };
				AddChild(skinsButton);
			}
			{
				Handle<MenuIconButton> button =
				    Handle<MenuIconButton>::New(manager, GuiIconType::Extension);
				modsButton = button.GetPointerOrNull();
				modsButton->caption = _Tr("MainScreen", "Mods");
				modsButton->SetBounds(AABB2(contentsLeft + 230.0F, footerPos, 110.0F, 30.0F));
				modsButton->activated = [this](UIElement&) {
					MainScreenMainMenuState st;
					st.activeTabIndex = 1;
					SetState(st);
				};
				AddChild(modsButton);
			}

			LoadServerList();
			LoadDemoList();
			LoadWorldList();

			if (helper->ShouldOpenModsTab())
				LoadModList();
		}

		void MainScreenMainMenu::LoadServerList() {
			if (loading)
				return;
			loaded = false;
			loading = true;
			serverList->SetModel(Handle<ListViewModel>::New().GetPointerOrNull()); // empty
			errorView->visible = false;
			loadingView->visible = true;
			helper->StartQuery();
		}

		void MainScreenMainMenu::LoadDemoList() {
			std::vector<std::string> demos = helper->GetDemoList();
			// Reverse so newest demos appear first
			std::vector<std::string> reversed(demos.rbegin(), demos.rend());
			Handle<DemoListModel> model = Handle<DemoListModel>::New(
			    &GetManager(), helper, std::move(reversed), demoDateColWidth, demoModeColWidth,
			    demoMapColWidth, demoSizeColWidth, demoContentsWidth);
			demoList->SetModel(model.GetPointerOrNull());
			model->itemActivated = [this](const std::string& f) { DemoListItemActivated(f); };
			model->itemDoubleClicked = [this](const std::string& f) {
				DemoListItemDoubleClicked(f);
			};
			currentDemoListModel = model;
		}

		void MainScreenMainMenu::OnTabChanged(UIElement& s) {
			if (worldsPanel && worldsPanel->visible)
				LoadWorldList();
			if (demoPanel && demoPanel->visible)
				LoadDemoList();
			if (modsPanel && modsPanel->visible) {
				LoadModList();
			}
		}

		MainScreenMainMenuState MainScreenMainMenu::GetState() {
			MainScreenMainMenuState state;
			if (modsPanel && modsPanel->visible)
				state.activeTabIndex = 1;
			else if (worldsPanel && worldsPanel->visible)
				state.activeTabIndex = 2;
			else if (demoPanel && demoPanel->visible)
				state.activeTabIndex = 3;
			else
				state.activeTabIndex = 0;
			return state;
		}

		void MainScreenMainMenu::SetState(const MainScreenMainMenuState& state) {
			serverPanel->visible = (state.activeTabIndex == 0);
			if (modsPanel)
				modsPanel->visible = (state.activeTabIndex == 1);
			if (worldsPanel)
				worldsPanel->visible = (state.activeTabIndex == 2);
			if (demoPanel)
				demoPanel->visible = (state.activeTabIndex == 3);

			// A restored tab is populated by the same path a clicked one is: the
			// constructor only fills the server list.
			OnTabChanged(*this);
		}

		void MainScreenMainMenu::LoadModList() {
			std::vector<std::string> names = modsHelper->GetModNames();
			std::vector<std::string> enabled = modsHelper->GetEnabledMods();

			// Sort mods by Category / Type: weapons first (SEMI, SMG, SHOTGUN, SPADE, GRENADE), then SFX, VFX, FONT, etc.
			auto getCategoryRank = [this](const std::string& name) -> int {
				std::string cat = modsHelper->GetModCategory(name);
				if (cat == "SEMI" || cat == "RIFLE") return 0;
				if (cat == "SMG") return 1;
				if (cat == "SHOTGUN") return 2;
				if (cat == "SPADE") return 3;
				if (cat == "GRENADE") return 4;
				if (cat == "SFX") return 5;
				if (cat == "VFX") return 6;
				if (cat == "FONT") return 7;
				return 8;
			};

			std::sort(names.begin(), names.end(), [&](const std::string& a, const std::string& b) {
				int rankA = getCategoryRank(a);
				int rankB = getCategoryRank(b);
				if (rankA != rankB) return rankA < rankB;
				std::string catA = modsHelper->GetModCategory(a);
				std::string catB = modsHelper->GetModCategory(b);
				if (catA != catB) return catA < catB;
				return modsHelper->GetModDisplayName(a) < modsHelper->GetModDisplayName(b);
			});

			// Displayed rows keep a fixed order: every mod on disk in its natural
			// order, then any enabled mod that no longer exists (orphaned entries,
			// shown so they can be removed). Toggling only changes a row's apply
			// number and colour, never its position.
			std::vector<std::string> list;
			std::vector<int> orders;
			std::vector<bool> exists;

			for (const std::string& name : names) {
				int idx = EnabledIndex(enabled, name);
				list.push_back(name);
				orders.push_back(idx + 1); // 0 when disabled (idx == -1)
				exists.push_back(true);
			}
			// Orphaned enabled mods (enabled but no longer on disk).
			for (size_t i = 0; i < enabled.size(); i++) {
				bool onDisk = false;
				for (const std::string& name : names) {
					if (name == enabled[i]) {
						onDisk = true;
						break;
					}
				}
				if (!onDisk) {
					list.push_back(enabled[i]);
					orders.push_back(static_cast<int>(i) + 1);
					exists.push_back(false);
				}
			}

			Handle<ModListModel> model = Handle<ModListModel>::New(
			    &GetManager(), modsHelper.GetPointerOrNull(), std::move(list), std::move(orders),
			    std::move(exists), modsCheckColWidth, modsOrderColWidth, modsTagColWidth,
			    modsNameColWidth, modsAuthorColWidth, modsSizeColWidth);
			modsList->SetModel(model.GetPointerOrNull());
			model->itemActivated = [this](const std::string& name) {
				selectedModCategory = modsHelper->GetModCategory(name);
				OnModToggle(name);
			};
			UpdateModsStatus();
		}

		int MainScreenMainMenu::EnabledIndex(const std::vector<std::string>& enabled,
		                                     const std::string& name) {
			for (size_t i = 0; i < enabled.size(); i++) {
				if (enabled[i] == name)
					return static_cast<int>(i);
			}
			return -1;
		}

		void MainScreenMainMenu::UpdateModsStatus() {
			if (modsApplyButton != nullptr) {
				// Toggles already persist; Apply only restarts to make pending
				// changes live, so it's actionable only when something changed.
				modsApplyButton->enable = modsDirty && !modsDownloading;
				modsApplyButton->caption = modsDirty ? _Tr("MainScreen", "Aplicar y Reiniciar")
				                                     : _Tr("MainScreen", "Apply changes");
			}
			if (modsDownloading) {
				int done = modsHelper->GetRefreshDone();
				int total = modsHelper->GetRefreshTotal();
				std::string item = modsHelper->GetRefreshCurrentItem();
				if (total <= 0) {
					modsStatusLabel->text = _Tr("MainScreen", "Fetching mod list...");
					modsProgressBar->fraction = 0.0F;
				} else {
					std::string label = std::to_string(done) + " / " + std::to_string(total);
					if (item.size() > 0)
						label += " — " + item;
					modsStatusLabel->text = label;
					modsProgressBar->fraction =
					    static_cast<float>(done) / static_cast<float>(total);
				}
				modsProgressBar->visible = true;
				return;
			}
			modsProgressBar->visible = false;
			std::string msg = modsHelper->GetRefreshMessage();
			if (msg.size() > 0) {
				modsStatusLabel->text = msg;
				return;
			}
			if (modsDirty) {
				modsStatusLabel->text =
				    _Tr("MainScreen", "Cambios pendientes: Haz clic en 'Aplicar y Reiniciar' para activar tus mods en el juego.");
				return;
			}
			modsStatusLabel->text = "";
		}

		void MainScreenMainMenu::OnDownloadModsPressed(UIElement&) {
			if (modsDownloading)
				return;
			std::string body =
			    _Tr("MainScreen", "Download the official ZeroSpades mod pack?") + "\n\n";
			body +=
			    _Tr("MainScreen", "These are mods reviewed and hosted by the ZeroSpades team.") +
			    "\n\n";
			body += _Tr("MainScreen", "Source: ") + modsHelper->GetIndexUrl() + "\n";
			body += _Tr("MainScreen", "Files will be saved into your Mods/ folder, overwriting "
			                          "any existing copies.");
			Handle<ConfirmScreen> cs = Handle<ConfirmScreen>::New(
			    this, body, std::min(500.0F, GetManager().screenHeight - 100.0F), true);
			cs->closed = [this](UIElement& s) { OnDownloadConfirmed(s); };
			cs->Run();
		}

		void MainScreenMainMenu::OnDownloadConfirmed(UIElement& sender) {
			ConfirmScreen* cs = dynamic_cast<ConfirmScreen*>(&sender);
			if (cs == nullptr || !cs->GetResult())
				return;
			if (modsDownloading)
				return;
			modsDownloading = true;
			modsHelper->StartRefresh();
			UpdateModsStatus();
		}

		// Apply = restart now so the enabled set is mounted. Toggles are already
		// saved, so this just relaunches straight back into the Mods tab.
		void MainScreenMainMenu::OnApplyModsPressed(UIElement&) {
			if (modsDownloading)
				return;
			helper->RelaunchForMods();
		}

		void MainScreenMainMenu::OnResetModsPressed(UIElement&) {
			Handle<ConfirmScreen> cs = Handle<ConfirmScreen>::New(
			    this, _Tr("MainScreen", "Disable all mods? This takes effect after a restart."),
			    200.0F, true);
			cs->closed = [this](UIElement& s) { OnResetConfirmed(s); };
			cs->Run();
		}

		void MainScreenMainMenu::OnResetConfirmed(UIElement& sender) {
			ConfirmScreen* cs = dynamic_cast<ConfirmScreen*>(&sender);
			if (cs == nullptr || !cs->GetResult())
				return;
			modsHelper->ClearEnabledMods();
			modsDirty = true;
			// Refresh the rows to clear every checkbox, not just the status line.
			LoadModList();
		}

		// Clicking a row toggles the mod in the enabled set. The change is saved
		// immediately and takes effect the next time the game starts.
		void MainScreenMainMenu::OnModToggle(const std::string& modName) {
			if (modsDownloading)
				return;
			std::vector<std::string> enabled = modsHelper->GetEnabledMods();
			bool enabling = (EnabledIndex(enabled, modName) < 0);
			if (enabling)
				modsHelper->EnableMod(modName);
			else
				modsHelper->DisableMod(modName);
			modsDirty = true;
			LoadModList();

			std::string promptMsg = enabling
			    ? _Tr("MainScreen", "Mod activado en la lista.\n¿Deseas reiniciar ySpades ahora para que se aplique en el juego?")
			    : _Tr("MainScreen", "Mod desactivado en la lista.\n¿Deseas reiniciar ySpades ahora para que se aplique en el juego?");

			Handle<ConfirmScreen> cs = Handle<ConfirmScreen>::New(this, promptMsg, 200.0F, true);
			cs->closed = [this](UIElement& sender) {
				ConfirmScreen* cs = dynamic_cast<ConfirmScreen*>(&sender);
				if (cs && cs->GetResult()) {
					helper->RelaunchForMods();
				}
			};
			cs->Run();
		}

		void MainScreenMainMenu::CheckModsRefresh() {
			if (!modsDownloading)
				return;
			if (modsHelper->PollRefreshState()) {
				modsDownloading = false;
				LoadModList();
				// A non-empty refresh message means the download failed. The
				// status label keeps it (sticky), but also raise a loud alert so
				// a failure can never be mistaken for "nothing happened".
				std::string err = modsHelper->GetRefreshMessage();
				if (!err.empty()) {
					Handle<AlertScreen> al = Handle<AlertScreen>::New(
					    this, _Tr("MainScreen", "Mod download failed:") + "\n\n" + err, 200.0F,
					    true);
					al->Run();
				}
				return;
			}
			UpdateModsStatus();
		}

		void MainScreenMainMenu::DemoListItemActivated(const std::string& filename) {
			selectedDemoPath = filename;
			demoNameField->SetText(StripDemoPath(filename));
		}

		void MainScreenMainMenu::DemoListItemDoubleClicked(const std::string& filename) {
			selectedDemoPath = filename;
			demoNameField->SetText(StripDemoPath(filename));
			PlaySelectedDemo();
		}

		void MainScreenMainMenu::PlaySelectedDemo() {
			if (selectedDemoPath.empty())
				return;
			std::string msg = helper->PlayDemo(selectedDemoPath);
			if (msg.size() > 0) {
				Handle<AlertScreen> al = Handle<AlertScreen>::New(this, msg, 200.0F, true);
				al->Run();
			}
		}

		void MainScreenMainMenu::OnPlayDemoPressed(UIElement&) { PlaySelectedDemo(); }

		void MainScreenMainMenu::OnDeleteDemoPressed(UIElement&) {
			if (selectedDemoPath.empty())
				return;
			std::string name = StripDemoPath(selectedDemoPath);
			Handle<ConfirmScreen> confirm = Handle<ConfirmScreen>::New(
			    this, _Tr("MainScreen",
			              "Are you sure you want to delete '{0}'?\nThis action cannot be undone.",
			              name), 200.0F, true);
			confirm->closed = [this](UIElement& s) { OnDeleteConfirmClosed(s); };
			confirm->Run();
		}

		void MainScreenMainMenu::OnDeleteConfirmClosed(UIElement& sender) {
			ConfirmScreen* confirm = dynamic_cast<ConfirmScreen*>(&sender);
			if (!confirm || !confirm->GetResult())
				return;
			if (!helper->DeleteDemo(selectedDemoPath))
				return;
			selectedDemoPath = "";
			demoNameField->SetText("");
			LoadDemoList();
		}

		void MainScreenMainMenu::OnRenameDemoPressed(UIElement&) {
			if (selectedDemoPath.empty())
				return;
			Handle<RenameScreen> rename =
			    Handle<RenameScreen>::New(this, StripDemoPath(selectedDemoPath));
			rename->closed = [this](UIElement& s) { OnRenameScreenClosed(s); };
			rename->Run();
		}

		void MainScreenMainMenu::OnRenameScreenClosed(UIElement& sender) {
			RenameScreen* rename = dynamic_cast<RenameScreen*>(&sender);
			if (!rename || !rename->result)
				return;
			std::string newName = rename->newName;
			if (newName.size() >= 4 && newName.substr(newName.size() - 4) == ".dem")
				newName = newName.substr(0, newName.size() - 4);
			if (newName.empty())
				return;
			std::string newPath = "Demos/" + newName + ".dem";
			if (!helper->RenameDemo(selectedDemoPath, newPath)) {
				Handle<AlertScreen> al =
				    Handle<AlertScreen>::New(this, _Tr("MainScreen", "Failed to rename demo."),
				                             200.0F, true);
				al->Run();
				return;
			}
			selectedDemoPath = newPath;
			demoNameField->SetText(StripDemoPath(newPath));
			LoadDemoList();
		}

		void MainScreenMainMenu::ServerListItemActivated(MainScreenServerItem& item) {
			addressField->SetText(item.GetAddress());
			cg_lastQuickConnectHost = addressField->GetText();
			selectedMapName = item.GetMapName();
			if (item.GetProtocol() == "0.75") {
				SetProtocolVersion(3);
			} else if (item.GetProtocol() == "0.76") {
				SetProtocolVersion(4);
			} else if (item.GetProtocol() == "Y" || item.GetProtocol() == "y" ||
			           item.GetProtocol() == "1.0" || item.GetProtocol() == "11") {
				SetProtocolVersion(11);
			}
			addressField->SelectAll();
		}

		void MainScreenMainMenu::ServerListItemDoubleClicked(MainScreenServerItem& item) {
			ServerListItemActivated(item);

			// Double-click to connect
			Connect();
		}

		void MainScreenMainMenu::ServerListItemRightClicked(MainScreenServerItem& item) {
			helper->SetServerFavorite(item.GetAddress(), !item.IsFavorite());
			UpdateServerList();
		}

		void MainScreenMainMenu::SortServerList(int keyId) {
			int sort = cg_serverlistSort;
			if ((sort & 0xFFF) == keyId) {
				sort ^= 0x4000;
			} else {
				sort = keyId;
			}
			cg_serverlistSort = sort;
			UpdateServerList();
		}

		void MainScreenMainMenu::UpdateServerList(bool refresh) {
			std::string key = "";
			switch (static_cast<int>(cg_serverlistSort) & 0xFFF) {
				case 0: key = "Ping"; break;
				case 1: key = "NumPlayers"; break;
				case 2: key = "Name"; break;
				case 3: key = "MapName"; break;
				case 4: key = "GameMode"; break;
				case 5: key = "Protocol"; break;
				case 6: key = "Country"; break;
			}
			std::vector<Handle<MainScreenServerItem>> list =
			    helper->GetServerList(key, (static_cast<int>(cg_serverlistSort) & 0x4000) != 0);
			if (list.empty() || loading) {
				serverList->SetModel(Handle<ListViewModel>::New().GetPointerOrNull()); // empty
				return;
			}

			// filter the server list
			bool filterFull = filterFullButton ? filterFullButton->toggled : false;
			std::string filterText = filterField ? filterField->GetText() : "";
			std::vector<Handle<MainScreenServerItem>> list2;
			for (const Handle<MainScreenServerItem>& item : list) {
				if (currentVersionFilter == ServerVersionFilter::v075 && item->GetProtocol() != "0.75")
					continue;
				if (currentVersionFilter == ServerVersionFilter::v076 && item->GetProtocol() != "0.76")
					continue;
				if (currentVersionFilter == ServerVersionFilter::vY &&
				    item->GetProtocol() != "Y" && item->GetProtocol() != "y" &&
				    item->GetProtocol() != "1.0" && item->GetProtocol() != "11")
					continue;

				if (filterFull && (item->GetNumPlayers() >= item->GetMaxPlayers()))
					continue;
				if (filterText.size() > 0) {
					if (!(StringContainsCaseInsensitive(item->GetName(), filterText) ||
					      StringContainsCaseInsensitive(item->GetMapName(), filterText) ||
					      StringContainsCaseInsensitive(item->GetGameMode(), filterText))) {
						continue;
					}
				}
				list2.push_back(item);
			}

			// If we are updating the list in real time, try not to replace the
			// model
			if (currentServerListModel && !refresh &&
			    currentServerListModel->list.size() == list2.size()) {
				currentServerListModel->ReplaceList(std::move(list2));
				return;
			}

			Handle<ServerListModel> model =
			    Handle<ServerListModel>::New(&GetManager(), helper, std::move(list2));
			serverList->SetModel(model.GetPointerOrNull());
			model->itemActivated = [this](MainScreenServerItem& i) { ServerListItemActivated(i); };
			model->itemDoubleClicked = [this](MainScreenServerItem& i) {
				ServerListItemDoubleClicked(i);
			};
			model->itemRightClicked = [this](MainScreenServerItem& i) {
				ServerListItemRightClicked(i);
			};

			currentServerListModel = model;

			if (refresh)
				serverList->ScrollToTop();
		}

		void MainScreenMainMenu::CheckServerList() {
			if (helper->PollServerListState()) {
				std::vector<Handle<MainScreenServerItem>> list = helper->GetServerList("", false);
				if (list.empty()) {
					// failed.
					// FIXME: show error message?
					loaded = false;
					loading = false;
					errorView->visible = true;
					loadingView->visible = false;
					serverList->SetModel(Handle<ListViewModel>::New().GetPointerOrNull()); // empty
					return;
				}
				loading = false;
				loaded = true;
				errorView->visible = false;
				loadingView->visible = false;
				UpdateServerList();
			}
			
			// A server on this machine answered: show it, even if the master list failed to load
			if (!loading && helper->PollLanServers()) {
				loaded = true;
				errorView->visible = false;
				UpdateServerList();
			}

			if ((static_cast<int>(cg_serverlistSort) & 0xfff) == 0 && loaded) {
				// Ping (RTT) is updated in real-time
				if (serverListUpdateTimer == 0) {
					UpdateServerList(false);
					serverListUpdateTimer = 5;
				}
				--serverListUpdateTimer;
			}
		}

		void MainScreenMainMenu::OnAddressChanged(UIElement&) {
			cg_lastQuickConnectHost = addressField->GetText();
			selectedMapName = "";
		}

		void MainScreenMainMenu::SetProtocolVersion(int ver) {
			cg_protocolVersion = ver;
		}

		void MainScreenMainMenu::OpenVersionDropdown() {
			if (!versionFilterButton)
				return;
			std::vector<std::string> items = {
				_Tr("MainScreen", "Todos"),
				_Tr("MainScreen", "Versión Y"),
				_Tr("MainScreen", "Versión 0.76"),
				_Tr("MainScreen", "Versión 0.75")
			};
			AABB2 b = versionFilterButton->GetScreenBounds();
			float dropW = 125.0F;
			float dropX = b.max.x - dropW;
			ui::ShowDropDownList(&GetManager(), dropX, b.max.y + 2.0F, dropW, std::move(items),
			                     [this](int index) {
				if (index < 0)
					return;
				switch (index) {
					case 0:
						currentVersionFilter = ServerVersionFilter::All;
						if (versionFilterButton)
							versionFilterButton->caption = _Tr("MainScreen", "Todos");
						break;
					case 1:
						currentVersionFilter = ServerVersionFilter::vY;
						cg_protocolVersion = 11;
						if (versionFilterButton)
							versionFilterButton->caption = _Tr("MainScreen", "Versión Y");
						break;
					case 2:
						currentVersionFilter = ServerVersionFilter::v076;
						cg_protocolVersion = 4;
						if (versionFilterButton)
							versionFilterButton->caption = _Tr("MainScreen", "Versión 0.76");
						break;
					case 3:
						currentVersionFilter = ServerVersionFilter::v075;
						cg_protocolVersion = 3;
						if (versionFilterButton)
							versionFilterButton->caption = _Tr("MainScreen", "Versión 0.75");
						break;
					default:
						break;
				}
				UpdateDirectConnectVisibility();
				UpdateServerList();
			});
		}

		void MainScreenMainMenu::LoadWorldList() {
			if (!worldsList)
				return;

			std::vector<std::string> mapFiles = FileManager::EnumFiles("Maps");
			std::vector<WorldItemData> worldItems;
			bool foundLAbeach = false;
			bool foundTitle = false;

			for (const std::string& rawFile : mapFiles) {
				std::string fn = rawFile;
				if (fn.size() > 5 && (fn.substr(0, 5) == "Maps/" || fn.substr(0, 5) == "Maps\\")) {
					fn = fn.substr(5);
				}
				std::string lowerFn = fn;
				std::transform(lowerFn.begin(), lowerFn.end(), lowerFn.begin(), ::tolower);
				if (lowerFn.size() < 4 || lowerFn.substr(lowerFn.size() - 4) != ".vxl")
					continue;

				WorldItemData data;
				data.filename = fn;

				if (lowerFn == "labeach.vxl") {
					foundLAbeach = true;
					data.displayName = "Playa Los Ángeles (LAbeach.vxl)";
					data.description = "aos.party #1303 • Playa tropical con palmeras, arena dorada y muelles";
					data.tag = "[★ Preinstalado]";
					data.sizeText = "2.1 MB";
					worldItems.insert(worldItems.begin(), data);
				} else if (lowerFn == "title.vxl") {
					foundTitle = true;
					data.displayName = "Mundo de Inicio (Title.vxl)";
					data.description = "Escenario principal del lobby interactivo de ySpades";
					data.tag = "[Integrado]";
					data.sizeText = "2.1 MB";
					worldItems.push_back(data);
				} else {
					data.displayName = fn;
					data.description = "Mapa de bloques voxel personalizado (.vxl)";
					data.tag = "[Guardado]";
					data.sizeText = "";
					worldItems.push_back(data);
				}
			}

			if (!foundLAbeach) {
				WorldItemData data;
				data.filename = "LAbeach.vxl";
				data.displayName = "Playa Los Ángeles (LAbeach.vxl)";
				data.description = "aos.party #1303 • Playa tropical con palmeras, arena dorada y muelles";
				data.tag = "[★ Preinstalado]";
				data.sizeText = "2.1 MB";
				worldItems.insert(worldItems.begin(), data);
			}
			if (!foundTitle) {
				WorldItemData data;
				data.filename = "Title.vxl";
				data.displayName = "Mundo de Inicio (Title.vxl)";
				data.description = "Escenario principal del lobby interactivo de ySpades";
				data.tag = "[Integrado]";
				data.sizeText = "2.1 MB";
				worldItems.push_back(data);
			}

			if (selectedWorldMap.empty())
				selectedWorldMap = "LAbeach.vxl";

			Handle<WorldListModel> model =
			    Handle<WorldListModel>::New(&GetManager(), worldItems, selectedWorldMap);
			worldsList->SetModel(model.GetPointerOrNull());
			model->itemActivated = [this](const std::string& fn) { WorldListItemActivated(fn); };
			model->itemDoubleClicked = [this](const std::string& fn) { WorldListItemDoubleClicked(fn); };
		}

		void MainScreenMainMenu::WorldListItemActivated(const std::string& mapName) {
			selectedWorldMap = mapName;
			LoadWorldList();
		}

		void MainScreenMainMenu::WorldListItemDoubleClicked(const std::string& mapName) {
			selectedWorldMap = mapName;
			PlaySelectedWorld();
		}

		void MainScreenMainMenu::PlaySelectedWorld() {
			if (selectedWorldMap.empty())
				selectedWorldMap = "LAbeach.vxl";
			std::string mapToPlay = selectedWorldMap;
			if (mapToPlay.find("Maps/") != 0 && mapToPlay.find("Maps\\") != 0) {
				if (FileManager::FileExists(("Maps/" + mapToPlay).c_str())) {
					mapToPlay = "Maps/" + mapToPlay;
				}
			}
			helper->PlayWorld(mapToPlay);
		}

		void MainScreenMainMenu::UpdateSearchFilterVisibility() {
			if (searchFilterButton)
				searchFilterButton->toggled = showSearchFilter;
			if (filterLabel)
				filterLabel->visible = showSearchFilter;
			if (filterField)
				filterField->visible = showSearchFilter;
			if (filterFullButton)
				filterFullButton->visible = showSearchFilter;

			float sw = GetManager().screenWidth;
			float sh = GetManager().screenHeight;
			float contentsWidth = std::min(sw - 16.0F, 750.0F);
			float contentsLeft = (sw - contentsWidth) * 0.5F;
			float footerPos = sh - 50.0F;
			float headerPos = 260.0F;
			float headerHeight = 24.0F;
			float listPos = headerPos + headerHeight;

			float bottomMargin = showSearchFilter ? 44.0F : 8.0F;
			if (serverList) {
				serverList->SetBounds(
				    AABB2(contentsLeft, listPos, contentsWidth, footerPos - listPos - bottomMargin));
			}
		}

		void MainScreenMainMenu::OnSkinsPressed(UIElement&) {
			Handle<SkinSelectScreen> screen =
			    Handle<SkinSelectScreen>::New(this, &ui->GetFontManager(), ui);
			screen->Run();
		}

		void MainScreenMainMenu::OnModsPressed(UIElement&) {
			Handle<ModWeaponSelectScreen> screen =
			    Handle<ModWeaponSelectScreen>::New(this, &ui->GetFontManager(), ui);
			screen->Run();
		}

		void MainScreenMainMenu::Connect() {
			// Pass selectedMapName if known (server clicked in list); otherwise pass empty
			// and let ConnectServer resolve it from the cached server list on the C++ side.
			std::string msg = helper->ConnectServer(addressField->GetText(),
			                                        static_cast<int>(cg_protocolVersion),
			                                        selectedMapName);
			if (msg.size() > 0) {
				Handle<AlertScreen> al = Handle<AlertScreen>::New(this, msg, 200.0F, true);
				al->Run();
			}
		}

		void MainScreenMainMenu::UpdateDirectConnectVisibility() {
			if (addressField)
				addressField->visible = showDirectConnect;
			if (connectButton)
				connectButton->visible = showDirectConnect;
			if (toggleIpButton)
				toggleIpButton->toggled = showDirectConnect;
			if (goldNoticeLabel)
				goldNoticeLabel->visible = (currentVersionFilter == ServerVersionFilter::vY && !showDirectConnect);
		}

		void MainScreenMainMenu::ReturnToLobby() {
			ui->ShowLobby();
		}

		void MainScreenMainMenu::HotKey(const std::string& key) {
			if (IsEnabled() && key == "Enter") {
				if (demoPanel->visible) {
					PlaySelectedDemo();
				} else if (worldsPanel && worldsPanel->visible) {
					PlaySelectedWorld();
				} else {
					Connect();
				}
			} else if (IsEnabled() && key == "Escape") {
				ReturnToLobby();
			} else {
				UIElement::HotKey(key);
			}
		}

		void MainScreenMainMenu::Render() {
			CheckServerList();
			CheckModsRefresh();
			UIElement::Render();

			// check for client error message.
			if (IsEnabled()) {
				std::string msg = helper->GetPendingErrorMessage();
				if (msg.size() > 0) {
					// try to make the "disconnected" message more friendly.
					std::string findStr = "Disconnected:";
					size_t ind1pos = msg.find(findStr);
					if (ind1pos != std::string::npos) {
						int ind1 = static_cast<int>(ind1pos);
						size_t ind2pos = msg.find('\n', ind1);
						int ind2 = (ind2pos == std::string::npos) ? static_cast<int>(msg.size())
						                                          : static_cast<int>(ind2pos);
						ind1 += static_cast<int>(findStr.size());
						msg = msg.substr(ind1, ind2 - ind1);
						msg = _Tr("MainScreen",
						          "You were disconnected from the server because of the following "
						          "reason:\n\n{0}",
						          msg);
					}

					// failed to connect.
					Handle<AlertScreen> al =
					    Handle<AlertScreen>::New(this, msg, 200.0F, true, true);
					al->Run();
				}
			}
		}
	} // namespace gui
} // namespace spades
