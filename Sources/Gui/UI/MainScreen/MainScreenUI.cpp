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
#include <cmath>
#include <fstream>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "CountryFlags.h"
#include "CreateProfileScreen.h"
#include "LobbyScreen.h"
#include "MainMenu.h"
#include "MainScreenUI.h"
#include "SelectCountryScreen.h"
#include "SkinSelectScreen.h"
#include "ModWeaponSelectScreen.h"
#include <Client/Fonts.h>
#include <Client/PlayerCosmetics.h>
#include <Client/GameMap.h>
#include <Client/IAudioDevice.h>
#include <Client/IImage.h>
#include <Client/IRenderer.h>
#include <Core/Bitmap.h>
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Core/Settings.h>
#include <Core/VoxelModel.h>
#include <Gui/MainScreenHelper.h>
#include <Gui/UI/Components/UpdateNotice.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/Widgets/DrawUtils.h>
#include <Gui/UpdateChecker.h>

SPADES_SETTING(cg_playerName); // defined in Gui/MainScreen.cpp
DEFINE_SPADES_SETTING(cg_playerNameIsSet, "0");
DEFINE_SPADES_SETTING(cg_country, "US");
DEFINE_SPADES_SETTING(cg_countryIsSet, "0");
SPADES_SETTING(cg_playerSkin);
SPADES_SETTING(cg_playerCape);
SPADES_SETTING(cg_playerAccessory);

namespace spades {
	namespace gui {
		using client::ModelRenderParam;

		MainScreenUI::MainScreenUI(client::IRenderer* renderer, client::IAudioDevice* audioDevice,
		                           client::FontManager* fontManager, MainScreenHelper* helper)
		    : renderer(renderer),
		      audioDevice(audioDevice),
		      fontManager(fontManager),
		      helper(helper) {
			SetupRenderer();

			manager = Handle<ui::UIManager>::New(renderer, audioDevice);
			manager->GetRootElement().SetFont(&fontManager->GetGuiFont());

			// The menu bakes the screen extent into its layout, so a resize has to
			// rebuild it rather than just resize the root.
			manager->screenSizeChanged = [this] { Reload(); };

			Init();
			InitLobby3D();

			// Runs once per process; the main screen is rebuilt on every return from
			// a game, and keeps the first answer.
			UpdateChecker::Get().Start();

			// Let the new player choose their IGN
			std::string nameStr = cg_playerName;
			if (nameStr != "" && nameStr != "Deuce")
				cg_playerNameIsSet = 1;

			// prompt window if user has an empty name
			if (nameStr == "")
				cg_playerNameIsSet = 0;

			// First time run: Prompt Country Selection first!
			if (!static_cast<bool>(cg_countryIsSet)) {
				Handle<SelectCountryScreen> sc = Handle<SelectCountryScreen>::New(
				    lobbyScreen.GetPointerOrNull(), fontManager,
				    [this](const std::string& code) {
					    SetCountry(code);
					    // If name is also not set, prompt profile after country selection
					    if (!static_cast<bool>(cg_playerNameIsSet)) {
						    Handle<CreateProfileScreen> al = Handle<CreateProfileScreen>::New(
						        lobbyScreen.GetPointerOrNull(), this->fontManager.GetPointerOrNull());
						    al->Run();
					    }
				    },
				    false /* cannot cancel on first run */
				);
				sc->Run();
			} else if (!static_cast<bool>(cg_playerNameIsSet)) {
				Handle<CreateProfileScreen> al =
				    Handle<CreateProfileScreen>::New(lobbyScreen.GetPointerOrNull(), fontManager);
				al->Run();
			}
		}

		MainScreenUI::~MainScreenUI() {}

		void MainScreenUI::Init() {
			mainMenu = Handle<MainScreenMainMenu>::New(this);
			mainMenu->SetBounds(manager->GetRootElement().GetBounds());
			manager->GetRootElement().AddChild(mainMenu.GetPointerOrNull());

			lobbyScreen = Handle<LobbyScreen>::New(this);
			manager->GetRootElement().AddChild(lobbyScreen.GetPointerOrNull());

			// Show Lobby by default, hide Server Browser
			mainMenu->visible = false;
			lobbyScreen->visible = true;
		}

		void MainScreenUI::Reload() {
			bool wasServerBrowser = (mainMenu && mainMenu->visible);
			MainScreenMainMenuState state;
			if (mainMenu)
				state = mainMenu->GetState();

			// Detach every top-level element
			ui::UIElement& root = manager->GetRootElement();
			while (!root.GetChildren().empty())
				root.RemoveChild(root.GetChildren()[0].GetPointerOrNull());

			Init();
			if (wasServerBrowser) {
				ShowServerBrowser();
				mainMenu->SetState(state);
			} else {
				ShowLobby();
			}
		}

		void MainScreenUI::ShowLobby() {
			if (mainMenu)
				mainMenu->visible = false;
			if (lobbyScreen) {
				lobbyScreen->visible = true;
			}
		}

		void MainScreenUI::ShowServerBrowser() {
			if (lobbyScreen)
				lobbyScreen->visible = false;
			if (mainMenu) {
				mainMenu->visible = true;
				MainScreenMainMenuState state;
				state.activeTabIndex = 0; // Default to servers tab
				mainMenu->SetState(state);
				mainMenu->LoadServerList();
			}
		}

		void MainScreenUI::ShowModsBrowser() {
			if (lobbyScreen)
				lobbyScreen->visible = false;
			if (mainMenu) {
				mainMenu->visible = true;
				MainScreenMainMenuState state;
				state.activeTabIndex = 1; // Mods tab right next to Servidores!
				mainMenu->SetState(state);
				mainMenu->LoadModList();
			}
		}

		void MainScreenUI::SetCountry(const std::string& code) {
			cg_country = code;
			cg_countryIsSet = 1;
			Settings::GetInstance()->Flush();
			RebuildFlagModel();
		}

		void MainScreenUI::OpenDetailedSettings() {
			if (helper)
				helper->OpenStartupScreen();
		}

		void MainScreenUI::InitLobby3D() {
			playerTorsoModel = renderer->RegisterModel("Models/Player/Torso.kv6");
			playerHeadModel = renderer->RegisterModel("Models/Player/Head.kv6");
			playerLegLeftModel = renderer->RegisterModel("Models/Player/Leg.kv6");
			playerLegRightModel = renderer->RegisterModel("Models/Player/LegRight.kv6");
			playerArmsModel = renderer->RegisterModel("Models/Player/Arms.kv6");

			// Create Flagpole VoxelModel
			try {
				int poleHeight = 48;
				Handle<VoxelModel> pole = Handle<VoxelModel>::New(2, 2, poleHeight);
				pole->SetOrigin(MakeVector3(-1.0F, -1.0F, -static_cast<float>(poleHeight) * 0.5F));
				uint32_t woodColor = 0x2E4A6E; // wood tone in voxel color format
				for (int z = 0; z < poleHeight; ++z) {
					pole->SetSolid(0, 0, z, woodColor);
					pole->SetSolid(1, 0, z, woodColor);
					pole->SetSolid(0, 1, z, woodColor);
					pole->SetSolid(1, 1, z, woodColor);
				}
				// Gold finial top
				uint32_t goldColor = 0x25C4E8;
				for (int z = 0; z < 2; ++z) {
					pole->SetSolid(0, 0, z, goldColor);
					pole->SetSolid(1, 0, z, goldColor);
					pole->SetSolid(0, 1, z, goldColor);
					pole->SetSolid(1, 1, z, goldColor);
				}

				flagpoleModel = renderer->CreateModel(*pole);
			} catch (...) {
				flagpoleModel.Set(nullptr, false);
			}

			RebuildFlagModel();
			RebuildCosmeticModels();
		}

		void MainScreenUI::RebuildFlagModel() {
			std::string countryCode = cg_country;
			flagSliceModels.clear();
			flagSliceModels.resize(16);
			loadedFlagCountry = countryCode;

			if (countryCode.empty() || countryCode == "NONE") {
				// Empty / White flag banner
				try {
					for (int col = 0; col < 16; ++col) {
						Handle<VoxelModel> slice = Handle<VoxelModel>::New(1, 1, 16);
						slice->SetOrigin(MakeVector3(0.0F, 0.0F, 0.0F));

						// Flag banner height: rows 3 to 13
						for (int row = 3; row < 14; ++row) {
							uint32_t colVal = 0xEEEEEE;
							slice->SetSolid(0, 0, row, colVal);
						}

						flagSliceModels[col] = renderer->CreateModel(*slice);
					}
				} catch (...) {}
				return;
			}

			int tileX = 0, tileY = 0;
			FlagIconRenderer::GetFlagCoordinates(countryCode, tileX, tileY);

			try {
				Handle<Bitmap> bmp = Bitmap::Load("Gfx/UI/Flags.png");
				if (!bmp)
					return;

				int startPixelX = tileX * 16;
				int startPixelY = tileY * 16;

				for (int col = 0; col < 16; ++col) {
					int solidCount = 0;
					for (int row = 0; row < 16; ++row) {
						uint32_t px = bmp->GetPixel(startPixelX + col, startPixelY + row);
						if ((px >> 24) > 32)
							solidCount++;
					}

					if (solidCount == 0)
						continue;

					Handle<VoxelModel> slice = Handle<VoxelModel>::New(1, 1, 16);
					slice->SetOrigin(MakeVector3(0.0F, 0.0F, 0.0F));

					for (int row = 0; row < 16; ++row) {
						uint32_t px = bmp->GetPixel(startPixelX + col, startPixelY + row);
						if ((px >> 24) > 32) {
							uint8_t r = px & 0xFF;
							uint8_t g = (px >> 8) & 0xFF;
							uint8_t b = (px >> 16) & 0xFF;
							uint32_t colVal = r | (g << 8) | (b << 16);
							slice->SetSolid(0, 0, row, colVal);
						}
					}

					flagSliceModels[col] = renderer->CreateModel(*slice);
				}
			} catch (...) {
				// gracefully ignore if texture loading fails
			}
		}

		void MainScreenUI::RebuildCosmeticModels() {
			std::string capeType = cg_playerCape;
			loadedCapeType = capeType;
			playerCapeModel = client::PlayerCosmetics::GetCapeModel(*renderer, capeType);

			std::string accType = cg_playerAccessory;
			loadedAccessoryType = accType;
			playerAccessoryModel = client::PlayerCosmetics::GetAccessoryModel(*renderer, accType);
		}

		void MainScreenUI::RenderLobbyScene(const client::SceneDefinition& sceneDef, float dt) {
			if (!playerTorsoModel || !playerHeadModel)
				return;

			std::string currentCountry = cg_country;
			if (currentCountry != loadedFlagCountry)
				RebuildFlagModel();

			Vector3 orig = sceneDef.viewOrigin;
			Vector3 dir = sceneDef.viewAxis[2];  // camera forward
			Vector3 side = sceneDef.viewAxis[0]; // camera right
			Vector3 up = sceneDef.viewAxis[1];   // camera up

			// Position character in view: slightly to the right, in front of camera
			float dist = 5.8F;
			float rightOffset = 1.05F;
			float upOffset = -0.55F;

			Vector3 playerPos = orig + dir * dist + side * rightOffset + up * upOffset;

			// Dynamic light on character
			{
				client::DynamicLightParam light;
				light.type = client::DynamicLightTypePoint;
				light.origin = playerPos + up * 1.5F - dir * 1.5F - side * 0.5F;
				light.radius = 8.0F;
				light.color = MakeVector3(1.0F, 0.96F, 0.9F);
				renderer->AddLight(light);
			}

			float breath = sinf(time * 2.2F);
			float pitch = 0.05F + breath * 0.02F;
			float armPitch = -0.55F + breath * 0.02F;

			// Turn character slightly 3/4 towards camera
			Matrix4 lower = Matrix4::Translate(playerPos)
			    * Matrix4::Rotate(MakeVector3(0, 0, 1), -0.35F);

			Matrix4 scaler = Matrix4::Scale(0.1F) * Matrix4::Scale(-1, -1, 1);

			std::string currentCape = cg_playerCape;
			if (currentCape != loadedCapeType)
				RebuildCosmeticModels();
			std::string currentAcc = cg_playerAccessory;
			if (currentAcc != loadedAccessoryType)
				RebuildCosmeticModels();

			ModelRenderParam param;
			std::string skin = cg_playerSkin;
			Vector3 skinCol(0.18F, 0.45F, 0.85F); // Blue team uniform (default)
			skinCol = client::PlayerCosmetics::GetSkinColor(skin, skinCol);
			param.customColor = skinCol;

			float const legsPosX = 0.25F;
			float const legsPosY = 0.0F;
			float const legsPosZ = 0.1F;
			float const headPosZ = 0.0F;
			float const torsoPosZ = 1.0F;
			float const armsPosZ = 0.1F;

			// Legs
			if (playerLegLeftModel) {
				param.matrix = lower * Matrix4::Translate(-legsPosX, legsPosY, -legsPosZ)
				    * Matrix4::Rotate(MakeVector3(0, 1, 0), -0.05F) * scaler;
				renderer->RenderModel(*playerLegLeftModel, param);
			}
			if (playerLegRightModel) {
				param.matrix = lower * Matrix4::Translate(legsPosX, legsPosY, -legsPosZ)
				    * Matrix4::Rotate(MakeVector3(0, 1, 0), 0.05F) * scaler;
				renderer->RenderModel(*playerLegRightModel, param);
			}

			// Torso with idle breathing
			Matrix4 torso = lower * Matrix4::Translate(0.0F, 0.0F, -torsoPosZ + breath * 0.015F)
			    * Matrix4::Rotate(MakeVector3(1, 0, 0), breath * 0.015F);

			param.matrix = torso * scaler;
			renderer->RenderModel(*playerTorsoModel, param);

			// Cape on back of Torso
			if (playerCapeModel) {
				float capeSwing = 0.25F + breath * 0.04F + sinf(time * 3.5F) * 0.08F;
				Matrix4 capeMat = torso * Matrix4::Translate(0.0F, 0.36F, -0.25F)
				    * Matrix4::Rotate(MakeVector3(1, 0, 0), capeSwing)
				    * Matrix4::Scale(0.095F) * Matrix4::Scale(-1, -1, 1);
				ModelRenderParam capeParam;
				capeParam.matrix = capeMat;
				renderer->RenderModel(*playerCapeModel, capeParam);
			}

			// Head
			Matrix4 head = torso * Matrix4::Translate(0.0F, 0.0F, -headPosZ)
			    * Matrix4::Rotate(MakeVector3(1, 0, 0), pitch)
			    * Matrix4::Rotate(MakeVector3(0, 0, 1), 0.15F);
			param.matrix = head * scaler;
			renderer->RenderModel(*playerHeadModel, param);

			// Accessory (Crown / Helmet) on Head
			if (playerAccessoryModel) {
				ModelRenderParam accParam;
				accParam.matrix = head * Matrix4::Translate(0.0F, 0.0F, -0.65F)
				    * Matrix4::Scale(0.11F) * Matrix4::Scale(-1, -1, 1);
				renderer->RenderModel(*playerAccessoryModel, accParam);
			}

			// Arms
			Matrix4 arms = torso * Matrix4::Translate(0.0F, 0.0F, armsPosZ)
			    * Matrix4::Rotate(MakeVector3(1, 0, 0), armPitch);

			if (playerArmsModel) {
				param.matrix = arms * scaler;
				renderer->RenderModel(*playerArmsModel, param);
			}

			// Flagpole in right hand
			Matrix4 poleMatrix = arms * Matrix4::Translate(0.38F, -0.65F, -0.1F)
			    * Matrix4::Rotate(MakeVector3(1, 0, 0), 0.30F)
			    * Matrix4::Rotate(MakeVector3(0, 1, 0), -0.20F);

			if (flagpoleModel) {
				ModelRenderParam poleParam;
				poleParam.matrix = poleMatrix * Matrix4::Scale(0.065F) * Matrix4::Scale(-1, -1, 1);
				renderer->RenderModel(*flagpoleModel, poleParam);
			}

			// Flag banner waving
			Matrix4 flagRoot = poleMatrix * Matrix4::Translate(0.02F, -0.05F, -1.05F)
			    * Matrix4::Rotate(MakeVector3(0, 0, 1), 0.1F);

			Matrix4 sliceScaler = Matrix4::Scale(0.038F) * Matrix4::Scale(-1, -1, 1);

			for (int col = 0; col < 16 && col < static_cast<int>(flagSliceModels.size()); ++col) {
				if (!flagSliceModels[col])
					continue;

				float wavePhase = time * 7.5F - static_cast<float>(col) * 0.45F;
				float amp = (static_cast<float>(col) / 15.0F) * 0.10F;
				float waveX = sinf(wavePhase) * amp;
				float waveZ = cosf(wavePhase) * amp * 0.35F;

				Vector3 sliceOffset(static_cast<float>(col) * 0.038F, waveX, waveZ);
				Matrix4 sliceMat = flagRoot * Matrix4::Translate(sliceOffset) * sliceScaler;

				ModelRenderParam sliceParam;
				sliceParam.matrix = sliceMat;
				renderer->RenderModel(*flagSliceModels[col], sliceParam);
			}
		}

		void MainScreenUI::SetupRenderer() {
			// load map
			std::unique_ptr<IStream> stream{FileManager::OpenForReading("Maps/Title.vxl")};
			if (stream) {
				titleMap.Set(client::GameMap::Load(stream.get()), false);
				renderer->SetGameMap(*titleMap);
				renderer->SetFogColor(MakeVector3(0.1F, 0.1F, 0.1F));
				renderer->SetFogDistance(128.0F);
			}
			time = -1.0F;

			// returned from the client game, so reload the server list.
			if (mainMenu)
				mainMenu->LoadServerList();

			if (manager)
				manager->KeyPanic();
		}

		void MainScreenUI::LoadBackgroundMap(const std::string& mapPath) {
			try {
				std::unique_ptr<IStream> stream{FileManager::OpenForReading(mapPath.c_str())};
				if (stream) {
					titleMap.Set(client::GameMap::Load(stream.get()), false);
					renderer->SetGameMap(*titleMap);
					if (mapPath.find("LAbeach") != std::string::npos) {
						renderer->SetFogColor(MakeVector3(0.55F, 0.75F, 0.95F)); // tropical sky blue
						renderer->SetFogDistance(240.0F);
					} else {
						renderer->SetFogColor(MakeVector3(0.1F, 0.1F, 0.1F));
						renderer->SetFogDistance(140.0F);
					}
					time = -1.0F;
				}
			} catch (...) {}
		}

		void MainScreenUI::DropFileEvent(const std::string& filePath) {
			std::string lowerPath = filePath;
			std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
			if (lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size() - 4) == ".png") {
#ifdef _WIN32
				_mkdir("Skins");
				_mkdir("Resources\\Skins");
#else
				mkdir("Skins", 0755);
				mkdir("Resources/Skins", 0755);
#endif
				std::ifstream src(filePath, std::ios::binary);
				if (src.is_open()) {
					std::vector<char> buffer((std::istreambuf_iterator<char>(src)),
					                         std::istreambuf_iterator<char>());
					src.close();
					if (!buffer.empty()) {
						std::ofstream dst1("Skins/custom.png", std::ios::binary);
						if (dst1.is_open()) {
							dst1.write(buffer.data(), buffer.size());
							dst1.close();
						}
						std::ofstream dst2("Resources/Skins/custom.png", std::ios::binary);
						if (dst2.is_open()) {
							dst2.write(buffer.data(), buffer.size());
							dst2.close();
						}

						cg_playerSkin = "custom";
						Settings::GetInstance()->Flush();

						client::PlayerCosmetics::ReloadCustomSkin();
						UpdateLobbyCosmetics();

						if (manager) {
							try {
								manager->PlaySound("Sounds/Feedback/Limbo/Select.opus");
							} catch (...) {}
						}

						if (activeSkinScreen) {
							activeSkinScreen->SelectCustomSkin();
						}
					}
				}
			}
		}

		void MainScreenUI::MouseEvent(float x, float y) { manager->MouseEvent(x, y); }
		void MainScreenUI::WheelEvent(float x, float y) { manager->WheelEvent(x, y); }
		void MainScreenUI::KeyEvent(const std::string& key, bool down) {
			manager->KeyEvent(key, down);
		}
		void MainScreenUI::TextInputEvent(const std::string& text) {
			manager->TextInputEvent(text);
		}
		void MainScreenUI::TextEditingEvent(const std::string& text, int start, int len) {
			manager->TextEditingEvent(text, start, len);
		}

		bool MainScreenUI::AcceptsTextInput() { return manager->AcceptsTextInput(); }
		AABB2 MainScreenUI::GetTextInputRect() { return manager->GetTextInputRect(); }

		client::SceneDefinition MainScreenUI::SetupCamera(client::SceneDefinition sceneDef,
		                                                  Vector3 eye, Vector3 at, Vector3 up,
		                                                  float fov, float ratio) {
			Vector3 dir = (at - eye).Normalize();
			Vector3 side = Vector3::Cross(dir, up).Normalize();
			up = -Vector3::Cross(dir, side);
			sceneDef.viewOrigin = eye;
			sceneDef.viewAxis[0] = side;
			sceneDef.viewAxis[1] = up;
			sceneDef.viewAxis[2] = dir;
			sceneDef.fovY = DEG2RAD(fov);
			sceneDef.fovX = 2.0F * std::atan(std::tan(sceneDef.fovY * 0.5F) * ratio);
			return sceneDef;
		}

		void MainScreenUI::RenderMods3DWeapon(const client::SceneDefinition& sceneDef, float dt) {
			if (!rifleModel) rifleModel = renderer->RegisterModel("Models/Weapons/Rifle/Weapon.kv6");
			if (!smgModel) smgModel = renderer->RegisterModel("Models/Weapons/SMG/Weapon.kv6");
			if (!shotgunModel) shotgunModel = renderer->RegisterModel("Models/Weapons/Shotgun/Weapon.kv6");
			if (!spadeModel) spadeModel = renderer->RegisterModel("Models/Weapons/Spade/Spade.kv6");

			modsWeaponYaw += dt * 1.5F;
			modsShowcaseTime += dt;

			std::string cat = mainMenu ? mainMenu->GetSelectedModCategory() : "SEMI";
			Handle<client::IModel> curModel = rifleModel;
			Vector3 lightCol = MakeVector3(0.65F, 0.85F, 1.0F);
			float scale = 0.024F;

			if (cat == "SMG") {
				curModel = smgModel;
				lightCol = MakeVector3(0.40F, 0.95F, 0.65F);
				scale = 0.026F;
			} else if (cat == "SHOTGUN") {
				curModel = shotgunModel;
				lightCol = MakeVector3(1.0F, 0.70F, 0.35F);
				scale = 0.025F;
			} else if (cat == "SPADE") {
				curModel = spadeModel;
				lightCol = MakeVector3(0.95F, 0.85F, 0.45F);
				scale = 0.025F;
			}

			if (!curModel)
				return;

			float sw = renderer->ScreenWidth();
			float sh = renderer->ScreenHeight();

			float tanHalfFovX = (sceneDef.fovX > 0.01F) ? tanf(sceneDef.fovX * 0.5F) : (tanf(DEG2RAD(35.0F)) * (sw / sh));
			float tanHalfFovY = (sceneDef.fovY > 0.01F) ? tanf(sceneDef.fovY * 0.5F) : tanf(DEG2RAD(35.0F));

			Vector3 orig = sceneDef.viewOrigin;
			Vector3 dir = sceneDef.viewAxis[2];
			Vector3 side = sceneDef.viewAxis[0];
			Vector3 up = sceneDef.viewAxis[1];

			// Position rotating weapon in view
			float ndcX = (sw >= 1100.0F) ? 0.72F : 0.62F;
			float ndcY = (sw >= 1100.0F) ? 0.05F : 0.35F;
			float dist = 3.8F;

			float rightOffset = ndcX * tanHalfFovX * dist;
			float upOffset = ndcY * tanHalfFovY * dist;
			Vector3 weaponPos = orig + dir * dist + side * rightOffset + up * upOffset;

			float bob = sinf(modsShowcaseTime * 2.2F) * 0.02F;
			Vector3 actualPos = weaponPos + up * bob;

			client::DynamicLightParam light;
			light.type = client::DynamicLightTypePoint;
			light.origin = actualPos + up * 0.8F - dir * 0.9F + side * 0.2F;
			light.radius = 5.5F;
			light.color = lightCol;
			renderer->AddLight(light);

			client::ModelRenderParam param;
			param.depthHack = true;

			Vector3 rotFwd = (side * cosf(modsWeaponYaw) - dir * sinf(modsWeaponYaw)).Normalize();
			Vector3 rotRight = (side * sinf(modsWeaponYaw) + dir * cosf(modsWeaponYaw)).Normalize();

			Vector3 weapBarrel = (rotFwd + up * 0.14F).Normalize();
			Vector3 weapDown = (-up + rotFwd * 0.14F).Normalize();
			Vector3 weapRight = rotRight;

			Matrix4 weapMatrix = Matrix4::FromAxis(weapRight, weapBarrel, weapDown, actualPos);
			param.matrix = weapMatrix * Matrix4::Scale(scale);

			renderer->RenderModel(*curModel, param);
		}

		void MainScreenUI::RunFrame(float dt) {
			if (time < 0.0F)
				time = 0.0F;

			float sw = renderer->ScreenWidth();
			float sh = renderer->ScreenHeight();

			client::SceneDefinition sceneDef;
			float cameraX = time;
			cameraX -= std::floor(cameraX / 512.0F) * 512.0F;
			cameraX = 512.0F - cameraX;

			Vector3 orig = MakeVector3(cameraX, 256.0F, 12.0F);
			Vector3 aimAt = MakeVector3(cameraX + 0.1F, 257.0F, 12.5F);
			Vector3 up = MakeVector3(0.0F, 0.0F, -1.0F);

			sceneDef = SetupCamera(sceneDef, orig, aimAt, up, 30.0F, sw / sh);
			sceneDef.zNear = 0.1F;
			sceneDef.zFar = 160.0F;
			sceneDef.time = static_cast<unsigned int>(time * 1000.0F);
			sceneDef.viewportWidth = static_cast<int>(sw);
			sceneDef.viewportHeight = static_cast<int>(sh);
			sceneDef.denyCameraBlur = true;
			sceneDef.depthOfFieldFocalLength = 100.0F;
			sceneDef.skipWorld = false;

			// fade the map
			float fade = Clamp((time - 1.0F) / 2.2F, 0.0F, 1.0F);
			sceneDef.globalBlur = Clamp((1.0F - (time - 1.0F) / 2.5F), 0.0F, 1.0F);
			if (!mainMenu->IsEnabled())
				sceneDef.globalBlur = std::max(sceneDef.globalBlur, 0.5F);

			renderer->StartScene(sceneDef);
			if (lobbyScreen && lobbyScreen->visible && !activeWeaponScreen) {
				RenderLobbyScene(sceneDef, dt);
			}
			if (mainMenu && mainMenu->visible && mainMenu->IsModsTabActive()) {
				RenderMods3DWeapon(sceneDef, dt);
			}
			if (activeWeaponScreen && activeWeaponScreen->visible) {
				activeWeaponScreen->Render3DWeapon(sceneDef, dt);
			}
			renderer->EndScene();

			// fade the map (draw)
			if (fade < 1.0F) {
				ui::SetColorNP(*renderer, MakeVector4(0.0F, 0.0F, 0.0F, 1.0F - fade));
				renderer->DrawImage(nullptr, AABB2(0.0F, 0.0F, sw, sh));
			}

			// draw title logo
			Handle<client::IImage> img = renderer->RegisterImage("Gfx/Title/Logo.png");
			ui::SetColorNP(*renderer, MakeVector4(1.0F, 1.0F, 1.0F, 1.0F));
			renderer->DrawImage(img, MakeVector2((sw - img->GetWidth()) * 0.5F, 64.0F));

			// The check answers asynchronously; announce a new release once, as soon
			// as the menu is free of other dialogs.
			UpdatePromptScreen::ShowIfPending(mainMenu.GetPointerOrNull(),
			                                  [this] { shouldExit = true; });

			manager->RunFrame(dt);
			manager->Render();

			time += std::min(dt, 0.05F);
		}

		void MainScreenUI::Closing() {
			Settings::GetInstance()->Flush();
			shouldExit = true;
		}

		bool MainScreenUI::WantsToBeClosed() { return shouldExit; }
	} // namespace gui
} // namespace spades
