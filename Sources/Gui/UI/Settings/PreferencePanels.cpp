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

#include "PreferenceView.h"
#include <Core/Settings.h>
#include <Core/Strings.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/MainScreen/SelectCountryScreen.h>
#include <Gui/ProxyManager.h>
#include <Gui/UI/Widgets/Button.h>
#include <Gui/SDLRunner.h>

SPADES_SETTING(cg_country);
SPADES_SETTING(cg_countryIsSet);
SPADES_SETTING(r_videoWidth);
SPADES_SETTING(r_videoHeight);
SPADES_SETTING(r_fullscreen);
SPADES_SETTING(r_vsync);
SPADES_SETTING(r_multisamples);
SPADES_SETTING(r_renderer);
SPADES_SETTING(cl_fps);
DEFINE_SPADES_SETTING(r_videoPreset, "0");
SPADES_SETTING(cl_customClientName);
SPADES_SETTING(cl_customLocale);
SPADES_SETTING(cg_unicode);
SPADES_SETTING(net_useProxy);
SPADES_SETTING(net_proxyType);
SPADES_SETTING(net_proxyServer);
SPADES_SETTING(net_proxyAuth);
SPADES_SETTING(net_proxyFile);

namespace spades {
	namespace gui {
		using ui::SimpleButton;
		using ui::UIElement;
		using ui::UIManager;

		// -- GameOptionsPanel --

		GameOptionsPanel::GameOptionsPanel(UIManager* manager, const PreferenceViewOptions& options,
		                                   client::FontManager* fontManager, HeadingNavIndex* nav)
		    : UIElement(manager) {
			layouter = Handle<StandardPreferenceLayouter>::New(this, fontManager);
			StandardPreferenceLayouter& l = *layouter;
			l.headingNav = nav;

			l.AddHeading(_Tr("Preferences", "Player Information"));
			ConfigField* nameField =
			    l.AddInputField(_Tr("Preferences", "Player Name"), "cg_playerName",
			                    !options.gameActive);
			nameField->maxLength = 15;
			nameField->denyNonAscii = false;
			nameField->removeNewlines = true;

			l.AddButtonField(_Tr("Preferences", "Country Flag"), _Tr("Preferences", "Change Flag..."),
			    [this, fontManager](UIElement&) {
				    UIElement* topOwner = this;
				    while (topOwner->GetParent() &&
				           topOwner->GetParent() != &GetManager().GetRootElement()) {
					    topOwner = topOwner->GetParent();
				    }

				    Handle<SelectCountryScreen> sc = Handle<SelectCountryScreen>::New(
				        topOwner, fontManager,
				        [](const std::string& code) {
					        cg_country = code;
					        cg_countryIsSet = 1;
					        Settings::GetInstance()->Flush();
				        },
				        true /* canCancel */
				    );
				    sc->Run();
			    });

			l.AddHeading(_Tr("Preferences", "Gameplay"));
			l.AddChoiceField(_Tr("Preferences", "Aiming Animation"), "cg_animations",
			                 {_Tr("Preferences", "FAST"), _Tr("Preferences", "NORMAL"),
			                  _Tr("Preferences", "OFF")},
			                 {2, 1, 0});
			l.AddToggleField(_Tr("Preferences", "Full Aim Down Sight"), "cg_trueAimDownSight");
			l.AddHelp(_Tr("Preferences",
				"Lets the viewmodel reach full aiming. When off, ADS "
				"stops halfway (partial / hip-style aim)."));
			l.AddToggleField(_Tr("Preferences", "Classic Weapon Recoil"), "cg_classicWeaponRecoil");
			l.AddHelp(_Tr("Preferences",
				"Original `AoS 0.75` recoil (stronger while jumping/moving, especially with "
				"the SMG). When off, uses the more controllable `OpenSpades 0.1.3` style."));
			l.AddToggleField(_Tr("Preferences", "Classic Sprinting"), "cg_classicSprinting");
			l.AddHelp(_Tr("Preferences",
				"Sprint input stays active even while standing still. When "
				"off, sprint cancels as soon as you stop moving."));
			l.AddToggleField(_Tr("Preferences", "Classic Zoom"), "cg_classicZoom");
			l.AddHelp(_Tr("Preferences",
				"Uses the same zoom level for every weapon, `AoS 0.75` behavior. When off, each "
				"weapon has its own zoom: rifle stronger, SMG moderate, shotgun weaker."));
			l.AddToggleField(_Tr("Preferences", "Automatic Unscope"), "cg_autoUnscope");
			l.AddHelp(_Tr("Preferences",
				"Automatically exits aim down sights after firing (except with the SMG). "
				"Useful for faster follow-up shots with rifle and shotgun."));
			l.AddBadgeNew();
			l.AddChoiceField(_Tr("Preferences", "Armas de Otros Jugadores"), "cg_simpleOtherWeapons",
			                 {_Tr("Preferences", "Normal"), _Tr("Preferences", "Básicas"),
			                  _Tr("Preferences", "Baja (FPS)")},
			                 {0, 1, 2});
			l.AddHelp(_Tr("Preferences", "Optimiza las armas de los otros jugadores a modelos básicos o de baja calidad para evitar caídas bruscas de FPS con mods de armas."));

			l.AddHeading(_Tr("Preferences", "Effects"));
			l.AddChoiceField(_Tr("Preferences", "Blood"), "cg_blood",
			                 {_Tr("Preferences", "MARKS"), _Tr("Preferences", "NORMAL"),
			                  _Tr("Preferences", "OFF")},
			                 {2, 1, 0});
			l.AddToggleField(_Tr("Preferences", "Ragdoll Corpses"), "cg_ragdoll");
			l.AddToggleField(_Tr("Preferences", "Enhanced Ragdoll Physics"), "cg_corpseLineCollision");
			l.AddChoiceField(_Tr("Preferences", "Bullet Tracers"), "cg_tracers",
			                 {_Tr("Preferences", "ON"), _Tr("Preferences", "3rd Person"),
			                  _Tr("Preferences", "OFF")},
			                 {1, 2, 0});
			l.AddChoiceField(_Tr("Preferences", "Muzzle Flashes"), "cg_muzzleFire",
			                 {_Tr("Preferences", "ON"), _Tr("Preferences", "3rd Person"),
			                  _Tr("Preferences", "OFF")},
			                 {1, 2, 0});
			l.AddToggleField(_Tr("Preferences", "Eject Bullet Casings"), "cg_ejectBrass");
			l.AddToggleField(_Tr("Preferences", "Hurt Screen Effects"), "cg_hurtScreenEffects");
			l.AddToggleField(_Tr("Preferences", "Heal Screen Effects"), "cg_healScreenEffects");
			l.AddChoiceField(_Tr("Preferences", "Camera Shake"), "cg_shake",
			                 {_Tr("Preferences", "MORE"), _Tr("Preferences", "NORMAL"),
			                  _Tr("Preferences", "OFF")},
			                 {2, 1, 0});
			l.AddChoiceField(_Tr("Preferences", "Particles"), "cg_particles",
			                 {_Tr("Preferences", "MORE"), _Tr("Preferences", "NORMAL"),
			                  _Tr("Preferences", "OFF")},
			                 {2, 1, 0});
			l.AddToggleField(_Tr("Preferences", "Water Splash"), "cg_waterImpact");

			l.AddHeading(_Tr("Preferences", "Feedbacks"));
			l.AddChoiceField(_Tr("Preferences", "Center Messages"), "cg_centerMessage",
			                 {_Tr("Preferences", "NORMAL"), _Tr("Preferences", "LESS")}, {2, 0});
			l.AddChoiceField(_Tr("Preferences", "Center Messages Size"), "cg_centerMessageSmallFont",
			                 {_Tr("Preferences", "NORMAL"), _Tr("Preferences", "SMALL")}, {0, 1},
			                 !options.gameActive);
			l.AddToggleField(_Tr("Preferences", "Ignore Chat Messages"), "cg_ignoreChatMessages");
			l.AddToggleField(_Tr("Preferences", "Ignore Private Messages"), "cg_ignorePrivateMessages");
			l.AddToggleField(_Tr("Preferences", "Traductor de Chat Ultra Ligero (Auto ES)"), "cg_chatTranslate");
			l.AddHelp(_Tr("Preferences", "Traduce en vivo los mensajes del chat al español en un hilo secundario sin consumir CPU ni bajar FPS."));
			l.AddSliderField(_Tr("Preferences", "Master Volume"), "s_volume", 0, 100, 1,
			                 NumberFormatter(0, "%"));
			l.AddVolumeSlider(_Tr("Preferences", "Death Camera Volume"), "cg_deathSoundGain");
			l.AddVolumeSlider(_Tr("Preferences", "Respawn Beep Volume"), "cg_respawnSoundGain");
			l.AddVolumeSlider(_Tr("Preferences", "Hit Feedback Volume"), "cg_hitFeedbackSoundGain");
			l.AddVolumeSlider(_Tr("Preferences", "Headshot Feedback Volume"),
			                  "cg_headshotFeedbackSoundGain");
			l.AddVolumeSlider(_Tr("Preferences", "Chat Notify Sounds"), "cg_chatBeep");
			l.AddToggleField(_Tr("Preferences", "Show Alerts"), "cg_alerts");
			l.AddToggleField(_Tr("Preferences", "Show Server Alerts"), "cg_serverAlert");
			l.AddVolumeSlider(_Tr("Preferences", "Alert Sounds"), "cg_alertSounds");
			l.AddToggleField(_Tr("Preferences", "Hit Log"), "cg_hitLog");
			l.AddHelp(_Tr("Preferences",
				"Logs each of your hits with the target name, distance, and body "
				"part. Messages appear in the chat window."));
			l.AddToggleField(_Tr("Preferences", "Hit Indicator"), "cg_hitIndicator");

			l.AddHeading(_Tr("Preferences", "Viewmodel"));
			l.AddToggleField(_Tr("Preferences", "Weapon Keychains"), "cg_weaponCharms");
			l.AddHelp(_Tr("Preferences",
				"Shows decorative keychains and charms hanging from weapons. "
				"Only visible if the active weapon skin supports them."));
			l.AddViewmodelPresetField(
			    _Tr("Preferences", "Viewmodel Position"),
			    {_Tr("Preferences", "Default"), _Tr("Preferences", "Balanced"),
			     _Tr("Preferences", "Minimal")},
			    {0.0F, -0.1F, -0.25F}, {0.0F, 0.05F, 0.25F}, {0.0F, 0.12F, 0.3F});
			l.AddSliderField(_Tr("Preferences", "Viewmodel Alignment"), "cg_viewWeaponSide", -1, 1,
			                 0.1F, ViewmodelSideFormatter());
			l.AddControl(_Tr("Preferences", "Switch Viewmodel Handedness"), "cg_keyToggleLeftHand");
			l.AddToggleField(_Tr("Preferences", "Hide Player Arms"), "cg_hideArms");
			l.AddToggleField(_Tr("Preferences", "Hide Player Body"), "cg_hideBody");
			l.AddToggleField(_Tr("Preferences", "Classic Player Model"), "cg_classicPlayerModels");
			l.AddHelp(_Tr("Preferences",
				"Uses the original `AoS 0.75` player models instead of the new ones."));
			l.AddToggleField(_Tr("Preferences", "Classic Viewmodel"), "cg_classicViewWeapon");
			l.AddHelp(_Tr("Preferences",
				"Uses the original `AoS 0.75` first-person weapon positions and "
				"animations instead of the new ones."));

			l.AddHeading(_Tr("Preferences", "Misc"));
			l.AddSliderField(_Tr("Preferences", "Field of View"), "cg_fov", 45, 110, 1,
			                 FOVFormatter());
			l.AddToggleField(_Tr("Preferences", "Horizontal FOV"), "cg_horizontalFov");
			l.AddHelp(_Tr("Preferences",
				"Applies the FOV value to the horizontal axis instead of the vertical. "
				"Useful on ultrawide monitors to keep a consistent horizontal view."));
			l.AddToggleField(_Tr("Preferences", "Environmental Audio"), "cg_environmentalAudio");
			l.AddHelp(_Tr("Preferences",
				"Adds environmental reverb to weapon sounds based on the surroundings. "
				"When off, gunshots sound more direct and consistent."));
			l.AddToggleField(_Tr("Preferences", "Skip dead players (death cam)"), "cg_skipDeadPlayersWhenDead");
			l.AddHelp(_Tr("Preferences",
				"When dead, other dead players will be skipped when cycling through players. When "
				"off, you can also follow dead players."));
			l.AddToggleField(_Tr("Preferences", "Save Final Score Screenshot"), "cg_autoScreenshot");
			l.AddHelp(_Tr("Preferences",
				"Automatically takes a screenshot of the scoreboard when a team wins."));
			l.AddToggleField(_Tr("Preferences", "Debug Hit Detection"), "cg_debugHitTest");
			l.AddHelp(_Tr("Preferences",
				"Shows a debug overlay of player hitboxes and bullet impact points "
				"after each shot. Useful for diagnosing hit registration issues."));
			l.AddSliderField(_Tr("Preferences", "Hit Debugger Size"), "cg_debugHitTestSize", 64, 256,
			                 8, NumberFormatter(0, "px"));
			l.AddControl(_Tr("Preferences", "Toggle Hit Debugger Zoom"), "cg_keyToggleHitTestZoom");
			l.AddSliderField(_Tr("Preferences", "Hit Debugger Fade Time"), "cg_debugHitTestFadeTime",
			                 1, 20, 1, NumberFormatter(0, "s"));
			l.AddToggleField(_Tr("Preferences", "Debug Weapon Spread"), "cg_debugAim");
			l.AddHelp(_Tr("Preferences",
				"Draws a visual indicator of the current weapon's spread cone. "
				"Useful for understanding accuracy while moving, jumping, or aiming."));
			l.AddToggleField(_Tr("Preferences", "Debug Block Cursor"), "cg_debugBlockCursor");
			l.AddToggleField(_Tr("Preferences", "Debug Players Hitbox"), "cg_debugPlayerHitboxes");
			l.AddToggleField(_Tr("Preferences", "Debug Weapon Anchor Points"),
			                 "cg_debugToolSkinAnchors");

			l.FinishLayout();
		}

		// -- InterfaceOptions --

		void InterfaceOptions::OnEditHUDClicked(UIElement& sender) {
			if (onEditHUDRequested)
				onEditHUDRequested(sender);
		}

		InterfaceOptions::InterfaceOptions(UIManager* manager, const PreferenceViewOptions& options,
		                                   client::FontManager* fontManager, HeadingNavIndex* nav)
		    : UIElement(manager) {
			layouter = Handle<StandardPreferenceLayouter>::New(this, fontManager);
			StandardPreferenceLayouter& l = *layouter;
			l.headingNav = nav;

			l.AddHeading(_Tr("Preferences", "World Markers"));
			l.AddChoiceField(_Tr("Preferences", "Damage Indicators"), "cg_damageIndicators",
			                 {_Tr("Preferences", "OFF"), _Tr("Preferences", "ON"),
			                  _Tr("Preferences", "Grenades")},
			                 {0, 1, 2});
			l.AddToggleField(_Tr("Preferences", "Show Hover Player Names"), "cg_playerNames");
			l.AddToggleField(_Tr("Preferences", "Show Dead Player Names"), "cg_playerNamesDead");

			l.AddHeading(_Tr("Preferences", "Heads-Up Display"));
			l.AddToggleField(_Tr("Preferences", "Hide HUD"), "cg_hideHud");
			l.AddSliderField(_Tr("Preferences", "HUD Color"), "cg_hudColor", 0, 10, 1,
			                 HUDColorFormatter());
			l.AddRGBSlider(_Tr("Preferences", "Custom Color"),
			               {"cg_hudColorR", "cg_hudColorG", "cg_hudColorB"});
			l.AddButtonField(_Tr("Preferences", "HUD Edge Positions"), _Tr("Preferences", "Edit"),
			                 [this](UIElement& s) { OnEditHUDClicked(s); }, options.gameActive);
			l.AddChoiceField(_Tr("Preferences", "HUD Ammo Style"), "cg_hudAmmoStyle",
			                 {_Tr("Preferences", "NORMAL"), _Tr("Preferences", "SIMPLE")}, {0, 1});
			l.AddToggleField(_Tr("Preferences", "Show Tool Hotbar"), "cg_hudHotbar");
			// The compass also needs the server's permission, so turning this on does
			// not put one on screen where the server does not allow it.
			l.AddToggleField(_Tr("Preferences", "Show Compass Bar"), "cg_hudCompassBar");
			l.AddToggleField(_Tr("Preferences", "Indicador de Equipo Ingame"), "cg_hudTeamIndicator");
			l.AddBadgeNew();
			l.AddChoiceField(_Tr("Preferences", "Show Alive Player Count"), "cg_hudPlayerCount",
			                 {_Tr("Preferences", "OFF"), _Tr("Preferences", "Top"),
			                  _Tr("Preferences", "Bottom")},
			                 {0, 1, 2});
			l.AddChoiceField(_Tr("Preferences", "Show Client Statistics"), "cg_stats",
			                 {_Tr("Preferences", "OFF"), _Tr("Preferences", "Top"),
			                  _Tr("Preferences", "Bottom")},
			                 {0, 2, 1});
			l.AddToggleField(_Tr("Preferences", "Small Stats Font"), "cg_statsSmallFont");
			l.AddChoiceField(_Tr("Preferences", "Superposición FPS y Ping WiFi"), "cg_showFpsMs",
			                 {_Tr("Preferences", "Apagado"), _Tr("Preferences", "FPS + ms"),
			                  _Tr("Preferences", "Detallado")},
			                 {0, 1, 2});
			l.AddToggleField(_Tr("Preferences", "Show Player Statistics"), "cg_playerStats");
			l.AddToggleField(_Tr("Preferences", "Show Placed Blocks"),
			                 "cg_playerStatsShowPlacedBlocks");
			l.AddSliderField(_Tr("Preferences", "Player Statistics Height"), "cg_playerStatsHeight",
			                 0, 100, 1, NumberFormatter(0, "px"));
			l.AddSliderField(_Tr("Preferences", "Chat Height"), "cg_chatHeight", 10, 100, 1,
			                 NumberFormatter(0, "px"));
			l.AddSliderField(_Tr("Preferences", "Chat Fade Time"), "cg_chatFadeTime", 5, 20, 1,
			                 NumberFormatter(0, "s"));
			l.AddSliderField(_Tr("Preferences", "Killfeed Height"), "cg_killfeedHeight", 10, 100, 1,
			                 NumberFormatter(0, "px"));
			l.AddSliderField(_Tr("Preferences", "Killfeed Fade Time"), "cg_killfeedFadeTime", 5, 20,
			                 1, NumberFormatter(0, "s"));
			l.AddToggleField(_Tr("Preferences", "Killfeed Icons"), "cg_killfeedIcons");
			l.AddToggleField(_Tr("Preferences", "Show Dominations"), "cg_killfeedStreaks");
			l.AddHelp(_Tr("Preferences",
				"Shows domination and multi-kill streak messages in the killfeed "
				"(e.g. when you repeatedly kill the same player)."));

			l.AddHeading(_Tr("Preferences", "Minimap"));
			l.AddSliderField(_Tr("Preferences", "Minimap Size"), "cg_minimapSize", 128, 256, 8,
			                 NumberFormatter(0, "px"));
			l.AddSliderField(_Tr("Preferences", "Minimap Scale Mode"), "cg_minimapScaleMode", 0, 3, 1,
			                 NumberFormatter(0, ""));
			l.AddSliderField(_Tr("Preferences", "Minimap Opacity"), "cg_minimapOpacity", 0.1F, 1,
			                 0.1F, NumberFormatter(1, ""));
			l.AddToggleField(_Tr("Preferences", "Use Weapon Icons"), "cg_minimapPlayerIcon");
			l.AddToggleField(_Tr("Preferences", "Use Random Colors"), "cg_minimapPlayerColor");
			l.AddChoiceField(_Tr("Preferences", "Show Map Location"), "cg_minimapCoords",
			                 {_Tr("Preferences", "OFF"), _Tr("Preferences", "Bottom"),
			                  _Tr("Preferences", "Side")},
			                 {0, 1, 2});
			l.AddToggleField(_Tr("Preferences", "Show Player Names"), "cg_minimapPlayerNames");
			l.AddToggleField(_Tr("Preferences", "Circular Minimap"), "cg_minimapCircular");
			l.AddToggleField(_Tr("Preferences", "Rotating Minimap"), "cg_minimapRotating");

			l.AddHeading(_Tr("Preferences", "Target"));
			l.AddTargetPreview();
			l.AddHeading("");
			l.AddChoiceField(_Tr("Preferences", "Target Type"), "cg_target",
			                 {_Tr("Preferences", "OFF (Desactivado)"),
			                  _Tr("Preferences", "Predeterminado"),
			                  _Tr("Preferences", "Personalizado"),
			                  _Tr("Preferences", "Clásico HD-Plus"),
			                  _Tr("Preferences", "Punto HD"),
			                  _Tr("Preferences", "Cruz Pro"),
			                  _Tr("Preferences", "Cruz + Punto HD"),
			                  _Tr("Preferences", "Círculo Táctico"),
			                  _Tr("Preferences", "Chevron / Flecha")},
			                 {0, 1, 2, 3, 4, 5, 6, 7, 8});
			l.AddToggleField(_Tr("Preferences", "Lines"), "cg_targetLines");
			l.AddSliderField(_Tr("Preferences", "Color"), "cg_targetColor", 0, 6, 1,
			                 TargetColorFormatter());
			l.AddRGBSlider(_Tr("Preferences", "Custom Color"),
			               {"cg_targetColorR", "cg_targetColorG", "cg_targetColorB"});
			l.AddSliderField(_Tr("Preferences", "Opacity"), "cg_targetAlpha", 0, 255, 1,
			                 NumberFormatter(0, ""));
			l.AddSliderGroup(_Tr("Preferences", "Length"),
			                 {"cg_targetSizeHorizontal", "cg_targetSizeVertical"}, 1, 10, 1, 0,
			                 {"", ""});
			l.AddSliderField(_Tr("Preferences", "Thickness"), "cg_targetThickness", 1, 4, 1,
			                 NumberFormatter(0, "px"));
			l.AddSliderField(_Tr("Preferences", "Gap"), "cg_targetGap", -10, 10, 1,
			                 NumberFormatter(0, "px"));
			l.AddToggleField(_Tr("Preferences", "Dynamic"), "cg_targetDynamic");
			l.AddSliderField(_Tr("Preferences", "Dynamic Split Dist"), "cg_targetDynamicSplitdist", 1,
			                 20, 1, NumberFormatter(0, ""));
			l.AddToggleField(_Tr("Preferences", "Outline"), "cg_targetOutline");
			l.AddRGBSlider(_Tr("Preferences", "Outline Color"),
			               {"cg_targetOutlineColorR", "cg_targetOutlineColorG",
			                "cg_targetOutlineColorB"});
			l.AddSliderField(_Tr("Preferences", "Outline Opacity"), "cg_targetOutlineAlpha", 0, 255,
			                 1, NumberFormatter(0, ""));
			l.AddSliderField(_Tr("Preferences", "Outline Thickness"), "cg_targetOutlineThickness", 1,
			                 4, 1, NumberFormatter(0, "px"));
			l.AddToggleField(_Tr("Preferences", "Rounded Corners Style"),
			                 "cg_targetOutlineRoundedStyle");
			l.AddToggleField(_Tr("Preferences", "Center Dot"), "cg_targetDot");
			l.AddRGBSlider(_Tr("Preferences", "Center Dot Color"),
			               {"cg_targetDotColorR", "cg_targetDotColorG", "cg_targetDotColorB"});
			l.AddSliderField(_Tr("Preferences", "Center Dot Opacity"), "cg_targetDotAlpha", 0, 255, 1,
			                 NumberFormatter(0, ""));
			l.AddSliderField(_Tr("Preferences", "Center Dot Thickness"), "cg_targetDotThickness", 1,
			                 4, 1, NumberFormatter(0, "px"));
			l.AddToggleField(_Tr("Preferences", "T Style"), "cg_targetTStyle");

			l.AddHeading(_Tr("Preferences", "Scope"));
			l.AddScopePreview();
			l.AddHeading("");
			l.AddChoiceField(_Tr("Preferences", "Reflex Sight"), "cg_reflexScope",
			                 {_Tr("Preferences", "OFF"), _Tr("Preferences", "Dot"),
			                  _Tr("Preferences", "Cross")},
			                 {0, 3, 4});
			l.AddSliderField(_Tr("Preferences", "Scope Type"), "cg_pngScope", 0, 3, 1,
			                 ScopeTypeFormatter());
			l.AddToggleField(_Tr("Preferences", "Lines"), "cg_scopeLines");
			l.AddRGBSlider(_Tr("Preferences", "Color"),
			               {"cg_scopeColorR", "cg_scopeColorG", "cg_scopeColorB"});
			l.AddSliderField(_Tr("Preferences", "Opacity"), "cg_scopeAlpha", 0, 255, 1,
			                 NumberFormatter(0, ""));
			l.AddSliderGroup(_Tr("Preferences", "Length"),
			                 {"cg_scopeSizeHorizontal", "cg_scopeSizeVertical"}, 1, 10, 1, 0,
			                 {"", ""});
			l.AddSliderField(_Tr("Preferences", "Thickness"), "cg_scopeThickness", 1, 4, 1,
			                 NumberFormatter(0, "px"));
			l.AddSliderField(_Tr("Preferences", "Gap"), "cg_scopeGap", -10, 10, 1,
			                 NumberFormatter(0, "px"));
			l.AddToggleField(_Tr("Preferences", "Dynamic"), "cg_scopeDynamic");
			l.AddSliderField(_Tr("Preferences", "Dynamic Split Dist"), "cg_scopeDynamicSplitdist", 1,
			                 20, 1, NumberFormatter(0, ""));
			l.AddToggleField(_Tr("Preferences", "Outline"), "cg_scopeOutline");
			l.AddRGBSlider(_Tr("Preferences", "Outline Color"),
			               {"cg_scopeOutlineColorR", "cg_scopeOutlineColorG",
			                "cg_scopeOutlineColorB"});
			l.AddSliderField(_Tr("Preferences", "Outline Opacity"), "cg_scopeOutlineAlpha", 0, 255, 1,
			                 NumberFormatter(0, ""));
			l.AddSliderField(_Tr("Preferences", "Outline Thickness"), "cg_scopeOutlineThickness", 1,
			                 4, 1, NumberFormatter(0, "px"));
			l.AddToggleField(_Tr("Preferences", "Rounded Corners Style"),
			                 "cg_scopeOutlineRoundedStyle");
			l.AddToggleField(_Tr("Preferences", "Center Dot"), "cg_scopeDot");
			l.AddRGBSlider(_Tr("Preferences", "Center Dot Color"),
			               {"cg_scopeDotColorR", "cg_scopeDotColorG", "cg_scopeDotColorB"});
			l.AddSliderField(_Tr("Preferences", "Center Dot Opacity"), "cg_scopeDotAlpha", 0, 255, 1,
			                 NumberFormatter(0, ""));
			l.AddSliderField(_Tr("Preferences", "Center Dot Thickness"), "cg_scopeDotThickness", 1, 4,
			                 1, NumberFormatter(0, "px"));
			l.AddToggleField(_Tr("Preferences", "T Style"), "cg_scopeTStyle");

			l.FinishLayout();
		}

		// -- RendererOptionsPanel --

		RendererOptionsPanel::RendererOptionsPanel(UIManager* manager,
		                                           const PreferenceViewOptions& options,
		                                           client::FontManager* fontManager,
		                                           HeadingNavIndex* nav)
		    : UIElement(manager) {
			(void)options;
			layouter = Handle<StandardPreferenceLayouter>::New(this, fontManager);
			StandardPreferenceLayouter& l = *layouter;
			l.headingNav = nav;

			l.AddHeading(_Tr("Preferences", "General"));
			l.AddButtonField(_Tr("Preferences", "Resolución y Video"), _Tr("Preferences", "Detallado..."),
			    [this](UIElement&) {
				    UIElement* p = this;
				    while (p && !dynamic_cast<PreferenceView*>(p))
					    p = p->GetParent();
				    if (auto* pv = dynamic_cast<PreferenceView*>(p)) {
					    pv->Close();
					    if (pv->onDetailedRequested)
						    pv->onDetailedRequested(*pv);
				    }
			    });
			l.AddSliderField(_Tr("Preferences", "Render Scale"), "r_scale", 0.2F, 1, 0.01F,
			                 NumberFormatter(0, "%", "", 100));
			l.AddSliderField(_Tr("Preferences", "Render Scaling Filter"), "r_scaleFilter", 0, 2, 1,
			                 RenderScaleFormatter());
			l.AddToggleField(_Tr("Preferences", "Rendering Statistics"), "r_debugTiming");
			l.AddToggleField(_Tr("Preferences", "Allow CPU Rendering"), "r_allowSoftwareRendering");
			l.AddHelp(_Tr("Preferences",
				"Allows OpenGL to fall back to a software implementation if no "
				"GPU is available. Very slow; mainly useful on Linux. Prefer the "
				"dedicated Software renderer instead."));

			l.AddHeading(_Tr("Preferences", "World"));
			l.AddToggleField(_Tr("Preferences", "Niebla (Fog)"), "cg_fog");
			l.AddHelp(_Tr("Preferences",
				"Activa o desactiva la niebla del juego. Desactivarla permite ver con total nitidez a través de todo el mapa."));
			l.AddToggleField(_Tr("Preferences", "Dynamic Lights"), "r_dlights");
			l.AddToggleField(_Tr("Preferences", "Tracers Lights"), "cg_tracerLights");
			l.AddToggleField(_Tr("Preferences", "Depth Prepass"), "r_depthPrepass");
			l.AddHelp(_Tr("Preferences",
				"Fills the depth buffer before shading so hidden pixels can be "
				"skipped. Usually improves performance. Forced on when SSAO is enabled."));
			l.AddToggleField(_Tr("Preferences", "Occlusion Querying"), "r_occlusionQuery");
			l.AddHelp(_Tr("Preferences",
				"Uses GPU occlusion queries when rendering water to skip it when "
				"fully hidden. Can improve FPS, but may crash some drivers."));
			l.AddToggleField(_Tr("Preferences", "Object Outlines"), "r_outlines");
			l.AddHelp(_Tr("Preferences",
				"Draws cartoon-style outlines around players and map. "
				"OpenGL renderer only."));

			l.AddHeading(_Tr("Preferences", "Post-processing"));
			l.AddToggleField(_Tr("Preferences", "Depth Of Field"), "r_depthOfField");
			l.AddToggleField(_Tr("Preferences", "Camera Blur"), "r_cameraBlur");
			l.AddToggleField(_Tr("Preferences", "FXAA Anti-Aliasing"), "r_fxaa");
			l.AddToggleField(_Tr("Preferences", "Lens Flares"), "r_lensFlare");
			l.AddToggleField(_Tr("Preferences", "Flares Lights"), "r_lensFlareDynamic");
			l.AddToggleField(_Tr("Preferences", "Color Correction"), "r_colorCorrection");
			l.AddSliderField(_Tr("Preferences", "Sharpening"), "r_sharpen", 0, 1, 0.1F,
			                 NumberFormatter(1, ""));
			l.AddSliderField(_Tr("Preferences", "Saturation"), "r_saturation", 0, 2, 0.1F,
			                 NumberFormatter(1, ""));
			l.AddSliderField(_Tr("Preferences", "Exposure"), "r_exposureValue", -18, 18, 0.1F,
			                 NumberFormatter(1, "EV"));

			l.AddHeading(_Tr("Preferences", "Software Renderer"));
			l.AddSliderField(_Tr("Preferences", "Thread Count"), "r_swNumThreads", 1, 16, 1,
			                 NumberFormatter(0, _Tr("Preferences", " Threads")));
			l.AddSliderField(_Tr("Preferences", "Undersampling"), "r_swUndersampling", 0, 4, 1,
			                 NumberFormatter(0, "x"));

			l.FinishLayout();
		}

		// -- VentanaOptionsPanel --

		VentanaOptionsPanel::VentanaOptionsPanel(UIManager* manager,
		                                         const PreferenceViewOptions& options,
		                                         client::FontManager* fontManager,
		                                         HeadingNavIndex* nav)
		    : UIElement(manager) {
			(void)options;
			layouter = Handle<StandardPreferenceLayouter>::New(this, fontManager);
			StandardPreferenceLayouter& l = *layouter;
			l.headingNav = nav;

			// ==========================================
			// 1. VENTANA++ CLÁSICA
			// ==========================================
			l.AddHeading(_Tr("Preferences", "Ventana++"));
			l.AddButtonField(_Tr("Preferences", "Ventana Clásica"), _Tr("Preferences", "Abrir Ventana++"),
			    [this](UIElement&) {
				    UIElement* p = this;
				    while (p && !dynamic_cast<PreferenceView*>(p))
					    p = p->GetParent();
				    if (auto* pv = dynamic_cast<PreferenceView*>(p)) {
					    auto cbVentana = pv->onVentanaRequested;
					    auto cbDetailed = pv->onDetailedRequested;
					    UIElement* owner = pv->GetOwner();
					    pv->Close();
					    if (cbVentana)
						    cbVentana(owner ? *owner : *this);
					    else if (cbDetailed)
						    cbDetailed(owner ? *owner : *this);
				    }
			    });
			l.AddHelp(_Tr("Preferences",
				"Abre la ventana clásica completa de configuración con resolución nativa, pantalla completa, SSAO, filtros, audio y proxies."));

			// ==========================================
			// 2. PANTALLA & GRÁFICOS (FOTO SETUP CLÁSICA)
			// ==========================================
			l.AddHeading(_Tr("Preferences", "Pantalla & Gráficos"));
			l.AddHelp(_Tr("Preferences",
				"Configuración idéntica a la pantalla clásica de inicio: resoluciones nativas, modo de pantalla, renderizador, antialiasing y HDR."));

			l.AddChoiceField(_Tr("Preferences", "Resolución de Pantalla"), "r_videoPreset",
			                 {"1080p", "900p", "768p", "720p"},
			                 {0, 1, 2, 3});
			l.AddHelp(_Tr("Preferences", "Selecciona una resolución nativa común. Pulsa '¡GUARDAR Y APLICAR CAMBIOS!' al final para aplicarla de inmediato."));

			l.AddToggleField(_Tr("Preferences", "Pantalla Completa (Fullscreen)"), "r_fullscreen");
			l.AddHelp(_Tr("Preferences", "Ejecuta el juego en pantalla completa exclusiva o ventana. También puedes alternar con Alt+Enter."));

			l.AddChoiceStringField(_Tr("Preferences", "Renderizador Gráfico"), "r_renderer",
			                       {"OpenGL", "Software"},
			                       {"gl", "sw"});
			l.AddHelp(_Tr("Preferences", "OpenGL aprovecha tu tarjeta gráfica para máxima fluidez. Software renderiza por CPU si tu GPU no es compatible."));

			l.AddChoiceField(_Tr("Preferences", "Suavizado de Bordes (MSAA)"), "r_multisamples",
			                 {"Apagado", "2x MSAA", "4x MSAA"},
			                 {0, 2, 4});
			l.AddHelp(_Tr("Preferences", "Antialiasing geométrico por hardware. Genera una imagen limpia eliminando los dientes de sierra en los bloques."));

			l.AddToggleField(_Tr("Preferences", "Filtro Antialiasing FXAA"), "r_fxaa");
			l.AddHelp(_Tr("Preferences", "Suavizado de bordes rápido por post-procesamiento. Puede usarse en conjunto con MSAA."));

			l.AddToggleField(_Tr("Preferences", "Iluminación Global (Radiosity)"), "r_radiosity");
			l.AddHelp(_Tr("Preferences", "Simulación física de la trayectoria de los rayos de luz para iluminación y sombras ambientales más realistas."));

			l.AddToggleField(_Tr("Preferences", "Renderizado Lineal HDR"), "r_hdr");
			l.AddHelp(_Tr("Preferences", "Usa un rango dinámico más amplio y cálculo de color lineal con energía real para mezclas de iluminación precisas."));

			l.AddChoiceField(_Tr("Preferences", "Sincronización Vertical (V-Sync)"), "r_vsync",
			                 {"Off", "On", "Adapt"},
			                 {0, 1, -1});
			l.AddHelp(_Tr("Preferences", "Sincroniza la tasa de refresco con el monitor para eliminar cortes horizontales de imagen (screen tearing)."));

			// ==========================================
			// 2. EFECTOS & RENDIMIENTO
			// ==========================================
			l.AddHeading(_Tr("Preferences", "Efectos & Rendimiento"));
			l.AddHelp(_Tr("Preferences",
				"Efectos visuales atmosféricos, partículas, iluminación dinámica, agua y filtros de rendimiento idénticos al panel setup."));

			l.AddChoiceField(_Tr("Preferences", "Calidad de Sombras"), "r_shadowMapQuality",
			                 {"Apagado", "Rápido", "Suave"},
			                 {0, 1, 2});
			l.AddHelp(_Tr("Preferences", "Sombras dinámicas proyectadas desde la luz solar o antorchas."));

			l.AddChoiceField(_Tr("Preferences", "Oclusión Ambiental SSAO"), "r_ssaoQuality",
			                 {"Apagado", "Rápido", "Completo"},
			                 {0, 1, 2});
			l.AddHelp(_Tr("Preferences", "Sombreado suave en rincones y uniones de bloques para mayor profundidad tridimensional."));

			l.AddToggleField(_Tr("Preferences", "Niebla del Juego"), "cg_fog");
			l.AddHelp(_Tr("Preferences", "Activa o desactiva la niebla del mapa. Desactivarla permite ver con total nitidez a través de todo el mapa."));

			l.AddSliderField(_Tr("Preferences", "Límite de FPS (Max FPS)"), "cl_fps", 0.0F, 360.0F, 10.0F,
			                 FPSFormatter());
			l.AddHelp(_Tr("Preferences", "Fija un tope máximo de fotogramas por segundo para optimizar temperaturas y fluidez."));

			l.AddChoiceField(_Tr("Preferences", "Superposición FPS y Ping WiFi"), "cg_showFpsMs",
			                 {_Tr("Preferences", "Apagado"), _Tr("Preferences", "FPS + ms"),
			                  _Tr("Preferences", "Detallado")},
			                 {0, 1, 2});
			l.AddHelp(_Tr("Preferences", "Superpone contador ultra optimizado de FPS y ping (ms de WiFi) con barra de estabilidad verde/amarillo/rojo."));

			l.AddChoiceField(_Tr("Preferences", "Armas de Otros Jugadores"), "cg_simpleOtherWeapons",
			                 {_Tr("Preferences", "Normal"), _Tr("Preferences", "Básicas"),
			                  _Tr("Preferences", "Baja (FPS)")},
			                 {0, 1, 2});
			l.AddHelp(_Tr("Preferences", "Optimiza las armas de los otros jugadores a modelos básicos o de baja calidad para evitar caídas bruscas de FPS con mods de armas."));

			l.AddToggleField(_Tr("Preferences", "Indicador de Equipo Ingame"), "cg_hudTeamIndicator");
			l.AddHelp(_Tr("Preferences", "Muestra la insignia flotante con tu equipo en pantalla (se oculta automáticamente al escribir en el chat)."));

			l.AddToggleField(_Tr("Preferences", "Traductor de Chat Ultra Ligero"), "cg_chatTranslate");
			l.AddHelp(_Tr("Preferences", "Traduce en vivo los mensajes del chat al español en un hilo secundario sin consumir CPU ni bajar FPS."));

			l.AddToggleField(_Tr("Preferences", "Mostrar Ventana al Iniciar"), "cl_showStartupWindow");
			l.AddHelp(_Tr("Preferences", "Muestra la pantalla de configuración clásica cada vez que ejecutas el cliente."));

			// ==========================================
			// 3. IDENTIDAD & GUARDAR CAMBIOS
			// ==========================================
			l.AddHeading(_Tr("Preferences", "Identidad y Guardar"));
			l.AddHelp(_Tr("Preferences",
				"Datos visibles para otros jugadores y guardado definitivo de la configuración."));

			l.AddInputField(_Tr("Preferences", "Nombre del Cliente"), "cl_customClientName", true, false);
			l.AddHelp(_Tr("Preferences", "Nombre de cliente reportado al servidor y visible para otros usuarios al consultar tu cliente (ej. ySpades, OpenSpades 0.1.6)."));

			l.AddButtonField(_Tr("Preferences", "Confirmar Ajustes"), _Tr("Preferences", "¡GUARDAR Y APLICAR CAMBIOS!"),
			    [](UIElement& btn) {
				    int preset = r_videoPreset;
				    switch (preset) {
					    case 0: r_videoWidth = 1920; r_videoHeight = 1080; break;
					    case 1: r_videoWidth = 1600; r_videoHeight = 900; break;
					    case 2: r_videoWidth = 1366; r_videoHeight = 768; break;
					    case 3: r_videoWidth = 1280; r_videoHeight = 720; break;
					    case 4: r_videoWidth = 1024; r_videoHeight = 768; break;
					    case 5: r_videoWidth = 800; r_videoHeight = 600; break;
				    }

				    SDLRunner::ApplyWindowVideoMode();

				    SPLog("Ventana++: ¡Ajustes guardados y aplicados con éxito!");
				    if (auto* b = dynamic_cast<SimpleButton*>(&btn)) {
					    b->caption = "¡APLICADO CORRECTAMENTE!";
				    }
			    });
			l.AddHelp(_Tr("Preferences", "Guarda todos los ajustes en disco (spades.json) y aplica de inmediato resolución, pantalla completa e identidad."));

			l.FinishLayout();
		}

		// -- ProxyOptionsPanel --

		ProxyOptionsPanel::ProxyOptionsPanel(UIManager* manager,
		                                     const PreferenceViewOptions& options,
		                                     client::FontManager* fontManager,
		                                     HeadingNavIndex* nav)
		    : UIElement(manager) {
			(void)options;
			layouter = Handle<StandardPreferenceLayouter>::New(this, fontManager);
			StandardPreferenceLayouter& l = *layouter;
			l.headingNav = nav;

			l.AddHeading(_Tr("Preferences", "Gestor de Proxies & Red"));
			l.AddHelp(_Tr("Preferences",
				"Configura un servidor proxy para anonimizar tu conexión y proteger tu IP pública al unirte a servidores de OpenSpades."));

			l.AddToggleField(_Tr("Preferences", "Activar Conexión Proxy"), "net_useProxy");
			l.AddHelp(_Tr("Preferences", "Habilita o deshabilita el enrutamiento de red a través del servidor proxy configurado."));

			l.AddChoiceStringField(_Tr("Preferences", "Tipo de Protocolo Proxy"), "net_proxyType",
			                       {"SOCKS5", "HTTP"},
			                       {"socks5", "http"});
			l.AddHelp(_Tr("Preferences", "SOCKS5 soporta UDP del juego y tráfico web. HTTP solo enruta lista de servidores."));

			l.AddInputField(_Tr("Preferences", "Servidor Proxy (IP:Puerto)"), "net_proxyServer", true, false);
			l.AddHelp(_Tr("Preferences", "Introduce directamente la IP y el puerto del servidor proxy (ejemplo: 209.50.190.137:1081)."));

			l.AddInputField(_Tr("Preferences", "Autenticación (Opcional)"), "net_proxyAuth", true, false);
			l.AddHelp(_Tr("Preferences", "Credenciales opcionales usuario:contraseña si tu proxy requiere autenticación privada."));

			l.AddInputField(_Tr("Preferences", "Archivo Proxies (Opcional)"), "net_proxyFile", true, false);
			l.AddHelp(_Tr("Preferences", "Ruta opcional a un archivo .txt con lista de proxies (ej. proxies.txt)."));

			l.AddButtonField(_Tr("Preferences", "Cargar desde Archivo"), _Tr("Preferences", "Cargar Archivo Opcional"),
			    [](UIElement& btn) {
				    std::string file = net_proxyFile.CString();
				    bool ok = ProxyManager::GetInstance().LoadProxyList(file, net_proxyType.CString());
				    if (auto* b = dynamic_cast<SimpleButton*>(&btn)) {
					    if (ok) {
						    size_t count = ProxyManager::GetInstance().GetLoadedProxies().size();
						    b->caption = "¡" + std::to_string(count) + " Proxies Cargados!";
					    } else {
						    b->caption = "Error: no se encontró archivo";
					    }
				    }
			    });
			l.AddHelp(_Tr("Preferences", "Carga la lista de proxies desde el archivo .txt especificado en el campo de texto (ej. proxy.txt)."));

			l.AddButtonField(_Tr("Preferences", "Probar Proxy Activo"), _Tr("Preferences", "Comprobar Latencia"),
			    [](UIElement& btn) {
				    std::string res = ProxyManager::GetInstance().TestActiveProxy();
				    SPLog("Prueba de Proxy: %s", res.c_str());
				    if (auto* b = dynamic_cast<SimpleButton*>(&btn)) {
					    b->caption = res.empty() ? "Error" : res;
				    }
			    });
			l.AddHelp(_Tr("Preferences", "Envía una solicitud al servidor maestro para verificar si el proxy responde y medir la latencia."));

			l.AddButtonField(_Tr("Preferences", "Confirmar Ajustes"), _Tr("Preferences", "¡Guardar Cambios!"),
			    [](UIElement& btn) {
				    Settings::GetInstance()->Flush();
				    bool isEnabled = (static_cast<int>(net_useProxy) != 0);
				    if (isEnabled) {
					    ProxyManager::GetInstance().TestActiveProxy();
					    SPLog("Proxy: Ajustes guardados. Proxy ACTIVADO.");
				    } else {
					    SPLog("Proxy: Ajustes guardados. Proxy APAGADO (conexión directa).");
				    }
				    if (auto* b = dynamic_cast<SimpleButton*>(&btn)) {
					    if (isEnabled) {
						    b->caption = "¡Proxy Guardado (Activado)!";
					    } else {
						    b->caption = "¡Proxy Guardado (Apagado)!";
					    }
				    }
			    });
			l.AddHelp(_Tr("Preferences", "Guarda permanentemente el estado y ajustes de proxy en spades.json."));

			l.FinishLayout();
		}

		// -- ControlOptionsPanel --

		ControlOptionsPanel::ControlOptionsPanel(UIManager* manager,
		                                         const PreferenceViewOptions& options,
		                                         client::FontManager* fontManager,
		                                         HeadingNavIndex* nav)
		    : UIElement(manager) {
			(void)options;
			layouter = Handle<StandardPreferenceLayouter>::New(this, fontManager);
			StandardPreferenceLayouter& l = *layouter;
			l.headingNav = nav;

			l.AddHeading(_Tr("Preferences", "Weapons/Tools"));
			l.AddControl(_Tr("Preferences", "Attack"), "cg_keyAttack");
			l.AddControl(_Tr("Preferences", "Alt. Attack"), "cg_keyAltAttack");
			l.AddToggleField(_Tr("Preferences", "Hold Aim Down Sight"), "cg_holdAimDownSight");
			l.AddSliderField(_Tr("Preferences", "Mouse Sensitivity Type"), "cg_mouseSensScale", 0, 5,
			                 1, SensScaleFormatter());
			l.AddSliderField(_Tr("Preferences", "Mouse Sensitivity"), "cg_mouseSensitivity", 0.1F, 10,
			                 0.1F, NumberFormatter(1, ""));
			l.AddSliderField(_Tr("Preferences", "ADS Mouse Sens. Scale"), "cg_zoomedMouseSensScale",
			                 0.05F, 3, 0.05F, NumberFormatter(2, "x"));
			l.AddToggleField(_Tr("Preferences", "Mouse Acceleration"), "cg_mouseAccel");
			l.AddHelp(_Tr("Preferences",
				"Scales mouse sensitivity by movement speed. Fast flicks turn more, "
				"slow movements less. When off, response is linear."));
			l.AddSliderField(_Tr("Preferences", "Exponential Power"), "cg_mouseExpPower", 0.5F, 1.5F,
							 0.02F, NumberFormatter(2, "", "^"));
			l.AddHelp(_Tr("Preferences",
				"Curve strength. Above 1.0 dampens slow movements, "
				"below 1.0 boosts them. Requires `{0}`.",
				_Tr("Preferences", "Mouse Acceleration")));
			l.AddToggleField(_Tr("Preferences", "Invert Y-axis Mouse Input"), "cg_invertMouseY");
			l.AddControl(_Tr("Preferences", "Reload"), "cg_keyReloadWeapon");
			l.AddControl(_Tr("Preferences", "Equip Spade"), "cg_keyToolSpade");
			l.AddControl(_Tr("Preferences", "Equip Block"), "cg_keyToolBlock");
			l.AddControl(_Tr("Preferences", "Build Block Underneath"), "cg_keyPlaceUnder");
			l.AddControl(_Tr("Preferences", "Equip Weapon"), "cg_keyToolWeapon");
			l.AddControl(_Tr("Preferences", "Equip Grenade"), "cg_keyToolGrenade");
			l.AddControl(_Tr("Preferences", "Last Used Tool"), "cg_keyLastTool");
			l.AddPlusMinusField(_Tr("Preferences", "Switch Tools by Wheel"), "cg_switchToolByWheel");

			l.AddHeading(_Tr("Preferences", "Movement"));
			l.AddControl(_Tr("Preferences", "Move Forward"), "cg_keyMoveForward");
			l.AddControl(_Tr("Preferences", "Move Backward"), "cg_keyMoveBackward");
			l.AddControl(_Tr("Preferences", "Move Left"), "cg_keyMoveLeft");
			l.AddControl(_Tr("Preferences", "Move Right"), "cg_keyMoveRight");
			l.AddControl(_Tr("Preferences", "Crouch"), "cg_keyCrouch");
			l.AddControl(_Tr("Preferences", "Sneak"), "cg_keySneak");
			l.AddControl(_Tr("Preferences", "Jump"), "cg_keyJump");
			l.AddControl(_Tr("Preferences", "Sprint"), "cg_keySprint");

			l.AddHeading(_Tr("Preferences", "Color Palette"));
			l.AddControl(_Tr("Preferences", "Capture Color"), "cg_keyCaptureColor");
			l.AddControl(_Tr("Preferences", "Navigate up"), "cg_keyPaletteUp");
			l.AddControl(_Tr("Preferences", "Navigate down"), "cg_keyPaletteDown");
			l.AddControl(_Tr("Preferences", "Navigate left"), "cg_keyPaletteLeft");
			l.AddControl(_Tr("Preferences", "Navigate right"), "cg_keyPaletteRight");
			l.AddControl(_Tr("Preferences", "Toggle extended palette"), "cg_keyExtendedPalette");

			l.AddHeading(_Tr("Preferences", "Misc"));
			l.AddControl(_Tr("Preferences", "Scoreboard"), "cg_keyScoreboard");
			l.AddToggleField(_Tr("Preferences", "Hold Large Map"), "cg_holdMapZoom");
			l.AddControl(_Tr("Preferences", "Minimap Scale"), "cg_keyChangeMapScale");
			l.AddControl(_Tr("Preferences", "Toggle Map"), "cg_keyToggleMapZoom");
			l.AddControl(_Tr("Preferences", "Flashlight"), "cg_keyFlashlight");
			l.AddControl(_Tr("Preferences", "Global Chat"), "cg_keyGlobalChat");
			l.AddControl(_Tr("Preferences", "Team Chat"), "cg_keyTeamChat");
			l.AddControl(_Tr("Preferences", "Chat Log"), "cg_keyChatLog");
			l.AddControl(_Tr("Preferences", "Chat Zoom"), "cg_keyZoomChatLog");
			l.AddControl(_Tr("Preferences", "Pie Menu"), "cg_keyPieMenu");
			l.AddBadgeNew();
			l.AddControl(_Tr("Preferences", "Show Teammates"), "cg_keyTeamOverlay");
			l.AddHelp(_Tr("Preferences",
				"Hold to show teammates through walls (team ESP). "
				"Only works on servers that allow it."));
			l.AddBadgeNew();
			l.AddControl(_Tr("Preferences", "Team Ping"), "cg_keyTeamPing");
			l.AddHelp(_Tr("Preferences",
				"Marks a point at the crosshair for your team. "
				"Requires server support for the teamplay extension."));
			l.AddBadgeNew();
			l.AddControl(_Tr("Preferences", "Limbo Menu"), "cg_keyLimbo");
			l.AddControl(_Tr("Preferences", "Save Map"), "cg_keySaveMap");
			l.AddControl(_Tr("Preferences", "Save Sceneshot"), "cg_keySceneshot");
			l.AddControl(_Tr("Preferences", "Save Screenshot"), "cg_keyScreenshot");
			l.AddControl(_Tr("Preferences", "Master Volume Up"), "cg_keyVolumeUp");
			l.AddControl(_Tr("Preferences", "Master Volume Down"), "cg_keyVolumeDown");
			l.AddControl(_Tr("Preferences", "Network Graph"), "cg_keyNetgraph");
			l.AddControl(_Tr("Preferences", "Force Spectator Mode"), "cg_keyStaffSpectating");
			l.AddHelp(_Tr("Preferences",
				"Toggles staff spectator mode. Lets you free-cam and follow players "
				"without leaving your team (intended for admins/staff)."));
			l.FinishLayout();
		}

		// -- MiscOptionsPanel --

		MiscOptionsPanel::MiscOptionsPanel(UIManager* manager, const PreferenceViewOptions& options,
		                                   client::FontManager* fontManager, HeadingNavIndex* nav)
		    : UIElement(manager) {
			(void)options;
			layouter = Handle<StandardPreferenceLayouter>::New(this, fontManager);
			StandardPreferenceLayouter& l = *layouter;
			l.headingNav = nav;

			l.AddHeading(_Tr("Preferences", "General"));
			ConfigSlider* fpsSlider = l.AddSliderField(_Tr("Preferences", "FPS Limit"), "cl_fps", 0,
			                                           300, 1, FPSFormatter());
			fpsSlider->mapper = [](float v) { return (v < 20) ? 0.0F : v; };
			l.AddSliderField(_Tr("Preferences", "Console Scrollback Lines"),
			                 "cl_consoleScrollbackLines", 100, 2000, 100, NumberFormatter(0, ""));
			l.AddInputField(_Tr("Preferences", "Server List URL"), "cl_serverListUrl", true, true);
			l.AddInputField(_Tr("Preferences", "Server List Alt. URL"), "cl_serverListUrlFallback",
			                true, true);
			l.AddInputField(_Tr("Preferences", "Mod List URL"), "cl_modsIndexUrl", true, true);
			l.AddToggleField(_Tr("Preferences", "Allow Unicode"), "cg_unicode");
			l.AddHelp(_Tr("Preferences",
				"Lets you use other alphabets and symbols (such as Cyrillic or Japanese) "
				"in chat and in your name. When off, they are replaced by a placeholder."));
			l.AddChoiceStringField(_Tr("Preferences", "Screenshot Format"), "cg_screenshotFormat",
			                       {"PNG", "JPEG", "TGA"}, {"png", "jpeg", "tga"});
			l.AddSliderField(_Tr("Preferences", "JPEG Quality"), "core_jpegQuality", 1, 100, 1,
			                 NumberFormatter(0, "%"));
			l.AddToggleField(_Tr("Preferences", "Enable Startup Window"), "cl_showStartupWindow");
			l.AddHelp(_Tr("Preferences",
				"Shows the setup window every time you start the game. When off, goes "
				"straight to the main menu."));
			l.AddToggleField(_Tr("Preferences", "Show Update Prompt"), "cl_zsUpdatePrompt");
			l.AddHelp(_Tr("Preferences",
				"Shows a notice on the main menu when a newer version is available."));
			l.AddHeading(_Tr("Preferences", "Demo Recording"));
			l.AddControl(_Tr("Preferences", "Start/Stop Recording"), "cg_keyDemoRecord");
			l.AddToggleField(_Tr("Preferences", "Auto Record"), "cg_demoAutoRecord");
			l.AddToggleField(_Tr("Preferences", "Auto Delete Old Demos"), "cg_demoAutoPrune");
			l.AddSliderField(_Tr("Preferences", "Max Stored Demos"), "cg_demoMaxFiles", 1, 256, 1,
			                 NumberFormatter(0, ""));

			l.AddHeading(_Tr("Preferences", "Demo Playback"));
			l.AddControl(_Tr("Preferences", "Pause/Resume"), "cg_keyDemoPlayPause");
			l.AddControl(_Tr("Preferences", "Speed Up"), "cg_keyDemoSpeedUp");
			l.AddControl(_Tr("Preferences", "Slow Down"), "cg_keyDemoSlowDown");
			l.AddControl(_Tr("Preferences", "Seek Forward"), "cg_keyDemoSeekForward");
			l.AddControl(_Tr("Preferences", "Seek Backward"), "cg_keyDemoSeekBackward");
			l.AddControl(_Tr("Preferences", "Toggle Demo HUD"), "cg_keyDemoToggleHud");

			l.FinishLayout();
		}

		// -- DetailedOptionsPanel --

		DetailedOptionsPanel::DetailedOptionsPanel(UIManager* manager,
		                                           const PreferenceViewOptions& options,
		                                           client::FontManager* fontManager,
		                                           HeadingNavIndex* nav)
		    : UIElement(manager) {
			(void)options;
			layouter = Handle<StandardPreferenceLayouter>::New(this, fontManager);
			StandardPreferenceLayouter& l = *layouter;
			l.headingNav = nav;

			l.AddHeading(_Tr("Preferences", "Pantalla y Ventana"));
			l.AddToggleField(_Tr("Preferences", "Pantalla Completa"), "r_fullscreen");
			l.AddHelp(_Tr("Preferences", "Ejecuta el juego en pantalla completa o en modo ventana."));
			l.AddToggleField(_Tr("Preferences", "Sincronización Vertical (VSync)"), "r_vsync");
			l.AddHelp(_Tr("Preferences", "Sincroniza los fotogramas con la tasa de refresco del monitor para evitar cortes."));
			l.AddSliderField(_Tr("Preferences", "Límite de FPS (Max FPS)"), "cl_fps", 0.0F, 360.0F, 10.0F,
			                 FPSFormatter());
			l.AddHelp(_Tr("Preferences", "Fija un límite máximo de cuadros por segundo. 0 significa sin límite."));

			l.AddHeading(_Tr("Preferences", "Gráficos Avanzados y Sombras"));
			l.AddToggleField(_Tr("Preferences", "Niebla (Fog)"), "cg_fog");
			l.AddHelp(_Tr("Preferences", "Permite activar o desactivar completamente la niebla para ver con total nitidez a través de todo el mapa."));
			l.AddToggleField(_Tr("Preferences", "Niebla Volumétrica"), "r_fogShadow");
			l.AddHelp(_Tr("Preferences", "Efecto de niebla volumétrica 3D con sombras y dispersión de luz atmosférica."));
			l.AddToggleField(_Tr("Preferences", "Bloom / Resplandor"), "r_bloom");
			l.AddHelp(_Tr("Preferences", "Efecto de resplandor para trazadoras, explosiones y fuentes de alta luminosidad."));
			l.AddToggleField(_Tr("Preferences", "Desenfoque de Cámara"), "r_cameraBlur");
			l.AddHelp(_Tr("Preferences", "Desenfoque de movimiento cinemático durante giros rápidos de la cámara."));
			l.AddToggleField(_Tr("Preferences", "Partículas Suaves"), "r_softParticles");
			l.AddHelp(_Tr("Preferences", "Suaviza la unión geométrica de las partículas de humo y explosiones con el terreno."));
			l.AddToggleField(_Tr("Preferences", "Sombras de Modelos"), "r_modelShadows");
			l.AddHelp(_Tr("Preferences", "Genera sombras dinámicas proyectadas desde los modelos de jugadores."));
			l.AddToggleField(_Tr("Preferences", "Iluminación Global (Radiosity)"), "r_radiosity");
			l.AddHelp(_Tr("Preferences", "Calcula rebote de iluminación indirecta sobre los bloques del mundo."));
			l.AddSliderField(_Tr("Preferences", "Calidad de Agua"), "r_water", 0.0F, 2.0F, 1.0F,
			                 NumberFormatter(0, ""));
			l.AddHelp(_Tr("Preferences", "0 = Agua simple opaca, 1 = Translúcida, 2 = Reflejos y cáusticas avanzadas."));

			l.AddHeading(_Tr("Preferences", "Rendimiento y Optimización"));
			l.AddChoiceField(_Tr("Preferences", "Superposición FPS y Ping WiFi"), "cg_showFpsMs",
			                 {_Tr("Preferences", "Apagado"), _Tr("Preferences", "FPS + ms"),
			                  _Tr("Preferences", "Detallado")},
			                 {0, 1, 2});
			l.AddHelp(_Tr("Preferences", "Superpone contador ultra optimizado de FPS y ping (ms de WiFi) con barra de estabilidad verde/amarillo/rojo."));
			l.AddChoiceField(_Tr("Preferences", "Armas de Otros Jugadores (FPS)"), "cg_simpleOtherWeapons",
			                 {_Tr("Preferences", "Normal"),
			                  _Tr("Preferences", "Básicas (FPS)")},
			                 {0, 1});
			l.AddHelp(_Tr("Preferences", "Usa modelos básicos y ligeros para otros jugadores, evitando caídas bruscas de FPS si tienes mods de armas instalados."));
			l.AddToggleField(_Tr("Preferences", "Depth Prepass"), "r_depthPrepass");
			l.AddHelp(_Tr("Preferences", "Rellena el búfer de profundidad primero para descartar píxeles ocultos. Aumenta FPS."));
			l.AddToggleField(_Tr("Preferences", "Occlusion Querying"), "r_occlusionQuery");
			l.AddHelp(_Tr("Preferences", "Utiliza consultas por hardware para omitir geometrías totalmente ocluidas."));
			l.AddToggleField(_Tr("Preferences", "Permitir Renderizado CPU"), "r_allowSoftwareRendering");
			l.AddHelp(_Tr("Preferences", "Permite usar el renderizador por software si la GPU falla o no es compatible."));
			l.AddSliderField(_Tr("Preferences", "Hilos CPU Software"), "r_swNumThreads", 1.0F, 16.0F, 1.0F,
			                 NumberFormatter(0, _Tr("Preferences", " Threads")));
			l.AddHelp(_Tr("Preferences", "Número de hilos de trabajo paralelos dedicados al renderizado por software."));

			l.FinishLayout();
		}
	} // namespace gui
} // namespace spades
