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

#include "HksMenu.h"

#include <algorithm>

#include <Client/ClientUI.h>
#include <Client/ClientUIHelper.h>
#include <Client/Fonts.h>
#include <Client/IRenderer.h>
#include <Core/Strings.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/Settings/Formatters.h>
#include <Gui/UI/Settings/PreferenceLayouter.h>
#include <Gui/UI/Settings/PreferenceView.h>
#include <Gui/UI/Widgets/Button.h>
#include <Gui/UI/Widgets/DrawUtils.h>
#include <Gui/UI/Widgets/Label.h>
#include <Gui/UI/Widgets/TextViewer.h>

namespace spades {
	namespace client {
		using gui::FormatConfigNumber;
		using gui::HeadingNavIndex;
		using gui::NumberFormatter;
		using gui::PreferenceMetrics;
		using gui::PreferenceTabButton;
		using gui::StandardPreferenceLayouter;
		using gui::ui::Label;
		using gui::ui::SetColorNP;
		using gui::ui::TextViewer;
		using gui::ui::UIElement;
		using gui::ui::UIManager;

		HksMenu::HksMenu(ClientUI* ui)
		    : UIElement(&ui->GetUIManager()), ui(ui), helper(&ui->GetHelper()) {
			SetBounds(ui->GetUIManager().GetRootElement().GetBounds());

			UIManager* manager = &GetManager();
			float sw = manager->screenWidth;
			float sh = manager->screenHeight;

			float contentsWidth = sw - 16.0F;
			if (contentsWidth > PreferenceMetrics::maxContentsWidth)
				contentsWidth = PreferenceMetrics::maxContentsWidth;

			contentsHeight = sh - 8.0F;
			if (contentsHeight > 550.0F)
				contentsHeight = 550.0F;

			float contentsTop = (sh - contentsHeight) * 0.5F;
			float contentsLeft = (sw - contentsWidth) * 0.5F;
			contentsRight = contentsLeft + contentsWidth;

			tabWidth = PreferenceMetrics::tabWidth;
			tabRowHeight = 30.0F;
			tabTop = contentsTop + 48.0F;
			tabLeft = contentsLeft + PreferenceMetrics::tabInset;

			float panelBorderOffset = 14.0F;
			panelTop = contentsTop - panelBorderOffset;
			panelBottom = contentsTop + contentsHeight + panelBorderOffset;

			// Dim background overlay
			{
				Handle<Label> overlay = Handle<Label>::New(manager);
				overlay->backgroundColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.7F);
				overlay->SetBounds(AABB2(0.0F, 0.0F, sw, sh));
				AddChild(overlay.GetPointerOrNull());
			}

			// Main settings panel background
			{
				Handle<Label> label = Handle<Label>::New(manager);
				label->backgroundColor = MakeVector4(0.0F, 0.0F, 0.0F, 0.9F);
				label->SetBounds(AABB2(0.0F, panelTop + 1.0F, size.x, (panelBottom - panelTop) - 1.0F));
				AddChild(label.GetPointerOrNull());
			}

			// ==========================================
			// TAB 1: Building & Mining Panel
			// ==========================================
			{
				Handle<UIElement> panel = Handle<UIElement>::New(manager);
				Handle<HeadingNavIndex> nav = Handle<HeadingNavIndex>::New();
				nav->helpHandler = [this](const std::string& text) {
					if (helpView)
						helpView->SetText(text);
				};

				Handle<StandardPreferenceLayouter> layouter =
				    Handle<StandardPreferenceLayouter>::New(panel.GetPointerOrNull(), &ui->GetFontManager());
				layouter->headingNav = nav;

				layouter->AddHeading(_Tr("Preferences", "Walk Building (Scaffold)"));

				layouter->AddToggleField(_Tr("Preferences", "Walk Building (Scaffold)"), "cg_scaffold");
				layouter->AddHelp(_Tr("Preferences",
				    "Enables walk building (scaffold). Automatically targets blocks under your feet or directly ahead as you walk across gaps and open air."));

				layouter->AddToggleField(_Tr("Preferences", "Click to Confirm"), "cg_scaffoldClickConfirm");
				layouter->AddHelp(_Tr("Preferences",
				    "When enabled (default), highlights the block ahead in white and places on left click. When disabled, automatically places blocks continuously while walking."));

				layouter->AddToggleField(_Tr("Preferences", "White Preview Box"), "cg_scaffoldPreview");
				layouter->AddHelp(_Tr("Preferences",
				    "Displays a glowing white preview cube and wireframe ahead showing the exact placement target."));

				layouter->AddToggleField(_Tr("Preferences", "Require Block Tool"), "cg_scaffoldRequireBlock");
				layouter->AddHelp(_Tr("Preferences",
				    "Only activate scaffold walk-building when holding the Block tool in your hands."));

				layouter->AddSliderField(_Tr("Preferences", "Ahead Distance"), "cg_scaffoldDistance",
				                         0.5F, 2.0F, 0.1F, NumberFormatter(1, " blocks"));
				layouter->AddHelp(_Tr("Preferences",
				    "Distance ahead in movement direction to project and build the next bridge block (default 1.0 block)."));

				layouter->AddHeading(_Tr("Preferences", "Block Construction"));

				layouter->AddToggleField(_Tr("Preferences", "Fast Block Placement"), "cg_fastPlace");
				layouter->AddHelp(_Tr("Preferences",
				    "Removes block placement cooldown. Hold left click to paint continuous blocks rapidly without delay."));

				layouter->AddToggleField(_Tr("Preferences", "Maintain Selection While Running"), "cg_maintainBlockSelection");
				layouter->AddHelp(_Tr("Preferences",
				    "Keep block line selection and right-click dragging active while running or sprinting without losing the selection."));

				layouter->AddToggleField(_Tr("Preferences", "Fast Stairs & Steps"), "cg_fastStairs");
				layouter->AddHelp(_Tr("Preferences",
				    "Climb stairs and block steps smoothly at full sprint speed without slowing down or losing momentum."));

				layouter->AddToggleField(_Tr("Preferences", "Infinite Blocks"), "cg_infiniteBlocks");
				layouter->AddHelp(_Tr("Preferences",
				    "Automatically keeps your block inventory replenished at 50 blocks so you never run out while building."));

				layouter->AddHeading(_Tr("Preferences", "Reach Distance"));

				layouter->AddToggleField(_Tr("Preferences", "Extended Reach"), "cg_extendedBuildRange");
				layouter->AddHelp(_Tr("Preferences",
				    "Extends maximum interaction distance for placing blocks, excavating trenches, and spade mining."));

				layouter->AddSliderField(_Tr("Preferences", "Reach Distance"), "cg_buildReach",
				                         3.0F, 24.0F, 1.0F, NumberFormatter(0, " blocks"));
				layouter->AddHelp(_Tr("Preferences",
				    "Maximum reach in blocks when Extended Reach is enabled (Default 8 blocks, up to 24)."));

				layouter->AddHeading(_Tr("Preferences", "Digging & Mining"));

				layouter->AddToggleField(_Tr("Preferences", "Instant Break (1-Hit)"), "cg_instantBreak");
				layouter->AddHelp(_Tr("Preferences",
				    "Destroys blocks with a single spade strike. Mine and clear terrain instantly on building servers."));

				layouter->AddToggleField(_Tr("Preferences", "Fast Spade"), "cg_fastSpade");
				layouter->AddHelp(_Tr("Preferences",
				    "Dramatically accelerates spade swing rate and right-click 3-block trench excavation to 20 hits/sec."));

				layouter->FinishLayout();
				AddTab(panel.GetPointerOrNull(), _Tr("Preferences", "Build"), nav.GetPointerOrNull(),
				       gui::GuiIconType::Building);
			}

			// ==========================================
			// TAB 2: Flight Panel
			// ==========================================
			{
				Handle<UIElement> panel = Handle<UIElement>::New(manager);
				Handle<HeadingNavIndex> nav = Handle<HeadingNavIndex>::New();
				nav->helpHandler = [this](const std::string& text) {
					if (helpView)
						helpView->SetText(text);
				};

				Handle<StandardPreferenceLayouter> layouter =
				    Handle<StandardPreferenceLayouter>::New(panel.GetPointerOrNull(), &ui->GetFontManager());
				layouter->headingNav = nav;

				layouter->AddHeading(_Tr("Preferences", "Flight Controls"));

				layouter->AddToggleField(_Tr("Preferences", "Flight Mode"), "cg_fly");
				layouter->AddHelp(_Tr("Preferences",
				    "Enables smooth 3D free flight. Move with [W][A][S][D], ascend with [Space], and descend with [Crouch] / [Ctrl]. Zero drift on release."));

				layouter->AddToggleField(_Tr("Preferences", "Noclip"), "cg_flyNoclip");
				layouter->AddHelp(_Tr("Preferences",
				    "Allows passing through voxels, terrain, and solid structures without collision."));

				layouter->AddSliderField(_Tr("Preferences", "Flight Speed"), "cg_flySpeed",
				                         0.5F, 15.0F, 0.5F, NumberFormatter(1, "x"));
				layouter->AddHelp(_Tr("Preferences",
				    "Movement speed multiplier while flying (Default 1.0x). Hold [Shift] to sprint (2x turbo) or [Sneak] for slow precision building."));

				layouter->AddToggleField(_Tr("Preferences", "Look-Direction Fly"), "cg_flyLookDir");
				layouter->AddHelp(_Tr("Preferences",
				    "Forward [W] and backward [S] move in the full 3D direction of your crosshair aim (including camera pitch)."));

				layouter->AddToggleField(_Tr("Preferences", "No Fall Damage"), "cg_noFallDamage");
				layouter->AddHelp(_Tr("Preferences",
				    "Negates falling momentum and disables taking impact damage when falling from any height."));

				layouter->FinishLayout();
				AddTab(panel.GetPointerOrNull(), _Tr("Preferences", "Flight"), nav.GetPointerOrNull(),
				       gui::GuiIconType::Flight);
			}

			// ==========================================
			// TAB 3: Ground Movement Panel
			// ==========================================
			{
				Handle<UIElement> panel = Handle<UIElement>::New(manager);
				Handle<HeadingNavIndex> nav = Handle<HeadingNavIndex>::New();
				nav->helpHandler = [this](const std::string& text) {
					if (helpView)
						helpView->SetText(text);
				};

				Handle<StandardPreferenceLayouter> layouter =
				    Handle<StandardPreferenceLayouter>::New(panel.GetPointerOrNull(), &ui->GetFontManager());
				layouter->headingNav = nav;

				layouter->AddHeading(_Tr("Preferences", "Speed & Sprinting"));

				layouter->AddToggleField(_Tr("Preferences", "Fast Stairs & Steps"), "cg_fastStairs");
				layouter->AddHelp(_Tr("Preferences",
				    "Climb stairs and block steps smoothly at full sprint speed without slowing down or losing momentum."));

				layouter->AddToggleField(_Tr("Preferences", "Speedhack"), "cg_speedHack");
				layouter->AddHelp(_Tr("Preferences",
				    "Increases walking and sprinting ground velocity. Automatically steps over 1-block obstacles while sprinting without getting stuck."));

				layouter->AddSliderField(_Tr("Preferences", "Speed Multiplier"), "cg_speedMultiplier",
				                         1.2F, 5.0F, 0.2F, NumberFormatter(1, "x"));
				layouter->AddHelp(_Tr("Preferences",
				    "Velocity multiplier applied to ground movement."));

				layouter->AddHeading(_Tr("Preferences", "Jumping & Bunnyhop"));

				layouter->AddToggleField(_Tr("Preferences", "Super Jump"), "cg_superJump");
				layouter->AddHelp(_Tr("Preferences",
				    "Empowers jump height, allowing you to leap over tall walls and structures."));

				layouter->AddSliderField(_Tr("Preferences", "Jump Multiplier"), "cg_jumpMultiplier",
				                         1.5F, 6.0F, 0.5F, NumberFormatter(1, "x"));
				layouter->AddHelp(_Tr("Preferences",
				    "Jump height multiplier applied when Super Jump is active."));

				layouter->AddToggleField(_Tr("Preferences", "Auto Bunnyhop"), "cg_bunnyhop");
				layouter->AddHelp(_Tr("Preferences",
				    "Automatically jumps the frame you hit the ground as long as [Space] is held."));

				layouter->FinishLayout();
				AddTab(panel.GetPointerOrNull(), _Tr("Preferences", "Movement"), nav.GetPointerOrNull(),
				       gui::GuiIconType::Movement);
			}

			// ==========================================
			// TAB 4: Visuals & ESP Panel
			// ==========================================
			{
				Handle<UIElement> panel = Handle<UIElement>::New(manager);
				Handle<HeadingNavIndex> nav = Handle<HeadingNavIndex>::New();
				nav->helpHandler = [this](const std::string& text) {
					if (helpView)
						helpView->SetText(text);
				};

				Handle<StandardPreferenceLayouter> layouter =
				    Handle<StandardPreferenceLayouter>::New(panel.GetPointerOrNull(), &ui->GetFontManager());
				layouter->headingNav = nav;

				layouter->AddHeading(_Tr("Preferences", "Player ESP"));

				layouter->AddToggleField(_Tr("Preferences", "Player Boxes"), "cg_espBoxes");
				layouter->AddHelp(_Tr("Preferences",
				    "Draws 3D bounding boxes around players through walls and structures."));

				layouter->AddToggleField(_Tr("Preferences", "Player Names"), "cg_espNames");
				layouter->AddHelp(_Tr("Preferences",
				    "Draws player names and distance in blocks above their heads through obstacles."));

				layouter->AddToggleField(_Tr("Preferences", "Snaplines"), "cg_espTracers");
				layouter->AddHelp(_Tr("Preferences",
				    "Draws colored lines from the bottom-center of the screen directly to each player."));

				layouter->AddHeading(_Tr("Preferences", "Head Hitbox & Chroma"));

				layouter->AddToggleField(_Tr("Preferences", "Head Border Box"), "cg_espHead");
				layouter->AddHelp(_Tr("Preferences",
				    "Draws a crisp border box outline directly around the player head hitbox."));

				layouter->AddToggleField(_Tr("Preferences", "RGB Chroma Head Outline"), "cg_espHeadRgb");
				layouter->AddHelp(_Tr("Preferences",
				    "Cycles the player head outline border with an animated rainbow RGB chroma spectrum."));

				layouter->AddHeading(_Tr("Preferences", "Filters & Visibility"));

				layouter->AddToggleField(_Tr("Preferences", "Enemies Only (No Teammates)"), "cg_espEnemiesOnly");
				layouter->AddHelp(_Tr("Preferences",
				    "Excludes teammates from all ESP features (boxes, names, tracers, and head outline borders)."));

				layouter->AddChoiceField(_Tr("Preferences", "Visible-Only ESP"), "cg_espVisibleOnly",
				    {_Tr("Preferences", "OFF"), _Tr("Preferences", "VISIBLE"), _Tr("Preferences", "DIM")},
				    {0, 1, 2});
				layouter->AddHelp(_Tr("Preferences",
				    "Filters ESP by line of sight: OFF shows everyone through walls, VISIBLE hides occluded enemies, and DIM reduces their brightness."));

				layouter->AddHeading(_Tr("Preferences", "Spectator Alerts"));
				layouter->AddToggleField(_Tr("Preferences", "Spectator HUD Alert"), "cg_spectatorHUD");
				layouter->AddHelp(_Tr("Preferences",
				    "Displays real-time notifications on screen when spectators are watching you or flying in the match."));

				layouter->FinishLayout();
				AddTab(panel.GetPointerOrNull(), _Tr("Preferences", "Visuals"), nav.GetPointerOrNull(),
				       gui::GuiIconType::Visuals);
			}

			// ==========================================
			// TAB 5: Combat Panel
			// ==========================================
			{
				Handle<UIElement> panel = Handle<UIElement>::New(manager);
				Handle<HeadingNavIndex> nav = Handle<HeadingNavIndex>::New();
				nav->helpHandler = [this](const std::string& text) {
					if (helpView)
						helpView->SetText(text);
				};

				Handle<StandardPreferenceLayouter> layouter =
				    Handle<StandardPreferenceLayouter>::New(panel.GetPointerOrNull(), &ui->GetFontManager());
				layouter->headingNav = nav;

				layouter->AddHeading(_Tr("Preferences", "Aimbot & Aim Assist"));

				layouter->AddToggleField(_Tr("Preferences", "Aimbot"), "cg_aimbot");
				layouter->AddHelp(_Tr("Preferences",
				    "Automatically aligns crosshair and locks onto the nearest visible enemy head."));

				layouter->AddSliderField(_Tr("Preferences", "Aimbot FOV"), "cg_aimbotFov",
				                         10.0F, 180.0F, 5.0F, NumberFormatter(0, " deg"));
				layouter->AddHelp(_Tr("Preferences",
				    "Maximum field of view angle within which the aimbot will acquire enemy targets."));

				layouter->AddSliderField(_Tr("Preferences", "Aimbot Smoothness"), "cg_aimbotSmooth",
				                         0.0F, 1.0F, 0.05F, NumberFormatter(2, "x"));
				layouter->AddHelp(_Tr("Preferences",
				    "Smoothness of aim correction. 0.00 = Instant snap, higher values = smooth gentle assist."));

				layouter->AddHeading(_Tr("Preferences", "Triggerbot & Automatic Actions"));

				layouter->AddToggleField(_Tr("Preferences", "Triggerbot"), "cg_triggerbot");
				layouter->AddHelp(_Tr("Preferences",
				    "Automatically fires your weapon the instant an enemy enters your crosshair."));

				layouter->AddToggleField(_Tr("Preferences", "Rapid Fire (Semi-Auto)"), "cg_rapidFire");
				layouter->AddHelp(_Tr("Preferences",
				    "Fires semi-automatic rifles and shotguns continuously at maximum cyclic rate while holding fire."));

				layouter->AddToggleField(_Tr("Preferences", "Auto Reload"), "cg_autoReload");
				layouter->AddHelp(_Tr("Preferences",
				    "Automatically initiates reload as soon as your active weapon magazine runs out of ammo."));

				layouter->AddHeading(_Tr("Preferences", "Accuracy & Recoil"));

				layouter->AddToggleField(_Tr("Preferences", "No Recoil"), "cg_noRecoil");
				layouter->AddHelp(_Tr("Preferences",
				    "Completely removes weapon muzzle rise and camera recoil kick when firing."));

				layouter->AddToggleField(_Tr("Preferences", "No Spread"), "cg_noSpread");
				layouter->AddHelp(_Tr("Preferences",
				    "Removes all bullet and pellet dispersion. Weapons shoot with laser pinpoint accuracy."));

				layouter->FinishLayout();
				AddTab(panel.GetPointerOrNull(), _Tr("Preferences", "Combat"), nav.GetPointerOrNull(),
				       gui::GuiIconType::Combat);
			}

			// Left sidebar Back to Game button (below tab buttons)
			float backBtnY = tabTop + static_cast<float>(tabs.size()) * tabRowHeight + 5.0F;
			{
				Handle<PreferenceTabButton> backBtn =
				    Handle<PreferenceTabButton>::New(manager, gui::GuiIconType::Back);
				backBtn->caption = _Tr("Preferences", "Back");
				backBtn->hotKeyText = _Tr("Client", "[Esc] / [Ins]");
				backBtn->SetBounds(AABB2(tabLeft, backBtnY, tabWidth, tabRowHeight));
				backBtn->activated = [this](UIElement&) { Close(); };
				AddChild(backBtn.GetPointerOrNull());
			}

			// Left sidebar help viewer
			{
				float helpTop = backBtnY + tabRowHeight + 10.0F;
				float helpH = std::max(0.0F, panelBottom - 10.0F - helpTop);
				Handle<TextViewer> help = Handle<TextViewer>::New(manager);
				help->parseInlineCode = true;
				help->SetBounds(AABB2(tabLeft, helpTop, tabWidth, helpH));
				AddChild(help.GetPointerOrNull());
				helpView = help.GetPointerOrNull();
			}
		}

		void HksMenu::AddTab(UIElement* panel, const std::string& caption, HeadingNavIndex* nav,
		                     gui::GuiIconType icon) {
			UIManager* manager = &GetManager();
			float tabY = tabTop + static_cast<float>(tabs.size()) * tabRowHeight;

			Handle<PreferenceTabButton> tabBtn = Handle<PreferenceTabButton>::New(manager, icon);
			tabBtn->toggle = true;
			tabBtn->caption = caption;
			tabBtn->SetBounds(AABB2(tabLeft, tabY, tabWidth, tabRowHeight));

			int tabIndex = static_cast<int>(tabs.size());
			tabBtn->activated = [this, tabIndex](UIElement&) {
				SelectTab(tabIndex);
			};
			AddChild(tabBtn.GetPointerOrNull());

			float tabRight = tabLeft + tabWidth;
			panel->SetBounds(AABB2(tabRight, tabTop, contentsRight - tabRight, contentsHeight - (tabTop - (panelTop + 14.0F))));
			panel->visible = (tabs.empty());
			AddChild(panel);

			HksMenuTab tab;
			tab.panel = panel;
			tab.button = tabBtn.GetPointerOrNull();
			tab.nav = nav;
			tabs.push_back(tab);

			if (tabs.size() == 1) {
				tabBtn->toggled = true;
			}
		}

		void HksMenu::SelectTab(int idx) {
			selectedTabIndex = idx;
			for (size_t i = 0; i < tabs.size(); i++) {
				bool active = (static_cast<int>(i) == idx);
				tabs[i].button->toggled = active;
				tabs[i].panel->visible = active;
			}
			if (helpView)
				helpView->SetText(std::string());
		}

		void HksMenu::HotKey(const std::string& key) {
			if (IsEnabled() && (key == "Escape" || key == "Insert")) {
				Close();
			} else {
				UIElement::HotKey(key);
			}
		}

		bool HksMenu::IsActive() const { return IsEnabled(); }
		void HksMenu::Open() {}
		void HksMenu::Close() {
			if (ui)
				ui->SetActiveUI(nullptr);
		}

		bool HksMenu::KeyEvent(const std::string& key, bool down) {
			if (down && (key == "Escape" || key == "Insert")) {
				Close();
				return true;
			}
			if (down)
				KeyDown(key);
			else
				KeyUp(key);
			return true;
		}

		void HksMenu::TextInputEvent(const std::string& text) { KeyPress(text); }
		bool HksMenu::AcceptsTextInput() const { return IsEnabled(); }
		void HksMenu::Draw() { Render(); }

		void HksMenu::Render() {
			client::IRenderer& r = GetManager().GetRenderer();
			Vector2 pos = GetScreenPosition();
			Vector2 sz = size;

			SetColorNP(r, MakeVector4(1.0F, 1.0F, 1.0F, 0.07F));
			r.DrawImage(nullptr, AABB2(pos.x, pos.y + panelTop, sz.x, 1.0F));
			r.DrawImage(nullptr, AABB2(pos.x, pos.y + panelBottom, sz.x, 1.0F));

			// Top amethyst glowing line for HKS panel
			SetColorNP(r, MakeVector4(0.85F, 0.45F, 1.0F, 0.85F));
			r.DrawImage(nullptr, AABB2(pos.x, pos.y + panelTop, sz.x, 2.0F));

			// Draw HKS Title and Subtitle in left sidebar header
			client::FontManager& fm = ui->GetFontManager();
			client::IFont& largeFont = fm.GetLargeFont();
			client::IFont& guiFont = fm.GetGuiFont();

			Vector2 titlePos = MakeVector2(pos.x + tabLeft + 4.0F, pos.y + (panelTop + 14.0F) + 2.0F);
			largeFont.DrawShadow("HKS", titlePos, 1.25F,
			                     MakeVector4(0.92F, 0.55F, 1.0F, 1.0F),
			                     MakeVector4(0.0F, 0.0F, 0.0F, 0.9F));

			Vector2 subPos = MakeVector2(pos.x + tabLeft + 4.0F, pos.y + (panelTop + 14.0F) + 26.0F);
			guiFont.DrawShadow("CHEAT & ASSISTANT", subPos, 0.70F,
			                   MakeVector4(0.70F, 0.78F, 0.95F, 0.80F),
			                   MakeVector4(0.0F, 0.0F, 0.0F, 0.7F));

			// Accent divider line below HKS title
			SetColorNP(r, MakeVector4(0.85F, 0.45F, 1.0F, 0.45F));
			r.DrawImage(nullptr, AABB2(pos.x + tabLeft, pos.y + (panelTop + 14.0F) + 40.0F, tabWidth - 6.0F, 1.5F));

			UIElement::Render();
		}
	} // namespace client
} // namespace spades
