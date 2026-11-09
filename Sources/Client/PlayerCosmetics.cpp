/*
 Copyright (c) 2026 Fran6nd, ySpades developers.

 This file is part of ySpades.

 ySpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 ySpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with ySpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "PlayerCosmetics.h"

#include <algorithm>
#include <map>
#include <Client/IModel.h>
#include <Client/IRenderer.h>
#include <Core/Bitmap.h>
#include <Core/FileManager.h>
#include <Core/VoxelModel.h>

namespace spades {
	namespace client {
		namespace {
			static std::map<std::string, Handle<IModel>> g_capeCache;
			static std::map<std::string, Handle<IModel>> g_accCache;
			static std::map<int, Handle<IModel>> g_simpleToolCache;
			static Handle<IModel> g_heliFuselageModel;
			static Handle<IModel> g_heliRotorModel;
			static Vector3 g_customSkinColor(0.20F, 0.50F, 0.85F);
			static bool g_customSkinLoaded = false;
		} // namespace

		Vector3 PlayerCosmetics::GetCustomSkinColor() {
			if (!g_customSkinLoaded)
				ReloadCustomSkin();
			return g_customSkinColor;
		}

		void PlayerCosmetics::ReloadCustomSkin() {
			g_customSkinLoaded = true;
			try {
				std::string path = "Skins/custom.png";
				if (!FileManager::FileExists(path.c_str()))
					path = "Resources/Skins/custom.png";
				if (FileManager::FileExists(path.c_str())) {
					Handle<Bitmap> bmp = Bitmap::Load(path);
					if (bmp && bmp->GetWidth() >= 16 && bmp->GetHeight() >= 16) {
						// Sample front torso shirt center around (24, 24)
						int sampleX = std::min(24, bmp->GetWidth() - 1);
						int sampleY = std::min(24, bmp->GetHeight() - 1);
						uint32_t px = bmp->GetPixel(sampleX, sampleY);
						if ((px >> 24) > 32) {
							float r = static_cast<float>(px & 0xFF) / 255.0F;
							float g = static_cast<float>((px >> 8) & 0xFF) / 255.0F;
							float b = static_cast<float>((px >> 16) & 0xFF) / 255.0F;
							g_customSkinColor = MakeVector3(r, g, b);
							return;
						}
					}
				}
			} catch (...) {}
			g_customSkinColor = MakeVector3(0.25F, 0.65F, 0.95F);
		}

		Vector3 PlayerCosmetics::GetSkinColor(const std::string& skinId, const Vector3& defaultColor) {
			if (skinId == "default" || skinId == "off" || skinId.empty())
				return defaultColor;
			else if (skinId == "blue")
				return MakeVector3(0.18F, 0.45F, 0.85F); // Classic Blue
			else if (skinId == "gold")
				return MakeVector3(1.0F, 0.82F, 0.15F); // VIP Gold
			else if (skinId == "green")
				return MakeVector3(0.22F, 0.45F, 0.25F); // Military Green
			else if (skinId == "red")
				return MakeVector3(0.85F, 0.20F, 0.20F); // Assault Red
			else if (skinId == "black")
				return MakeVector3(0.15F, 0.15F, 0.18F); // Tactical Shadow
			else if (skinId == "white")
				return MakeVector3(0.92F, 0.94F, 0.96F); // Arctic Snow
			else if (skinId == "custom")
				return GetCustomSkinColor();
			return defaultColor;
		}

		Handle<IModel> PlayerCosmetics::GetCapeModel(IRenderer& renderer, const std::string& capeId) {
			if (capeId.empty() || capeId == "none")
				return Handle<IModel>();

			auto it = g_capeCache.find(capeId);
			if (it != g_capeCache.end() && it->second)
				return it->second;

			try {
				int const cw = 10;
				int const ch = 16;
				int const cd = 1;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(cw, cd, ch);
				vm->SetOrigin(MakeVector3(-static_cast<float>(cw) * 0.5F, 0.0F, 0.0F));

				// Colors stored as little-endian 0xBBGGRR
				uint32_t mainCol = 0x884422;   // Classic Blue
				uint32_t borderCol = 0xEEEEEE; // Silver / White trim
				if (capeId == "gold") {
					mainCol = 0x1AE0FF;   // Vibrant Gold
					borderCol = 0x05B0E0; // Deep Golden Rim
				} else if (capeId == "black") {
					mainCol = 0x181818;   // Deep Charcoal
					borderCol = 0x383838; // Dark Steel
				} else if (capeId == "red") {
					mainCol = 0x2020E0;   // Crimson Red
					borderCol = 0xEEEEEE; // White Border
				}

				for (int x = 0; x < cw; ++x) {
					for (int z = 0; z < ch; ++z) {
						bool isBorder = (x == 0 || x == cw - 1 || z == 0 || z == ch - 1);
						uint32_t c = isBorder ? borderCol : mainCol;
						// Gold cape insignia in center
						if (capeId == "gold" && (x == cw / 2 || x == cw / 2 - 1) && (z >= 4 && z <= 8))
							c = 0xFFFFFF;
						vm->SetSolid(x, 0, z, c);
					}
				}

				Handle<IModel> model = renderer.CreateModel(*vm);
				g_capeCache[capeId] = model;
				return model;
			} catch (...) {
				return Handle<IModel>();
			}
		}

		Handle<IModel> PlayerCosmetics::GetAccessoryModel(IRenderer& renderer, const std::string& accId) {
			if (accId.empty() || accId == "none")
				return Handle<IModel>();

			auto it = g_accCache.find(accId);
			if (it != g_accCache.end() && it->second)
				return it->second;

			try {
				if (accId == "crown") {
					// Authentic Imperial Royal Crown
					// Sculpted with ermine fur base, gold circlet with rubies, emeralds, sapphires,
					// crimson velvet bonnet, golden arches, and apical orb with royal cross.
					int const sz = 12;
					int const h = 10;
					Handle<VoxelModel> vm = Handle<VoxelModel>::New(sz, sz, h);
					vm->SetOrigin(MakeVector3(-static_cast<float>(sz) * 0.5F, -static_cast<float>(sz) * 0.5F, -9.0F));

					// Little-endian RGB: 0xBBGGRR
					const uint32_t goldBright  = 0x1AE0FF; // Pure gold
					const uint32_t goldDeep    = 0x0DA0D0; // Shaded gold
					const uint32_t ermineWhite = 0xF5F5F5; // Ermine fur white
					const uint32_t ermineSpot  = 0x151515; // Ermine black tail dot
					const uint32_t ruby        = 0x1818EE; // Royal ruby (crimson red)
					const uint32_t sapphire    = 0xEE5018; // Royal sapphire (rich blue)
					const uint32_t emerald     = 0x20D820; // Royal emerald (vibrant green)
					const uint32_t diamond     = 0xFFFFFF; // Diamond / pearl
					const uint32_t amethyst    = 0x902090; // Royal amethyst
					const uint32_t velvetRed   = 0x241484; // Crimson velvet bonnet

					// Layer z = 9: Base ermine fur trim
					for (int x = 0; x < sz; ++x) {
						for (int y = 0; y < sz; ++y) {
							if (x == 0 || x == sz - 1 || y == 0 || y == sz - 1) {
								bool isSpot = ((x % 3 == 0 && y % 3 == 0) || x == 5 || x == 6 || y == 5 || y == 6);
								vm->SetSolid(x, y, 9, isSpot ? ermineSpot : ermineWhite);
							}
						}
					}

					// Layer z = 8: Golden circlet band with royal jewels
					for (int x = 0; x < sz; ++x) {
						for (int y = 0; y < sz; ++y) {
							if (x == 0 || x == sz - 1 || y == 0 || y == sz - 1) {
								uint32_t c = goldBright;
								if ((x == 0 && y == 0) || (x == sz - 1 && y == 0) ||
								    (x == 0 && y == sz - 1) || (x == sz - 1 && y == sz - 1)) {
									c = diamond;
								} else if ((x == 5 || x == 6) && y == 0) {
									c = ruby;
								} else if ((x == 5 || x == 6) && y == sz - 1) {
									c = sapphire;
								} else if ((y == 5 || y == 6) && x == 0) {
									c = emerald;
								} else if ((y == 5 || y == 6) && x == sz - 1) {
									c = ruby;
								} else if ((x == 2 || x == 9) && (y == 0 || y == sz - 1)) {
									c = amethyst;
								}
								vm->SetSolid(x, y, 8, c);
							}
						}
					}

					// Layer z = 7: Upper golden rim & interior velvet lining
					for (int x = 0; x < sz; ++x) {
						for (int y = 0; y < sz; ++y) {
							if (x == 0 || x == sz - 1 || y == 0 || y == sz - 1) {
								vm->SetSolid(x, y, 7, goldDeep);
							} else if (x >= 2 && x <= sz - 3 && y >= 2 && y <= sz - 3) {
								vm->SetSolid(x, y, 7, velvetRed);
							}
						}
					}

					// Layer z = 6: Fleur-de-lis peaks & velvet bonnet dome
					for (int x = 0; x < sz; ++x) {
						for (int y = 0; y < sz; ++y) {
							bool isCornerPeak = (x <= 1 && y <= 1) || (x >= sz - 2 && y <= 1) ||
							                    (x <= 1 && y >= sz - 2) || (x >= sz - 2 && y >= sz - 2);
							bool isMidPeak = ((x == 5 || x == 6) && (y == 0 || y == sz - 1)) ||
							                 ((y == 5 || y == 6) && (x == 0 || x == sz - 1));
							if (isCornerPeak || isMidPeak) {
								vm->SetSolid(x, y, 6, goldBright);
							} else if (x >= 2 && x <= sz - 3 && y >= 2 && y <= sz - 3) {
								vm->SetSolid(x, y, 6, velvetRed);
							}
						}
					}

					// Layer z = 5: Fleuron jewels, rising arches, velvet dome
					// Peaks finials
					vm->SetSolid(5, 0, 5, ruby);
					vm->SetSolid(6, 0, 5, ruby);
					vm->SetSolid(5, sz - 1, 5, sapphire);
					vm->SetSolid(6, sz - 1, 5, sapphire);
					vm->SetSolid(0, 5, 5, emerald);
					vm->SetSolid(0, 6, 5, emerald);
					vm->SetSolid(sz - 1, 5, 5, ruby);
					vm->SetSolid(sz - 1, 6, 5, ruby);
					// Arches curving inward
					vm->SetSolid(5, 1, 5, goldBright);
					vm->SetSolid(6, 1, 5, goldBright);
					vm->SetSolid(5, sz - 2, 5, goldBright);
					vm->SetSolid(6, sz - 2, 5, goldBright);
					vm->SetSolid(1, 5, 5, goldBright);
					vm->SetSolid(1, 6, 5, goldBright);
					vm->SetSolid(sz - 2, 5, 5, goldBright);
					vm->SetSolid(sz - 2, 6, 5, goldBright);
					// Velvet bonnet dome
					for (int x = 3; x <= sz - 4; ++x) {
						for (int y = 3; y <= sz - 4; ++y) {
							vm->SetSolid(x, y, 5, velvetRed);
						}
					}

					// Layer z = 4: Arches converging with pearls & bonnet apex
					vm->SetSolid(5, 2, 4, diamond);
					vm->SetSolid(6, 2, 4, goldBright);
					vm->SetSolid(5, sz - 3, 4, goldBright);
					vm->SetSolid(6, sz - 3, 4, diamond);
					vm->SetSolid(2, 5, 4, goldBright);
					vm->SetSolid(2, 6, 4, diamond);
					vm->SetSolid(sz - 3, 5, 4, diamond);
					vm->SetSolid(sz - 3, 6, 4, goldBright);

					vm->SetSolid(5, 3, 4, goldBright);
					vm->SetSolid(6, 3, 4, goldBright);
					vm->SetSolid(5, sz - 4, 4, goldBright);
					vm->SetSolid(6, sz - 4, 4, goldBright);
					vm->SetSolid(3, 5, 4, goldBright);
					vm->SetSolid(3, 6, 4, goldBright);
					vm->SetSolid(sz - 4, 5, 4, goldBright);
					vm->SetSolid(sz - 4, 6, 4, goldBright);

					// Velvet apex
					for (int x = 4; x <= sz - 5; ++x) {
						for (int y = 4; y <= sz - 5; ++y) {
							vm->SetSolid(x, y, 4, velvetRed);
						}
					}

					// Layer z = 3: Arches intersecting at summit
					for (int x = 4; x <= 7; ++x) {
						for (int y = 4; y <= 7; ++y) {
							bool isPearl = (x == 5 && y == 4) || (x == 6 && y == 7) ||
							               (x == 4 && y == 5) || (x == 7 && y == 6);
							vm->SetSolid(x, y, 3, isPearl ? diamond : goldBright);
						}
					}

					// Layer z = 2: Golden Monde (Orb / Globe)
					vm->SetSolid(5, 5, 2, goldBright);
					vm->SetSolid(6, 5, 2, emerald);
					vm->SetSolid(5, 6, 2, emerald);
					vm->SetSolid(6, 6, 2, goldBright);

					// Layer z = 1: Royal Cross Pattée horizontal crossbar
					for (int x = 4; x <= 7; ++x) {
						for (int y = 5; y <= 6; ++y) {
							bool isCenter = (x == 5 || x == 6);
							vm->SetSolid(x, y, 1, isCenter ? diamond : goldBright);
						}
					}

					// Layer z = 0: Royal Cross Pattée vertical apex
					vm->SetSolid(5, 5, 0, goldBright);
					vm->SetSolid(6, 5, 0, diamond);
					vm->SetSolid(5, 6, 0, diamond);
					vm->SetSolid(6, 6, 0, goldBright);

					Handle<IModel> model = renderer.CreateModel(*vm);
					g_accCache[accId] = model;
					return model;
				} else if (accId == "helmet") {
					// Tactical Military Combat Helmet
					int const sz = 12;
					int const h = 8;
					Handle<VoxelModel> vm = Handle<VoxelModel>::New(sz, sz, h);
					vm->SetOrigin(MakeVector3(-static_cast<float>(sz) * 0.5F, -static_cast<float>(sz) * 0.5F, -7.0F));

					uint32_t camoDark  = 0x1E3520; // Deep olive
					uint32_t camoMid   = 0x284828; // Foliage green
					uint32_t camoBrown = 0x1E2B38; // Earth brown
					uint32_t camoTrim  = 0x121A12; // Black Kevlar rim

					// Layer z = 7: Rim & straps
					for (int x = 0; x < sz; ++x) {
						for (int y = 0; y < sz; ++y) {
							if (x == 0 || x == sz - 1 || y == 0 || y == sz - 1) {
								vm->SetSolid(x, y, 7, camoTrim);
							}
						}
					}

					// Layers z = 3 to 6: Outer curved dome
					for (int z = 3; z <= 6; ++z) {
						int inset = (z <= 4) ? 2 : 1;
						for (int x = inset; x < sz - inset; ++x) {
							for (int y = inset; y < sz - inset; ++y) {
								bool isEdge = (x == inset || x == sz - 1 - inset ||
								               y == inset || y == sz - 1 - inset);
								if (isEdge || z <= 4) {
									uint32_t col = camoDark;
									if ((x + y + z) % 3 == 0)
										col = camoMid;
									else if ((x * 2 + y) % 5 == 0)
										col = camoBrown;
									vm->SetSolid(x, y, z, col);
								}
							}
						}
					}

					// Layer z = 2: Top plate
					for (int x = 4; x <= sz - 5; ++x) {
						for (int y = 4; y <= sz - 5; ++y) {
							vm->SetSolid(x, y, 2, camoDark);
						}
					}

					Handle<IModel> model = renderer.CreateModel(*vm);
					g_accCache[accId] = model;
					return model;
				}
				return Handle<IModel>();
			} catch (...) {
				return Handle<IModel>();
			}
		}

		Handle<IModel> PlayerCosmetics::GetSimpleToolModel(IRenderer& renderer, int toolType, int weaponType) {
			int key = toolType * 10 + weaponType;
			auto it = g_simpleToolCache.find(key);
			if (it != g_simpleToolCache.end() && it->second)
				return it->second;

			try {
				Handle<VoxelModel> vm;
				// Colors stored as little-endian 0xBBGGRR
				const uint32_t darkSteel = 0x222222;
				const uint32_t steel = 0x444444;
				const uint32_t woodBrown = 0x1E3B5C;
				const uint32_t brightMetal = 0x888888;
				const uint32_t oliveDrab = 0x204525;

				if (toolType == 0) { // Player::ToolSpade
					int w = 3, d = 16, h = 3;
					vm = Handle<VoxelModel>::New(w, d, h);
					vm->SetOrigin(MakeVector3(-1.0F, -2.0F, -1.0F));
					// Handle / shaft
					for (int y = 0; y < 11; ++y)
						vm->SetSolid(1, y, 1, woodBrown);
					// Spade blade
					for (int y = 11; y < 16; ++y) {
						for (int x = 0; x < 3; ++x) {
							vm->SetSolid(x, y, 1, brightMetal);
						}
					}
				} else if (toolType == 1) { // Player::ToolBlock
					int s = 5;
					vm = Handle<VoxelModel>::New(s, s, s);
					vm->SetOrigin(MakeVector3(-2.0F, -2.0F, -2.0F));
					for (int x = 0; x < s; ++x)
						for (int y = 0; y < s; ++y)
							for (int z = 0; z < s; ++z)
								vm->SetSolid(x, y, z, 0x757575);
				} else if (toolType == 2) { // Player::ToolWeapon
					if (weaponType == 0) { // RIFLE_WEAPON
						int w = 2, d = 18, h = 4;
						vm = Handle<VoxelModel>::New(w, d, h);
						vm->SetOrigin(MakeVector3(-1.0F, -5.0F, -2.0F));
						// Stock
						for (int y = 0; y < 6; ++y)
							for (int z = 0; z < 3; ++z)
								vm->SetSolid(0, y, z, woodBrown);
						// Receiver
						for (int y = 6; y < 12; ++y)
							for (int z = 1; z < 3; ++z)
								vm->SetSolid(0, y, z, darkSteel);
						// Barrel
						for (int y = 12; y < 18; ++y)
							vm->SetSolid(0, y, 2, steel);
					} else if (weaponType == 1) { // SMG_WEAPON
						int w = 2, d = 13, h = 5;
						vm = Handle<VoxelModel>::New(w, d, h);
						vm->SetOrigin(MakeVector3(-1.0F, -4.0F, -2.0F));
						// Body
						for (int y = 1; y < 10; ++y)
							for (int z = 2; z < 4; ++z)
								vm->SetSolid(0, y, z, darkSteel);
						// Magazine (extending down)
						for (int z = 0; z < 2; ++z)
							vm->SetSolid(0, 5, z, 0x151515);
						// Short barrel
						for (int y = 10; y < 13; ++y)
							vm->SetSolid(0, y, 3, steel);
					} else { // SHOTGUN_WEAPON
						int w = 3, d = 16, h = 4;
						vm = Handle<VoxelModel>::New(w, d, h);
						vm->SetOrigin(MakeVector3(-1.5F, -5.0F, -2.0F));
						// Heavy stock
						for (int y = 0; y < 6; ++y)
							for (int z = 0; z < 3; ++z)
								vm->SetSolid(1, y, z, woodBrown);
						// Forend pump
						for (int y = 8; y < 12; ++y)
							vm->SetSolid(1, y, 1, woodBrown);
						// Dual barrel
						for (int y = 6; y < 16; ++y) {
							vm->SetSolid(0, y, 2, steel);
							vm->SetSolid(1, y, 2, steel);
						}
					}
				} else if (toolType == 3) { // Player::ToolGrenade
					int w = 4, d = 4, h = 5;
					vm = Handle<VoxelModel>::New(w, d, h);
					vm->SetOrigin(MakeVector3(-2.0F, -2.0F, -2.0F));
					// Body
					for (int x = 0; x < 4; ++x)
						for (int y = 0; y < 4; ++y)
							for (int z = 0; z < 4; ++z)
								vm->SetSolid(x, y, z, oliveDrab);
					// Top pin
					vm->SetSolid(1, 1, 4, brightMetal);
					vm->SetSolid(2, 1, 4, brightMetal);
				}

				if (vm) {
					Handle<IModel> model = renderer.CreateModel(*vm);
					g_simpleToolCache[key] = model;
					return model;
				}
			} catch (...) {}

			return Handle<IModel>();
		}

		static HelicopterState g_worldHeli;
		HelicopterState& PlayerCosmetics::GetGlobalHelicopter() {
			return g_worldHeli;
		}

		static ClassicCarState g_worldCar;
		ClassicCarState& PlayerCosmetics::GetGlobalCar() {
			return g_worldCar;
		}

		static Handle<IModel> g_carBodyModels[6];
		static Handle<IModel> g_carWheelModels[6];

		static AirBattleState g_airBattle;
		static Handle<IModel> g_warbirdFuselageAllied;
		static Handle<IModel> g_warbirdFuselageAxis;
		static Handle<IModel> g_warbirdPropModel;

		AirBattleState& PlayerCosmetics::GetGlobalAirBattle() {
			return g_airBattle;
		}

		Handle<IModel> PlayerCosmetics::GetHelicopterFuselageModel(IRenderer& renderer) {
			if (g_heliFuselageModel)
				return g_heliFuselageModel;

			try {
				int const w = 38;
				int const l = 88;
				int const h = 32;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				// Center at: X=19 (fuselage center), Y=34 (rotor mast center), Z=0 (landing skids base)
				vm->SetOrigin(MakeVector3(-19.0F, -34.0F, 0.0F));

				const int cx = w / 2; // 19

				// Little-endian hex RGB: 0xBBGGRR
				const uint32_t colHullBase    = 0x24442A; // Camouflage olive drab
				const uint32_t colHullDark    = 0x1A301E; // Shadowed underbelly olive
				const uint32_t colCamoEarth   = 0x263C52; // Camo earth brown
				const uint32_t colArmorSteel  = 0x1A1B1D; // Tactical armor black/steel
				const uint32_t colCockpitGlass= 0xF0D280; // Crystal tinted canopy glass
				const uint32_t colGlassFrame  = 0x121214; // Canopy frame mullions
				const uint32_t colIntakeMesh  = 0x2C2D30; // Dark titanium intake mesh
				const uint32_t colExhaustAlloy= 0x485058; // Burnt titanium exhaust alloy
				const uint32_t colSkidSteel   = 0x202224; // Steel landing skids
				const uint32_t colGunMetal    = 0x2D2E30; // 30mm chain gun receiver & barrel
				const uint32_t colRocketPod   = 0x203822; // Hydra 70 rocket pod housing
				const uint32_t colRocketTip   = 0x98A0A8; // Silver alloy rocket warheads
				const uint32_t colMissileBody = 0x224838; // Hellfire missile body
				const uint32_t colMissileHead = 0x101518; // Seeker optic head
				const uint32_t colSensorFLIR  = 0x161618; // Nose sensor pod housing
				const uint32_t colFLIRLens    = 0x08A0F8; // Golden/amber infrared lens
				const uint32_t colHUDGreen    = 0x20E040; // HUD phosphor green glow
				const uint32_t colBeaconRed   = 0x1414F0; // Strobe warning beacon
				const uint32_t colHazardYel   = 0x10D4F0; // Caution hazard stripe

				auto SetVoxel = [&](int x, int y, int z, uint32_t col) {
					if (x >= 0 && x < w && y >= 0 && y < l && z >= 0 && z < h)
						vm->SetSolid(x, y, z, col);
				};

				// 1. Landing Skids (z = 0..8, y = 10..52)
				for (int side : {-9, 9}) {
					int sx = cx + side;
					for (int y = 10; y <= 52; ++y) {
						SetVoxel(sx, y, 0, colSkidSteel);
						SetVoxel(sx, y, 1, colSkidSteel);
					}
					// Curved ski toes
					SetVoxel(sx, 9, 2, colSkidSteel);
					SetVoxel(sx, 8, 3, colSkidSteel);
					SetVoxel(sx, 7, 4, colSkidSteel);
					SetVoxel(sx, 53, 2, colSkidSteel);

					// Heavy diagonal mounting struts
					for (int sy : {18, 42}) {
						for (int sz = 2; sz <= 7; ++sz) {
							SetVoxel(sx, sy, sz, colSkidSteel);
							int inwardX = (side < 0) ? sx + (sz - 2) / 2 : sx - (sz - 2) / 2;
							SetVoxel(inwardX, sy, sz, colSkidSteel);
						}
					}
				}

				// 2. Chin 30mm Chain Gun & Sensor Turret (y = 0..12, z = 4..9)
				// FLIR sensor ball
				for (int dx = -2; dx <= 2; ++dx) {
					for (int dy = 3; dy <= 6; ++dy) {
						for (int dz = 6; dz <= 9; ++dz) {
							SetVoxel(cx + dx, dy, dz, colSensorFLIR);
						}
					}
				}
				SetVoxel(cx, 2, 7, colFLIRLens);
				SetVoxel(cx, 2, 8, colFLIRLens);
				SetVoxel(cx - 1, 3, 7, 0x00FF80); // Targeting laser designator

				// 30mm Chain Gun Turret Cradle
				for (int dx = -1; dx <= 1; ++dx) {
					for (int dy = 4; dy <= 8; ++dy) {
						SetVoxel(cx + dx, dy, 5, colGunMetal);
					}
				}
				for (int dy = 0; dy <= 5; ++dy) {
					SetVoxel(cx, dy, 4, colGunMetal);
				}
				SetVoxel(cx, 0, 4, colArmorSteel); // Muzzle brake

				// 3. Aerodynamic Nose & Lower Forward Fuselage (y = 7..20, z = 7..15)
				for (int y = 7; y <= 20; ++y) {
					float progress = (y - 7) / 13.0F;
					int halfWidth = static_cast<int>(2 + progress * 5);
					for (int dx = -halfWidth; dx <= halfWidth; ++dx) {
						for (int z = 7; z <= 12; ++z) {
							uint32_t c = ((dx + y) % 5 == 0) ? colCamoEarth : (z == 7 ? colHullDark : colHullBase);
							SetVoxel(cx + dx, y, z, c);
						}
					}
				}

				// 4. Stepped Tandem Cockpit & Canopy (y = 12..38, z = 10..22)
				for (int y = 12; y <= 38; ++y) {
					for (int dx = -5; dx <= 5; ++dx) {
						SetVoxel(cx + dx, y, 10, colHullDark);
					}
				}
				// Gunner seat (front)
				for (int dx = -1; dx <= 1; ++dx) {
					SetVoxel(cx + dx, 17, 11, colArmorSteel);
					SetVoxel(cx + dx, 18, 12, colArmorSteel);
					SetVoxel(cx + dx, 18, 13, colArmorSteel);
				}
				SetVoxel(cx, 14, 13, colHUDGreen);
				SetVoxel(cx, 15, 12, colArmorSteel);

				// Pilot seat (rear elevated)
				for (int dx = -1; dx <= 1; ++dx) {
					SetVoxel(cx + dx, 27, 12, colArmorSteel);
					SetVoxel(cx + dx, 28, 13, colArmorSteel);
					SetVoxel(cx + dx, 28, 14, colArmorSteel);
					SetVoxel(cx + dx, 28, 15, colArmorSteel);
				}
				SetVoxel(cx, 24, 15, colHUDGreen);

				// Canopy structural frame & open windows (allows conductor in window to be clearly visible from outside)
				for (int y = 12; y <= 38; ++y) {
					int topZ = 13 + static_cast<int>((y - 12) * 0.28F);
					if (topZ > 20) topZ = 20;

					// Canopy roof spine & arches
					for (int dx = -4; dx <= 4; ++dx) {
						bool isRoofFrame = (std::abs(dx) == 4 || dx == 0 || y == 12 || y == 22 || y == 34 || y == 38);
						if (isRoofFrame) {
							SetVoxel(cx + dx, y, topZ, colGlassFrame);
						}
					}
					// Side window frame posts & sill (leaving windows open for pilot visibility)
					for (int z = 11; z < topZ; ++z) {
						bool isSideFrame = (y == 12 || y == 22 || y == 34 || y == 38 || z == 11);
						if (isSideFrame) {
							SetVoxel(cx - 4, y, z, colGlassFrame);
							SetVoxel(cx + 4, y, z, colGlassFrame);
						}
					}
					// Front windshield center dividing mullion
					if (y <= 14) {
						SetVoxel(cx, y, topZ, colGlassFrame);
					}
				}

				// 5. Main Fuselage, Sponsons & Cabin (y = 20..50, z = 7..18)
				for (int y = 20; y <= 50; ++y) {
					for (int dx = -7; dx <= 7; ++dx) {
						for (int z = 7; z <= 18; ++z) {
							bool isSponson = (std::abs(dx) >= 5 && z <= 13);
							bool isCore = (std::abs(dx) <= 5);
							if (isSponson || isCore) {
								uint32_t col = colHullBase;
								if (z == 7)
									col = colHullDark;
								else if ((dx + y + z) % 7 == 0)
									col = colCamoEarth;
								else if (std::abs(dx) == 7 && z == 10)
									col = colArmorSteel;
								SetVoxel(cx + dx, y, z, col);
							}
						}
					}
				}

				// Lateral Open Edge Benches for 2 Companions (Left & Right side platforms with safety rails)
				for (int side : {-8, 8}) {
					for (int by = 28; by <= 36; ++by) {
						// Bench step platform
						SetVoxel(cx + side, by, 9, colArmorSteel);
						SetVoxel(cx + side + (side < 0 ? 1 : -1), by, 9, colArmorSteel);
						// Safety corner handrails
						if (by == 28 || by == 36) {
							SetVoxel(cx + side, by, 10, colSkidSteel);
							SetVoxel(cx + side, by, 11, colSkidSteel);
						}
					}
				}

				// 6. Dual Turboshaft Engine Nacelles (y = 26..46, z = 18..25)
				for (int side : {-6, 6}) {
					for (int y = 26; y <= 46; ++y) {
						for (int dx = -2; dx <= 2; ++dx) {
							for (int dz = -2; dz <= 2; ++dz) {
								int ex = cx + side + dx;
								int ez = 21 + dz;
								if (dx * dx + dz * dz <= 5) {
									uint32_t c = colArmorSteel;
									if (y == 26)
										c = colIntakeMesh;
									else if (y >= 43)
										c = colExhaustAlloy;
									else if ((y + dx) % 4 == 0)
										c = colHullBase;
									SetVoxel(ex, y, ez, c);
								}
							}
						}
					}
				}

				// Center Transmission Deck & Doghouse
				for (int y = 28; y <= 42; ++y) {
					for (int dx = -3; dx <= 3; ++dx) {
						for (int z = 19; z <= 24; ++z) {
							SetVoxel(cx + dx, y, z, colArmorSteel);
						}
					}
				}

				// Main Rotor Mast Housing (z = 25..29, y = 33..35)
				for (int z = 25; z <= 29; ++z) {
					for (int dx = -1; dx <= 1; ++dx) {
						for (int dy = -1; dy <= 1; ++dy) {
							SetVoxel(cx + dx, 34 + dy, z, colArmorSteel);
						}
					}
				}
				for (int dx = -2; dx <= 2; ++dx) {
					SetVoxel(cx + dx, 34, 27, colExhaustAlloy);
					SetVoxel(cx, 34 + dx, 27, colExhaustAlloy);
				}

				// 7. Stub Wings & Weapons Pylons (y = 28..34, z = 10..13)
				for (int side : {-1, 1}) {
					for (int d = 7; d <= 16; ++d) {
						int wx = cx + side * d;
						for (int y = 29; y <= 33; ++y) {
							SetVoxel(wx, y, 11, colArmorSteel);
							SetVoxel(wx, y, 12, colHullBase);
						}
					}

					// Hydra 70 Rocket Pods
					int podX = cx + side * 11;
					for (int y = 25; y <= 35; ++y) {
						for (int dx = -1; dx <= 1; ++dx) {
							for (int dz = -1; dz <= 1; ++dz) {
								uint32_t c = (y == 25) ? colRocketTip : colRocketPod;
								SetVoxel(podX + dx, y, 9 + dz, c);
							}
						}
					}

					// AGM Hellfire Missiles
					int misX = cx + side * 15;
					for (int my : {24, 27}) {
						(void)my;
						for (int y = 23; y <= 34; ++y) {
							uint32_t c = (y == 23) ? colMissileHead : colMissileBody;
							SetVoxel(misX, y, 9, c);
							SetVoxel(misX + side, y, 9, c);
						}
					}
				}

				// 8. Tapered Tail Boom & Empennage (y = 50..84, z = 13..28)
				for (int y = 50; y <= 76; ++y) {
					float taper = (y - 50) / 26.0F;
					int halfW = static_cast<int>(4.0F - taper * 2.5F);
					if (halfW < 1) halfW = 1;
					int boomZ = 14 + static_cast<int>(taper * 2.0F);

					for (int dx = -halfW; dx <= halfW; ++dx) {
						for (int dz = 0; dz <= 3; ++dz) {
							uint32_t c = ((y + dx) % 6 == 0) ? colCamoEarth : colHullBase;
							if (dz == 0) c = colHullDark;
							SetVoxel(cx + dx, y, boomZ + dz, c);
						}
					}
				}

				// Blade Antenna & Strobe Beacon
				SetVoxel(cx, 56, 18, colArmorSteel);
				SetVoxel(cx, 56, 19, colArmorSteel);
				SetVoxel(cx, 56, 20, colArmorSteel);
				SetVoxel(cx, 66, 18, colBeaconRed);

				// Horizontal Stabilator Wings (y = 72..75, z = 16)
				for (int dx = -8; dx <= 8; ++dx) {
					for (int y = 72; y <= 75; ++y) {
						SetVoxel(cx + dx, y, 16, colHullBase);
					}
					if (std::abs(dx) == 8) {
						for (int ez = 14; ez <= 18; ++ez) {
							SetVoxel(cx + dx, 73, ez, colArmorSteel);
						}
					}
				}

				// Swept Vertical Stabilizer Fin (y = 75..83, z = 15..30)
				for (int y = 75; y <= 83; ++y) {
					float finProgress = (y - 75) / 8.0F;
					int maxFinZ = 18 + static_cast<int>(finProgress * 12.0F);
					for (int z = 15; z <= maxFinZ; ++z) {
						uint32_t c = (z == maxFinZ) ? colArmorSteel : colHullBase;
						SetVoxel(cx, y, z, c);
					}
				}
				SetVoxel(cx, 83, 30, colBeaconRed);

				// Starboard Tail Rotor Assembly
				int tailRotorX = cx + 2;
				for (int dz = -1; dz <= 1; ++dz) {
					for (int dy = -1; dy <= 1; ++dy) {
						SetVoxel(tailRotorX - 1, 80 + dy, 24 + dz, colArmorSteel);
						SetVoxel(tailRotorX, 80 + dy, 24 + dz, colExhaustAlloy);
					}
				}
				for (int r = 2; r <= 5; ++r) {
					uint32_t c = (r >= 4) ? colHazardYel : colArmorSteel;
					SetVoxel(tailRotorX, 80, 24 + r, c);
					SetVoxel(tailRotorX, 80, 24 - r, c);
					SetVoxel(tailRotorX, 80 + r, 24, c);
					SetVoxel(tailRotorX, 80 - r, 24, c);
				}

				g_heliFuselageModel = renderer.CreateModel(*vm);
				return g_heliFuselageModel;
			} catch (...) {
				return Handle<IModel>();
			}
		}

		
		// =====================================================================
		// 1. CHEVROLET CHEVELLE / MALIBU STREET COUPE 1970s (VEHICLE 1 - HD)
		// =====================================================================
		static Handle<IModel> GenerateChevelleBody(IRenderer& renderer) {
			try {
				int const w = 28;
				int const l = 58;
				int const h = 18;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-14.0F, -29.0F, 0.0F));
				const int cx = w / 2; // 14

				// Authentic 1970s Street Muscle Palette (0xBBGGRR little-endian):
				const uint32_t colWhite        = 0xEDEDED; // Weathered vintage classic white
				const uint32_t colWhiteShade   = 0xD0D4D8; // Body contour shadow
				const uint32_t colWhiteHigh    = 0xF8FAFC; // Sunlit hood highlight
				const uint32_t colWhiteDark    = 0xB8BEC4; // Underside rocker panels
				const uint32_t colRedStripe    = 0x2020C8; // Iconic 70s Muscle Red side stripe
				const uint32_t colRedBezel     = 0x1A1AB4; // Front quad headlight surround red bezel
				const uint32_t colChrome       = 0xF0F4F8; // Mirror chrome bumpers and trim
				const uint32_t colChromeDark   = 0x9098A0; // Chrome shadow
				const uint32_t colBlack        = 0x141416; // Grille, underbody
				const uint32_t colLightGlass   = 0xF0F8FF; // Quad sealed-beam round headlight glass
				const uint32_t colLightBulb    = 0xFFFDF2; // Intense headlight center bulb
				const uint32_t colAmber        = 0x1088F0; // Amber turn signal indicator
				const uint32_t colTaillight    = 0x1818D8; // Deep ruby red rear taillight
				const uint32_t colTaillightRev = 0xD2D8E0; // Clear white reverse section
				const uint32_t colInterior     = 0x282624; // Dark vinyl dashboard
				const uint32_t colSeat         = 0x3C3630; // 70s vinyl bucket seats
				const uint32_t colSteering     = 0x181818; // 3-spoke classic sport steering wheel

				auto SetVoxel = [&](int x, int y, int z, uint32_t col) {
					if (x >= 0 && x < w && y >= 0 && y < l && z >= 0 && z < h)
						vm->SetSolid(x, y, z, col);
				};

				// 1. Underbody & Frame (Z = 2..4, Y = 4..53)
				for (int y = 4; y <= 53; ++y) {
					for (int dx = -8; dx <= 8; ++dx) {
						SetVoxel(cx + dx, y, 3, colBlack);
						if (std::abs(dx) <= 6)
							SetVoxel(cx + dx, y, 4, colBlack);
					}
				}

				// 2. Front Massive Wrap-around Chrome Bumper (Y = 1..3, Z = 4..7)
				for (int x = 2; x <= 25; ++x) {
					SetVoxel(x, 1, 5, colChrome);
					SetVoxel(x, 2, 5, colChrome);
					SetVoxel(x, 1, 4, colChromeDark);
					SetVoxel(x, 2, 4, colChromeDark);
					SetVoxel(x, 1, 6, colChrome);
				}
				// Bumper wrap-around corners
				SetVoxel(1, 2, 5, colChrome);
				SetVoxel(1, 3, 5, colChrome);
				SetVoxel(26, 2, 5, colChrome);
				SetVoxel(26, 3, 5, colChrome);

				// Front central license plate (AMJ-736 style)
				for (int dx = -3; dx <= 3; ++dx) {
					SetVoxel(cx + dx, 0, 4, 0xEEEEEE);
					SetVoxel(cx + dx, 0, 5, 0x1A1A1A);
				}

				// 3. Recessed Black Grille & Quad Headlights with RED BEZELS
				for (int x = 3; x <= 24; ++x) {
					for (int z = 6; z <= 9; ++z) {
						SetVoxel(x, 2, z, colBlack);
						SetVoxel(x, 3, z, colBlack);
					}
				}
				auto DrawQuadHeadlightPair = [&](int startX) {
					for (int dx = 0; dx <= 6; ++dx) {
						for (int dz = 0; dz <= 3; ++dz) {
							SetVoxel(startX + dx, 2, 6 + dz, colRedBezel);
						}
					}
					int lamp1X = startX + 1;
					int lamp2X = startX + 4;
					for (int dz = 1; dz <= 2; ++dz) {
						SetVoxel(lamp1X, 1, 6 + dz, colLightGlass);
						SetVoxel(lamp1X + 1, 1, 6 + dz, colLightGlass);
						SetVoxel(lamp2X, 1, 6 + dz, colLightGlass);
						SetVoxel(lamp2X + 1, 1, 6 + dz, colLightGlass);
					}
					SetVoxel(lamp1X, 1, 7, colLightBulb);
					SetVoxel(lamp2X, 1, 7, colLightBulb);
				};
				DrawQuadHeadlightPair(3);
				DrawQuadHeadlightPair(18);

				// Amber corner indicator lights
				for (int z = 6; z <= 8; ++z) {
					SetVoxel(2, 2, z, colAmber);
					SetVoxel(25, 2, z, colAmber);
				}

				// 4. Long Muscle Car Hood with Cowl Induction Bulge (Y = 4..21, Z = 9..11)
				for (int y = 4; y <= 21; ++y) {
					for (int x = 2; x <= 25; ++x) {
						int hoodZ = 9;
						if (std::abs(x - cx) <= 4 && y >= 7 && y <= 20)
							hoodZ = 10; // Cowl induction scoop
						SetVoxel(x, y, hoodZ, (std::abs(x - cx) <= 3) ? colWhiteHigh : ((x == 2 || x == 25) ? colWhiteShade : colWhite));
						SetVoxel(x, y, hoodZ - 1, colWhiteShade);
					}
				}

				// 5. Flanks, Doors & PERFECTLY CENTERED WHEEL ARCHES
				// Front Arch: centered at Y = 12 (Y - 29 = -17.0F), range y = 7..17 (11 voxels wide)
				// Rear Arch: centered at Y = 44 (Y - 29 = +15.0F), range y = 39..49 (11 voxels wide)
				for (int y = 4; y <= 53; ++y) {
					bool inFrontArch = (y >= 7 && y <= 17);
					bool inRearArch  = (y >= 39 && y <= 49);
					int zStart = (inFrontArch || inRearArch) ? 8 : 4;

					for (int z = zStart; z <= 10; ++z) {
						uint32_t colL = colWhite;
						uint32_t colR = colWhite;
						if (z == 4) {
							colL = colR = colWhiteDark;
						} else if (z == 8) {
							colL = colR = colRedStripe; // Iconic Red Muscle Stripe
						} else if (z == 10) {
							colL = colR = colWhiteShade;
						}
						SetVoxel(2, y, z, colL);
						SetVoxel(3, y, z, colL);
						SetVoxel(24, y, z, colR);
						SetVoxel(25, y, z, colR);
					}

					// Chrome wheel arch trim along the cutout rim
					if (inFrontArch || inRearArch) {
						SetVoxel(2, y, 8, colChrome);
						SetVoxel(25, y, 8, colChrome);
					}
				}

				// Chrome side mirrors on doors (Y = 22, Z = 11)
				SetVoxel(0, 22, 11, colChrome);
				SetVoxel(1, 22, 11, colChrome);
				SetVoxel(26, 22, 11, colChrome);
				SetVoxel(27, 22, 11, colChrome);

				// 6. Cabin Interior: Vinyl bucket seats, dash, steering wheel
				for (int y = 22; y <= 35; ++y) {
					for (int x = 4; x <= 23; ++x) {
						SetVoxel(x, y, 4, colInterior);
					}
				}
				for (int y = 26; y <= 31; ++y) {
					for (int x = 6; x <= 12; ++x) { // Driver bucket seat
						SetVoxel(x, y, 5, colSeat);
						SetVoxel(x, 31, 6, colSeat);
						SetVoxel(x, 31, 7, colSeat);
					}
					for (int x = 15; x <= 21; ++x) { // Passenger bucket seat
						SetVoxel(x, y, 5, colSeat);
						SetVoxel(x, 31, 6, colSeat);
						SetVoxel(x, 31, 7, colSeat);
					}
				}
				// Dashboard & 3-spoke sport steering wheel
				for (int x = 5; x <= 22; ++x) {
					SetVoxel(x, 22, 9, colInterior);
					SetVoxel(x, 22, 10, colInterior);
				}
				int stX = 9;
				for (int dx = -1; dx <= 1; ++dx) {
					for (int dz = -1; dz <= 1; ++dz) {
						if (std::abs(dx) == 1 || std::abs(dz) == 1)
							SetVoxel(stX + dx, 24, 10 + dz, colSteering);
					}
				}
				SetVoxel(stX, 23, 10, colChrome);

				// 7. Slanted A-Pillars, Open Windows & Fastback Roof
				for (int step = 0; step < 4; ++step) {
					int pY = 22 + step * 2;
					int pZ = 11 + step;
					SetVoxel(3, pY, pZ, colWhiteShade);
					SetVoxel(4, pY, pZ, colWhite);
					SetVoxel(23, pY, pZ, colWhite);
					SetVoxel(24, pY, pZ, colWhiteShade);
				}
				// Solid Roof (Y = 28..37, Z = 14)
				for (int y = 28; y <= 37; ++y) {
					for (int x = 4; x <= 23; ++x) {
						SetVoxel(x, y, 14, (x == 4 || x == 23 || y == 28 || y == 37) ? colWhiteShade : colWhite);
					}
				}
				// Thin B-pillar posts
				for (int z = 11; z <= 13; ++z) {
					SetVoxel(3, 31, z, colWhiteShade);
					SetVoxel(24, 31, z, colWhiteShade);
				}
				// Fastback C-Pillars sloping into rear deck with hockey-stick red stripe
				for (int step = 0; step < 6; ++step) {
					int cY = 38 + step * 2;
					int cZ = 14 - step;
					if (cZ >= 10) {
						for (int wX = 0; wX <= 2; ++wX) {
							SetVoxel(2 + wX, cY, cZ, colWhite);
							SetVoxel(25 - wX, cY, cZ, colWhite);
						}
						if (cZ == 10 || cZ == 11) {
							SetVoxel(2, cY, cZ, colRedStripe);
							SetVoxel(3, cY, cZ, colRedStripe);
							SetVoxel(24, cY, cZ, colRedStripe);
							SetVoxel(25, cY, cZ, colRedStripe);
						}
					}
				}

				// 8. Rear Trunk Deck & Chrome Bumper
				for (int y = 46; y <= 55; ++y) {
					for (int x = 3; x <= 24; ++x) {
						SetVoxel(x, y, 9, (x == 3 || x == 24) ? colWhiteShade : colWhite);
						SetVoxel(x, y, 8, colWhiteDark);
					}
				}
				// Rear Tail Panel & Taillights
				for (int x = 3; x <= 24; ++x) {
					for (int z = 6; z <= 8; ++z) {
						SetVoxel(x, 56, z, colWhiteDark);
					}
				}
				for (int dx = 0; dx <= 4; ++dx) {
					SetVoxel(4 + dx, 56, 7, colTaillight);
					SetVoxel(4 + dx, 56, 8, colTaillight);
					SetVoxel(19 + dx, 56, 7, colTaillight);
					SetVoxel(19 + dx, 56, 8, colTaillight);
				}
				SetVoxel(7, 56, 7, colTaillightRev);
				SetVoxel(20, 56, 7, colTaillightRev);

				// Rear Chrome Bumper (Y = 56..57, Z = 4..5)
				for (int x = 2; x <= 25; ++x) {
					SetVoxel(x, 56, 5, colChrome);
					SetVoxel(x, 57, 5, colChrome);
					SetVoxel(x, 56, 4, colChromeDark);
					SetVoxel(x, 57, 4, colChromeDark);
				}
				SetVoxel(1, 55, 5, colChrome);
				SetVoxel(26, 55, 5, colChrome);

				// Dual Chrome Exhaust Pipes
				SetVoxel(7, 57, 3, colChrome);
				SetVoxel(20, 57, 3, colChrome);

				return renderer.CreateModel(*vm);
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// =====================================================================
		// 2. TOYOTA LAND CRUISER SERIE 79 PICK-UP "MACHO" 4X4 (VEHICLE 2 - HD)
		// =====================================================================
		static Handle<IModel> GenerateToyotaLC79Body(IRenderer& renderer) {
			try {
				int const w = 28;
				int const l = 60;
				int const h = 23;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-14.0F, -30.0F, 0.0F));
				const int cx = w / 2; // 14

				// Authentic Venezuelan Land Cruiser 70-series Palette (0xBBGGRR):
				const uint32_t colWhite        = 0xF2F4F6; // Pure Toyota White
				const uint32_t colWhiteShade   = 0xD0D6DC; // Contour shadow
				const uint32_t colWhiteHigh    = 0xFFFFFF; // Sunlit roof & hood highlight
				const uint32_t colBlackSteel   = 0x141416; // ARB-style matte black steel bumper
				const uint32_t colGrilleBlack  = 0x101012; // Front mesh grille
				const uint32_t colLetterWhite  = 0xFFFFFF; // Bold white "TOYOTA" emblem
				const uint32_t colHeadlight    = 0xF8FAFC; // Twin round sealed-beam lenses
				const uint32_t colAmber        = 0x1080F0; // Corner indicators
				const uint32_t colFogLamp      = 0xEEFFFF; // Round auxiliary bumper LED driving lights
				const uint32_t colSnorkel      = 0x161618; // Heavy-duty raised snorkel on right A-pillar
				const uint32_t colStripeOrange = 0x1874E4; // Retro side stripe: bright orange
				const uint32_t colStripeRed    = 0x141EC4; // Retro side stripe: bold red
				const uint32_t colFenderFlare  = 0x18181A; // Wide black offroad fender flare arches
				const uint32_t colBedLiner     = 0x1E2022; // Textured black bedliner floor
				const uint32_t colInterior     = 0x2A2C30; // Heavy-duty utility dashboard
				const uint32_t colSeat         = 0x40444A; // Durable grey cloth off-road seats
				const uint32_t colTaillight    = 0x1616D0; // Red rear lights

				auto SetVoxel = [&](int x, int y, int z, uint32_t col) {
					if (x >= 0 && x < w && y >= 0 && y < l && z >= 0 && z < h)
						vm->SetSolid(x, y, z, col);
				};

				// 1. Heavy Boxed Steel Ladder Chassis (Z = 3..5, Y = 4..56)
				for (int y = 4; y <= 56; ++y) {
					for (int dx = -7; dx <= 7; ++dx) {
						SetVoxel(cx + dx, y, 4, 0x121214);
						if (std::abs(dx) <= 5)
							SetVoxel(cx + dx, y, 5, 0x121214);
					}
				}

				// 2. Heavy-Duty Steel Bullbar & Winch Bumper (Y = 1..3, Z = 5..9)
				for (int x = 1; x <= 26; ++x) {
					SetVoxel(x, 1, 6, colBlackSteel);
					SetVoxel(x, 2, 6, colBlackSteel);
					SetVoxel(x, 1, 7, colBlackSteel);
					SetVoxel(x, 2, 7, colBlackSteel);
				}
				// Angled bumper wings protecting wheel arches
				SetVoxel(1, 3, 6, colBlackSteel);
				SetVoxel(1, 3, 7, colBlackSteel);
				SetVoxel(26, 3, 6, colBlackSteel);
				SetVoxel(26, 3, 7, colBlackSteel);
				// Center grille-guard tubular hoop
				for (int x = 8; x <= 19; ++x) {
					SetVoxel(x, 1, 10, colBlackSteel);
				}
				SetVoxel(8, 1, 8, colBlackSteel);
				SetVoxel(8, 1, 9, colBlackSteel);
				SetVoxel(19, 1, 8, colBlackSteel);
				SetVoxel(19, 1, 9, colBlackSteel);
				// Round auxiliary LED spot lamps
				SetVoxel(6, 0, 7, colFogLamp);
				SetVoxel(7, 0, 7, colFogLamp);
				SetVoxel(20, 0, 7, colFogLamp);
				SetVoxel(21, 0, 7, colFogLamp);
				// Venezuelan A76BJ2P License Plate
				for (int px = 11; px <= 16; ++px) {
					SetVoxel(px, 0, 6, 0xEEEEEE);
					SetVoxel(px, 0, 7, 0x2244AA);
				}

				// 3. Black Horizontal Grille with Bold White "TOYOTA" & Round Headlights
				for (int x = 3; x <= 24; ++x) {
					for (int z = 8; z <= 12; ++z) {
						SetVoxel(x, 2, z, colGrilleBlack);
					}
				}
				// Twin round headlights
				auto DrawRoundHeadlight = [&](int hx) {
					for (int dx = 0; dx <= 2; ++dx) {
						for (int dz = 0; dz <= 2; ++dz) {
							if ((dx == 0 || dx == 2) && (dz == 0 || dz == 2))
								continue;
							SetVoxel(hx + dx, 1, 9 + dz, colHeadlight);
						}
					}
				};
				DrawRoundHeadlight(4);
				DrawRoundHeadlight(21);
				// Corner Amber Turn Signals
				SetVoxel(2, 2, 9, colAmber);
				SetVoxel(2, 2, 10, colAmber);
				SetVoxel(25, 2, 9, colAmber);
				SetVoxel(25, 2, 10, colAmber);

				// Bold "TOYOTA" in center of grille
				for (int bx = 11; bx <= 16; ++bx) {
					SetVoxel(bx, 1, 10, colLetterWhite);
				}

				// 4. Sturdy High Square Hood (Y = 4..22, Z = 11..13)
				for (int y = 4; y <= 22; ++y) {
					for (int x = 2; x <= 25; ++x) {
						SetVoxel(x, y, 12, (x == 2 || x == 25) ? colWhiteShade : colWhite);
						SetVoxel(x, y, 11, colWhiteShade);
					}
				}

				// 5. Safari Snorkel on Right A-Pillar (Driver/Passenger side depending on LH/RH)
				// Rises from front fender (x = 26, y = 14) up along A-pillar to roof (z = 21)
				for (int z = 10; z <= 21; ++z) {
					SetVoxel(26, 21, z, colSnorkel);
					SetVoxel(27, 21, z, colSnorkel);
				}
				// Ram-air forward scoop
				for (int sy = 18; sy <= 21; ++sy) {
					for (int sz = 20; sz <= 22; ++sz) {
						SetVoxel(26, sy, sz, colSnorkel);
						SetVoxel(27, sy, sz, colSnorkel);
					}
				}

				// 6. PERFECTLY CENTERED WHEEL ARCHES & FLARES:
				// Front Arch: centered at Y = 12 (Y - 30 = -18.0F), range y = 7..17 (11 voxels wide)
				// Rear Arch: centered at Y = 46 (Y - 30 = +16.0F), range y = 41..51 (11 voxels wide)
				for (int y = 4; y <= 56; ++y) {
					bool inFrontArch = (y >= 7 && y <= 17);
					bool inRearArch  = (y >= 41 && y <= 51);
					int zStart = (inFrontArch || inRearArch) ? 9 : 6;

					for (int z = zStart; z <= 12; ++z) {
						uint32_t c = colWhite;
						if (z == 6) c = 0x222224; // Lower black rocker sill
						else if (z == 9) c = colStripeOrange; // Retro side stripe
						else if (z == 10) c = colStripeRed;    // Retro side stripe
						else if (z == 12) c = colWhiteShade;

						SetVoxel(2, y, z, c);
						SetVoxel(3, y, z, c);
						SetVoxel(24, y, z, c);
						SetVoxel(25, y, z, c);
					}

					// Black protective flared wheel arch aletines extending outwards
					if (inFrontArch || inRearArch) {
						SetVoxel(1, y, 9, colFenderFlare);
						SetVoxel(2, y, 9, colFenderFlare);
						SetVoxel(25, y, 9, colFenderFlare);
						SetVoxel(26, y, 9, colFenderFlare);
					}
				}

				// Black side mirrors on doors (Y = 23, Z = 13..15)
				for (int mz = 13; mz <= 15; ++mz) {
					SetVoxel(0, 23, mz, colBlackSteel);
					SetVoxel(27, 23, mz, colBlackSteel);
				}

				// 7. Single Cab Interior: Seats, Dash, Steering Wheel
				for (int y = 23; y <= 34; ++y) {
					for (int x = 4; x <= 23; ++x) {
						SetVoxel(x, y, 7, colInterior);
					}
				}
				for (int y = 28; y <= 33; ++y) {
					for (int x = 5; x <= 13; ++x) { // Driver seat
						SetVoxel(x, y, 8, colSeat);
						SetVoxel(x, 33, 9, colSeat);
						SetVoxel(x, 33, 10, colSeat);
					}
					for (int x = 15; x <= 23; ++x) { // Passenger seat
						SetVoxel(x, y, 8, colSeat);
						SetVoxel(x, 33, 9, colSeat);
						SetVoxel(x, 33, 10, colSeat);
					}
				}
				for (int x = 5; x <= 22; ++x) {
					SetVoxel(x, 23, 11, colInterior);
					SetVoxel(x, 23, 12, colInterior);
				}
				for (int dx = -1; dx <= 1; ++dx) {
					for (int dz = -1; dz <= 1; ++dz) {
						if (std::abs(dx) == 1 || std::abs(dz) == 1)
							SetVoxel(8 + dx, 25, 12 + dz, 0x181818);
					}
				}

				// 8. Slanted A-Pillars, Open Window Frames & Solid Roof
				for (int step = 0; step < 4; ++step) {
					int pY = 22 + step * 2;
					int pZ = 13 + step * 1;
					SetVoxel(3, pY, pZ, colWhiteShade);
					SetVoxel(4, pY, pZ, colWhite);
					SetVoxel(23, pY, pZ, colWhite);
					SetVoxel(24, pY, pZ, colWhiteShade);
				}
				for (int y = 28; y <= 36; ++y) {
					for (int x = 3; x <= 24; ++x) {
						SetVoxel(x, y, 18, (x == 3 || x == 24 || y == 28 || y == 36) ? colWhiteShade : colWhiteHigh);
					}
				}
				// Rear cab vertical wall (Y = 36) with open center slider window for shooting
				for (int x = 3; x <= 24; ++x) {
					for (int z = 7; z <= 17; ++z) {
						if (z >= 13 && z <= 16 && x >= 7 && x <= 20)
							continue; // Open rear window
						SetVoxel(x, 36, z, colWhite);
					}
				}

				// 9. Rugged Pickup Cargo Bed & Tailgate
				for (int y = 37; y <= 56; ++y) {
					for (int x = 4; x <= 23; ++x) {
						SetVoxel(x, y, 8, ((x % 2) == 0) ? colBedLiner : 0x141618);
					}
				}
				for (int y = 37; y <= 56; ++y) {
					for (int z = 9; z <= 13; ++z) {
						SetVoxel(3, y, z, colWhite);
						SetVoxel(4, y, z, colWhiteShade);
						SetVoxel(23, y, z, colWhiteShade);
						SetVoxel(24, y, z, colWhite);
					}
				}
				// Tailgate (Y = 56) with bold "TOYOTA" stamp
				for (int x = 3; x <= 24; ++x) {
					for (int z = 8; z <= 13; ++z) {
						SetVoxel(x, 56, z, (z == 13) ? colWhiteShade : colWhite);
					}
				}
				for (int tx = 10; tx <= 17; ++tx) {
					SetVoxel(tx, 57, 11, colBlackSteel);
				}
				// Vertical Red Tail Lights
				for (int tz = 9; tz <= 12; ++tz) {
					SetVoxel(3, 57, tz, colTaillight);
					SetVoxel(24, 57, tz, colTaillight);
				}
				// Rear heavy steel step bumper
				for (int x = 2; x <= 25; ++x) {
					SetVoxel(x, 57, 6, colBlackSteel);
					SetVoxel(x, 58, 6, colBlackSteel);
				}
				// Rubber mudflaps behind rear wheels
				for (int mz = 4; mz <= 8; ++mz) {
					SetVoxel(2, 52, mz, colBlackSteel);
					SetVoxel(3, 52, mz, colBlackSteel);
					SetVoxel(24, 52, mz, colBlackSteel);
					SetVoxel(25, 52, mz, colBlackSteel);
				}

				return renderer.CreateModel(*vm);
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// =====================================================================
		// 3. MILITARY ISV (INFANTRY SQUAD VEHICLE) TACTICAL BUGGY (VEHICLE 3 - HD)
		// =====================================================================
		static Handle<IModel> GenerateMilitaryISVBody(IRenderer& renderer) {
			try {
				int const w = 28;
				int const l = 58;
				int const h = 21;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-14.0F, -29.0F, 0.0F));
				const int cx = w / 2; // 14

				// Authentic CARC Olive Drab Military Palette (0xBBGGRR):
				const uint32_t colOliveDrab    = 0x304834; // CARC Olive Drab green
				const uint32_t colOliveShade   = 0x223626; // Shadow camouflage green
				const uint32_t colOliveHigh    = 0x3E5C44; // Sunlit highlight olive
				const uint32_t colTubeCage     = 0x1A2E1E; // Heavy-duty welded tubular roll cage
				const uint32_t colChassisBlack = 0x141416; // Armor skid plates & frame
				const uint32_t colSpringYellow = 0x14D0E8; // Yellow coilover suspension springs
				const uint32_t colShockSilver  = 0xD8DDE2; // Silver shock absorber body
				const uint32_t colSeatTan      = 0x3E5466; // US Military Coyote Tan tactical bucket seats
				const uint32_t colHarness      = 0x18181A; // 5-point harness belts
				const uint32_t colStealthHead  = 0xE8F8FF; // Slim stealth LED headlights
				const uint32_t colWinch        = 0x242428; // Front tactical recovery winch

				auto SetVoxel = [&](int x, int y, int z, uint32_t col) {
					if (x >= 0 && x < w && y >= 0 && y < l && z >= 0 && z < h)
						vm->SetSolid(x, y, z, col);
				};

				// Continuous Solid Tubular Beam Generator (Guarantees zero floating gaps/holes):
				auto DrawTubeLine = [&](int x1, int y1, int z1, int x2, int y2, int z2, uint32_t col, bool doubleThick = false) {
					int dx = std::abs(x2 - x1);
					int dy = std::abs(y2 - y1);
					int dz = std::abs(z2 - z1);
					int steps = std::max(dx, std::max(dy, dz));
					if (steps == 0) {
						SetVoxel(x1, y1, z1, col);
						if (doubleThick) SetVoxel(x1, y1, z1 + 1, col);
						return;
					}
					for (int i = 0; i <= steps; ++i) {
						float t = static_cast<float>(i) / static_cast<float>(steps);
						int px = static_cast<int>(std::round(x1 + (x2 - x1) * t));
						int py = static_cast<int>(std::round(y1 + (y2 - y1) * t));
						int pz = static_cast<int>(std::round(z1 + (z2 - z1) * t));
						SetVoxel(px, py, pz, col);
						if (doubleThick) {
							SetVoxel(px, py, pz + 1, col);
						}
					}
				};

				// 1. Solid Heavy Armor Floorpan & Skid Plates (Z = 4..5, Y = 4..54, X = 4..23)
				// Zero gaps under the cockpit or passenger compartment
				for (int y = 4; y <= 54; ++y) {
					for (int x = 4; x <= 23; ++x) {
						SetVoxel(x, y, 4, colChassisBlack);
						SetVoxel(x, y, 5, colChassisBlack);
					}
				}

				// 2. Heavy-Duty Side Rocker Sills (X = 3, 24, Y = 10..52, Z = 5..6)
				// Continuous solid structural lower tube with dark stamped lightening holes
				for (int y = 10; y <= 52; ++y) {
					bool isLighteningHole = ((y % 5) == 0 && y >= 15 && y <= 45);
					uint32_t cSill = isLighteningHole ? colChassisBlack : colTubeCage;
					SetVoxel(3, y, 5, cSill);
					SetVoxel(3, y, 6, cSill);
					SetVoxel(24, y, 5, cSill);
					SetVoxel(24, y, 6, cSill);
				}

				// 3. Angular Stealth Hood, Nose, Winch & Recessed LED Headlights (Y = 2..18)
				for (int y = 2; y <= 18; ++y) {
					int hoodZ = 7 + (18 - y) / 5;
					for (int x = 4; x <= 23; ++x) {
						uint32_t c = (x == 4 || x == 23 || y == 2) ? colOliveShade : colOliveDrab;
						SetVoxel(x, y, hoodZ, c);
						SetVoxel(x, y, hoodZ - 1, colOliveShade);
					}
				}
				// Solid Firewall separating engine from cockpit (Y = 18..19, Z = 6..10)
				for (int y = 18; y <= 19; ++y) {
					for (int x = 4; x <= 23; ++x) {
						for (int z = 6; z <= 10; ++z) {
							SetVoxel(x, y, z, colOliveShade);
						}
					}
				}
				// Front Nose & Slim Tactical LED Headlights
				for (int x = 5; x <= 22; ++x) {
					SetVoxel(x, 1, 7, colChassisBlack);
					SetVoxel(x, 1, 8, colOliveShade);
				}
				SetVoxel(6, 1, 9, colStealthHead);
				SetVoxel(7, 1, 9, colStealthHead);
				SetVoxel(20, 1, 9, colStealthHead);
				SetVoxel(21, 1, 9, colStealthHead);
				// Front recovery winch
				for (int wx = 12; wx <= 15; ++wx) {
					SetVoxel(wx, 0, 6, colWinch);
					SetVoxel(wx, 0, 7, colWinch);
				}

				// 4. Heavy-Duty Suspension Coilovers & Arch Fenders:
				// Front Arch Center: Y = 11 (range y = 6..16)
				// Rear Arch Center: Y = 45 (range y = 40..50)
				auto DrawSolidCoilover = [&](int archCenterY) {
					for (int sz = 4; sz <= 9; ++sz) {
						uint32_t sc = (sz % 2 == 0) ? colSpringYellow : colShockSilver;
						SetVoxel(6, archCenterY, sz, sc);
						SetVoxel(6, archCenterY - 1, sz, sc);
						SetVoxel(21, archCenterY, sz, sc);
						SetVoxel(21, archCenterY - 1, sz, sc);
					}
				};
				DrawSolidCoilover(11);
				DrawSolidCoilover(45);

				// Tubular Fenders above wheels
				for (int y = 6; y <= 16; ++y) {
					SetVoxel(3, y, 9, colTubeCage);
					SetVoxel(24, y, 9, colTubeCage);
				}
				for (int y = 40; y <= 50; ++y) {
					SetVoxel(3, y, 9, colTubeCage);
					SetVoxel(24, y, 9, colTubeCage);
				}

				// 5. Squad Tactical Bucket Seats (Rows 1 & 2)
				auto DrawTacticalSeat = [&](int seatX, int seatY) {
					for (int sx = 0; sx <= 4; ++sx) {
						for (int sy = 0; sy <= 4; ++sy) {
							SetVoxel(seatX + sx, seatY + sy, 6, colSeatTan);
						}
						for (int sz = 7; sz <= 11; ++sz) {
							SetVoxel(seatX + sx, seatY + 4, sz, colSeatTan);
							if (sx == 1 || sx == 3)
								SetVoxel(seatX + sx, seatY + 3, sz, colHarness);
						}
					}
				};
				DrawTacticalSeat(5, 22);  // Driver
				DrawTacticalSeat(18, 22); // Gunner / Passenger
				DrawTacticalSeat(5, 34);  // Squad seat 3
				DrawTacticalSeat(18, 34); // Squad seat 4

				// Center console & tactical steering
				for (int x = 12; x <= 15; ++x) {
					for (int y = 20; y <= 38; ++y) {
						SetVoxel(x, y, 6, colChassisBlack);
					}
				}
				for (int dx = -1; dx <= 1; ++dx) {
					for (int dz = -1; dz <= 1; ++dz) {
						if (std::abs(dx) == 1 || std::abs(dz) == 1)
							SetVoxel(7 + dx, 20, 10 + dz, 0x1A1A1A);
					}
				}

				// 6. Complete Continuous Triangulated Exoskeleton Roll Cage (100% Solid, NO Floating Gaps)
				// A-Pillar tubes (Cowl y=18, z=10 to roof y=25, z=18)
				DrawTubeLine(3, 18, 10, 3, 25, 18, colTubeCage, true);
				DrawTubeLine(24, 18, 10, 24, 25, 18, colTubeCage, true);

				// Front Roof Header
				DrawTubeLine(3, 25, 18, 24, 25, 18, colTubeCage, true);

				// Roof Longitudinal Outer Rails (Y = 25..48, Z = 18)
				DrawTubeLine(3, 25, 18, 3, 48, 18, colTubeCage, true);
				DrawTubeLine(24, 25, 18, 24, 48, 18, colTubeCage, true);

				// B-Pillar Vertical Posts (Y = 34, Z = 6..18)
				DrawTubeLine(3, 34, 6, 3, 34, 18, colTubeCage, true);
				DrawTubeLine(24, 34, 6, 24, 34, 18, colTubeCage, true);
				DrawTubeLine(3, 34, 18, 24, 34, 18, colTubeCage, true); // B-pillar roof crossbar

				// C-Pillar Vertical Posts (Y = 44, Z = 6..18)
				DrawTubeLine(3, 44, 6, 3, 44, 18, colTubeCage, true);
				DrawTubeLine(24, 44, 6, 24, 44, 18, colTubeCage, true);
				DrawTubeLine(3, 44, 18, 24, 44, 18, colTubeCage, true); // C-pillar roof crossbar

				// Rear Stanchion / D-Pillar (Y = 54, Z = 6 to Y = 48, Z = 18)
				DrawTubeLine(3, 54, 6, 3, 48, 18, colTubeCage, true);
				DrawTubeLine(24, 54, 6, 24, 48, 18, colTubeCage, true);
				DrawTubeLine(3, 48, 18, 24, 48, 18, colTubeCage, true); // Rear roof crossbar

				// Side Diagonal Braces (Completely continuous, no gaps, authentic military truss)
				DrawTubeLine(3, 34, 18, 3, 44, 7, colTubeCage, true);
				DrawTubeLine(3, 34, 7, 3, 44, 18, colTubeCage, true);
				DrawTubeLine(24, 34, 18, 24, 44, 7, colTubeCage, true);
				DrawTubeLine(24, 34, 7, 24, 44, 18, colTubeCage, true);

				DrawTubeLine(3, 44, 18, 3, 54, 7, colTubeCage, true);
				DrawTubeLine(24, 44, 18, 24, 54, 7, colTubeCage, true);

				// Roof Center Longitudinal Spine & Cargo Net Webbing
				DrawTubeLine(cx, 25, 18, cx, 48, 18, colTubeCage);
				for (int y = 26; y <= 47; y += 3) {
					for (int x = 4; x <= 23; ++x) {
						if ((x + y) % 2 == 0)
							SetVoxel(x, y, 18, 0x1E2420); // Tactical bungee net
					}
				}

				// 7. Rear Cargo Bed (Solid floor, ammo crates, rear bumper & lights)
				for (int y = 45; y <= 54; ++y) {
					for (int x = 5; x <= 22; ++x) {
						SetVoxel(x, y, 6, colOliveDrab);
						SetVoxel(x, y, 7, ((x + y) % 3 == 0) ? colSeatTan : colOliveShade);
					}
				}
				// Rear Bumper & LED Taillights
				for (int x = 3; x <= 24; ++x) {
					SetVoxel(x, 55, 6, colTubeCage);
				}
				SetVoxel(5, 55, 7, 0x1616C8);
				SetVoxel(22, 55, 7, 0x1616C8);

				return renderer.CreateModel(*vm);
			try_end:;
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// =====================================================================
		// 4. GMC SIERRA DENALI 4X4 LUXURY CREW CAB TRUCK (VEHICLE 4 - HD)
		// =====================================================================
		static Handle<IModel> GenerateGMCDenaliBody(IRenderer& renderer) {
			try {
				int const w = 30;
				int const l = 64;
				int const h = 23;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-15.0F, -32.0F, 0.0F));
				const int cx = w / 2; // 15

				// Authentic GMC Quicksilver Metallic & Denali Luxury Palette (0xBBGGRR):
				const uint32_t colSilverMetal   = 0xA2ACB6; // Metallic Quicksilver body
				const uint32_t colSilverShade   = 0x76808A; // Chiseled body crease shadow
				const uint32_t colSilverHigh    = 0xC4CED8; // Sunlit metallic specular crest
				const uint32_t colMirrorChrome  = 0xF4F8FC; // Gleaming mirror chrome Denali grille & trim
				const uint32_t colChromeShade   = 0xC8D0D8; // Chrome louver depth shadow
				const uint32_t colGMCRed        = 0x1414D8; // Vibrant GMC bold red block lettering
				const uint32_t colLED_C_Clamp   = 0xFFFFFF; // Signature glowing C-shaped DRL light ribbons
				const uint32_t colProjectorLens = 0xD0E8FA; // Stacked dual projector LED lenses
				const uint32_t colProjectorBase = 0x242830; // Dark projector headlight housing
				const uint32_t colAmberMarker   = 0x0874E8; // Amber clearance & side marker lights
				const uint32_t colDarkCharcoal  = 0x1E2024; // Lower bumper valance, wheel arch flares, bedliner
				const uint32_t colInterior      = 0x222428; // Premium Denali jet black leather dashboard
				const uint32_t colSeat          = 0x32363C; // Luxury perforated leather seating
				const uint32_t colTaillight     = 0x1414D8; // Vertical red C-shaped LED taillights

				auto SetVoxel = [&](int x, int y, int z, uint32_t col) {
					if (x >= 0 && x < w && y >= 0 && y < l && z >= 0 && z < h)
						vm->SetSolid(x, y, z, col);
				};

				// 1. Hydroformed High-Strength Steel Frame & Chassis (Z = 3..5, Y = 3..61)
				for (int y = 3; y <= 61; ++y) {
					for (int dx = -8; dx <= 8; ++dx) {
						SetVoxel(cx + dx, y, 4, 0x121416);
						if (std::abs(dx) <= 6)
							SetVoxel(cx + dx, y, 5, 0x121416);
					}
				}

				// 2. Sculpted Front Bumper & Lower Skid Plate (Y = 0..3)
				// Dark charcoal lower aerodynamic valance with center chrome skid plate
				for (int x = 2; x <= 27; ++x) {
					SetVoxel(x, 2, 5, colDarkCharcoal);
					SetVoxel(x, 1, 5, colDarkCharcoal);
					SetVoxel(x, 2, 6, colDarkCharcoal);
					SetVoxel(x, 1, 6, colSilverMetal);
					SetVoxel(x, 2, 7, colSilverMetal);
				}
				// Center Mirror-Chrome Skid Plate Shield (X = 10..19, Z = 5)
				for (int x = 10; x <= 19; ++x) {
					SetVoxel(x, 0, 5, colMirrorChrome);
					SetVoxel(x, 1, 5, colMirrorChrome);
				}
				// Dual Chrome Heavy Recovery Tow Hooks
				SetVoxel(8,  0, 5, colMirrorChrome);
				SetVoxel(21, 0, 5, colMirrorChrome);
				// Lower LED Fog Lamps
				for (int dx = 0; dx <= 2; ++dx) {
					SetVoxel(4 + dx, 1, 6, colLED_C_Clamp);
					SetVoxel(23 + dx, 1, 6, colLED_C_Clamp);
				}

				// 3. Towering Denali Mirror-Chrome Grille (Center X = 9..20, Z = 8..15)
				// Surrounded by thick mirror-chrome border
				for (int x = 9; x <= 20; ++x) {
					for (int z = 8; z <= 15; ++z) {
						bool isBorder = (x == 9 || x == 20 || z == 8 || z == 15);
						bool isChromeBar = (z % 2 == 1);
						uint32_t c = isBorder ? colMirrorChrome : (isChromeBar ? colMirrorChrome : 0x1A1C20);
						SetVoxel(x, 2, z, c);
						SetVoxel(x, 3, z, 0x141618); // Radiator background
					}
				}
				// Bold GMC Red Letters in Center (Z = 11..13, X = 11..18) with Silver Borders
				for (int z = 11; z <= 13; ++z) {
					// "G"
					SetVoxel(11, 1, z, colGMCRed);
					SetVoxel(12, 1, z, (z == 12) ? colMirrorChrome : colGMCRed);
					// "M"
					SetVoxel(14, 1, z, colGMCRed);
					SetVoxel(15, 1, z, colGMCRed);
					// "C"
					SetVoxel(17, 1, z, colGMCRed);
					SetVoxel(18, 1, z, (z == 12) ? colMirrorChrome : colGMCRed);
				}

				// 4. Iconic GMC Sierra C-Clamp LED Daytime Running Lights & Stacked Dual Projectors
				// Left Assembly (X = 3..8, Z = 8..15)
				auto DrawDenaliHeadlampUnit = [&](int xMin, int xMax, bool isLeft) {
					// Dark charcoal housing interior
					for (int x = xMin; x <= xMax; ++x) {
						for (int z = 8; z <= 15; ++z) {
							SetVoxel(x, 2, z, colProjectorBase);
						}
					}
					// Outer vertical glowing ribbon
					int outerX = isLeft ? xMin : xMax;
					for (int z = 8; z <= 15; ++z) {
						SetVoxel(outerX, 1, z, colLED_C_Clamp);
					}
					// Top horizontal brow & bottom horizontal jaw
					for (int x = xMin; x <= xMax; ++x) {
						SetVoxel(x, 1, 15, colLED_C_Clamp);
						SetVoxel(x, 1, 8, colLED_C_Clamp);
					}
					// Dual Stacked High-Output Projector LED Lenses
					int projX = isLeft ? (xMin + 2) : (xMax - 2);
					SetVoxel(projX, 1, 10, colProjectorLens);
					SetVoxel(projX + 1, 1, 10, colProjectorLens);
					SetVoxel(projX, 1, 13, colProjectorLens);
					SetVoxel(projX + 1, 1, 13, colProjectorLens);
					// Amber Clearance Marker Light at Top Outer Corner
					SetVoxel(outerX, 1, 16, colAmberMarker);
					SetVoxel(isLeft ? (outerX - 1) : (outerX + 1), 2, 15, colAmberMarker);
				};
				DrawDenaliHeadlampUnit(3, 8, true);
				DrawDenaliHeadlampUnit(21, 26, false);

				// 5. Chiseled Power-Dome Hood (Y = 4..24, Z = 14..16)
				for (int y = 4; y <= 24; ++y) {
					for (int x = 3; x <= 26; ++x) {
						bool isRidge = ((x >= 8 && x <= 10) || (x >= 19 && x <= 21)) && (y >= 6 && y <= 22);
						int hoodZ = isRidge ? 16 : 15;
						if (y <= 5) hoodZ = 15;
						uint32_t col = isRidge ? colSilverHigh : ((x == 3 || x == 26) ? colSilverShade : colSilverMetal);
						SetVoxel(x, y, hoodZ, col);
						SetVoxel(x, y, hoodZ - 1, colSilverShade);
					}
				}
				// Central Hood Scoop & Chrome Leading Edge Brow
				for (int x = 9; x <= 20; ++x) {
					SetVoxel(x, 3, 15, colMirrorChrome); // Chrome hood lip above grille
				}
				for (int sx = cx - 3; sx <= cx + 3; ++sx) {
					SetVoxel(sx, 7, 16, colDarkCharcoal); // Functional intake vent
				}

				// 6. Sculpted Fenders, Protective Wheel Arch Mouldings & Centered Wheel Cutouts
				// Front Arch: centered at Y = 12 (Y - 32 = -20.0F), range y = 7..17
				// Rear Arch: centered at Y = 49 (Y - 32 = +17.0F), range y = 44..54
				for (int y = 4; y <= 60; ++y) {
					bool inFrontArch = (y >= 7 && y <= 17);
					bool inRearArch  = (y >= 44 && y <= 54);
					int zStart = (inFrontArch || inRearArch) ? 10 : 7;

					for (int z = zStart; z <= 15; ++z) {
						uint32_t c = (z == 7) ? colSilverShade : colSilverMetal;
						SetVoxel(3, y, z, c);
						SetVoxel(4, y, z, c);
						SetVoxel(25, y, z, c);
						SetVoxel(26, y, z, c);
					}

					// Protective dark charcoal wheel arch mouldings
					if (inFrontArch || inRearArch) {
						SetVoxel(2, y, 10, colDarkCharcoal);
						SetVoxel(27, y, 10, colDarkCharcoal);
						SetVoxel(3, y, 10, colDarkCharcoal);
						SetVoxel(26, y, 10, colDarkCharcoal);
					}
				}

				// Horizontal Body Character Crease (Y = 5..60, Z = 12)
				for (int y = 5; y <= 60; ++y) {
					SetVoxel(2, y, 12, colSilverHigh);
					SetVoxel(27, y, 12, colSilverHigh);
				}

				// Chrome Tubular Running Boards / Side Steps (Y = 24..44, Z = 5)
				for (int y = 24; y <= 44; ++y) {
					SetVoxel(1, y, 5, colMirrorChrome);
					SetVoxel(2, y, 5, colMirrorChrome);
					SetVoxel(27, y, 5, colMirrorChrome);
					SetVoxel(28, y, 5, colMirrorChrome);
				}
				// Chrome Side Mirrors with Integrated Amber LED Turn Signal (Y = 25, Z = 14)
				SetVoxel(0, 25, 14, colMirrorChrome);
				SetVoxel(1, 25, 14, colAmberMarker);
				SetVoxel(28, 25, 14, colAmberMarker);
				SetVoxel(29, 25, 14, colMirrorChrome);

				// Chrome DENALI Door Badge (Y = 23, Z = 11)
				SetVoxel(2, 23, 11, colMirrorChrome);
				SetVoxel(27, 23, 11, colMirrorChrome);

				// Chrome Door Handles (Front Y = 31, Rear Y = 41)
				SetVoxel(2, 31, 13, colMirrorChrome);
				SetVoxel(27, 31, 13, colMirrorChrome);
				SetVoxel(2, 41, 13, colMirrorChrome);
				SetVoxel(27, 41, 13, colMirrorChrome);

				// 7. Crew Cab 4-Door Interior (Leather Seats, Center Console, Dashboard)
				for (int y = 25; y <= 45; ++y) {
					for (int x = 5; x <= 24; ++x) {
						SetVoxel(x, y, 7, colInterior);
					}
				}
				// Front row leather bucket seats
				for (int y = 28; y <= 33; ++y) {
					for (int x = 6; x <= 12; ++x) {
						SetVoxel(x, y, 8, colSeat);
						SetVoxel(x, 33, 9, colSeat);
						SetVoxel(x, 33, 10, colSeat);
					}
					for (int x = 17; x <= 23; ++x) {
						SetVoxel(x, y, 8, colSeat);
						SetVoxel(x, 33, 9, colSeat);
						SetVoxel(x, 33, 10, colSeat);
					}
				}
				// Rear row leather bench seat
				for (int y = 39; y <= 44; ++y) {
					for (int x = 6; x <= 23; ++x) {
						SetVoxel(x, y, 8, colSeat);
						SetVoxel(x, 44, 9, colSeat);
						SetVoxel(x, 44, 10, colSeat);
					}
				}
				// Dashboard & steering wheel
				for (int x = 5; x <= 24; ++x) {
					SetVoxel(x, 25, 12, colInterior);
					SetVoxel(x, 25, 13, colInterior);
				}
				for (int dx = -1; dx <= 1; ++dx) {
					for (int dz = -1; dz <= 1; ++dz) {
						if (std::abs(dx) == 1 || std::abs(dz) == 1)
							SetVoxel(9 + dx, 27, 13 + dz, 0x1A1A1A);
					}
				}

				// 8. Slanted A-Pillars, B-Pillars, C-Pillars & Aerodynamic Roof
				for (int step = 0; step < 5; ++step) {
					int pY = 24 + step;
					int pZ = 16 + step;
					SetVoxel(4, pY, pZ, colSilverShade);
					SetVoxel(5, pY, pZ, colSilverMetal);
					SetVoxel(24, pY, pZ, colSilverMetal);
					SetVoxel(25, pY, pZ, colSilverShade);
				}
				// Black B-Pillar divider between front and rear passenger doors
				for (int z = 15; z <= 20; ++z) {
					SetVoxel(4, 35, z, colDarkCharcoal);
					SetVoxel(25, 35, z, colDarkCharcoal);
				}
				// Solid Metallic Roof Panel with Aero Ribs (Y = 29..45, Z = 21)
				for (int y = 29; y <= 45; ++y) {
					for (int x = 4; x <= 25; ++x) {
						bool isAeroRib = (x == 10 || x == 19);
						uint32_t rc = isAeroRib ? colSilverHigh : ((x == 4 || x == 25 || y == 29 || y == 45) ? colSilverShade : colSilverMetal);
						SetVoxel(x, y, 21, rc);
					}
				}
				// Shark-Fin Antenna
				SetVoxel(cx, 44, 22, colDarkCharcoal);
				SetVoxel(cx, 45, 22, colDarkCharcoal);

				// Rear cab vertical wall (Y = 45) with open center window
				for (int x = 4; x <= 25; ++x) {
					for (int z = 8; z <= 20; ++z) {
						if (z >= 15 && z <= 19 && x >= 9 && x <= 20)
							continue; // Open center window for cabin shooting
						SetVoxel(x, 45, z, colSilverMetal);
					}
				}

				// 9. Cargo Bed with Spray-On Bedliner, MultiPro Tailgate & CornerStep Rear Bumper
				for (int y = 46; y <= 60; ++y) {
					for (int x = 5; x <= 24; ++x) {
						SetVoxel(x, y, 8, colDarkCharcoal); // Textured bedliner
					}
					// Outer bed sidewalls
					for (int z = 9; z <= 15; ++z) {
						SetVoxel(4, y, z, colSilverMetal);
						SetVoxel(5, y, z, colDarkCharcoal);
						SetVoxel(24, y, z, colDarkCharcoal);
						SetVoxel(25, y, z, colSilverMetal);
					}
				}
				// Tailgate (Y = 60) with GMC badge & chrome handle
				for (int x = 4; x <= 25; ++x) {
					for (int z = 8; z <= 15; ++z) {
						SetVoxel(x, 60, z, (z == 15) ? colSilverHigh : colSilverMetal);
					}
				}
				// Red GMC badge on Tailgate
				SetVoxel(cx - 1, 61, 12, colGMCRed);
				SetVoxel(cx,     61, 12, colGMCRed);
				SetVoxel(cx + 1, 61, 12, colGMCRed);
				SetVoxel(cx,     61, 14, colMirrorChrome); // Chrome tailgate handle

				// Signature C-Shaped LED Taillights
				for (int tz = 10; tz <= 15; ++tz) {
					SetVoxel(4, 61, tz, colTaillight);
					SetVoxel(25, 61, tz, colTaillight);
				}
				SetVoxel(5, 61, 15, colTaillight);
				SetVoxel(5, 61, 10, colTaillight);
				SetVoxel(24, 61, 15, colTaillight);
				SetVoxel(24, 61, 10, colTaillight);

				// Rear Chrome Bumper with GMC CornerSteps
				for (int x = 3; x <= 26; ++x) {
					bool isCornerStep = ((x >= 3 && x <= 5) || (x >= 24 && x <= 26));
					uint32_t bc = isCornerStep ? colDarkCharcoal : colMirrorChrome;
					SetVoxel(x, 61, 6, bc);
					SetVoxel(x, 62, 6, bc);
				}

				return renderer.CreateModel(*vm);
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// =====================================================================
		// 5. FIAT UNO TURBO I.E. RED HOT HATCHBACK (VEHICLE 5 - HD)
		// =====================================================================
		static Handle<IModel> GenerateFiatUnoTurboBody(IRenderer& renderer) {
			try {
				int const w = 26;
				int const l = 50;
				int const h = 18;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-13.0F, -25.0F, 0.0F));
				const int cx = w / 2; // 13

				// Authentic Italian Rosso Corsa & Giugiaro Styling Palette (0xBBGGRR):
				const uint32_t colFiatRed      = 0x1A20D6; // Iconic Rosso Corsa vibrant red
				const uint32_t colRedShade     = 0x1418AA; // Dark red shadow
				const uint32_t colRedHighlight = 0x2632EE; // Sunlit red highlight
				const uint32_t colBlackPlastic = 0x1E1E20; // Deep matte black bumpers & body cladding
				const uint32_t colGrilleBar    = 0xD8DDE2; // Silver 5-diagonal bars Fiat emblem
				const uint32_t colHeadlight    = 0xF0F4F8; // Rectangular halogen headlights
				const uint32_t colAmber        = 0x1080F0; // Amber front & side blinkers
				const uint32_t colFogYellow    = 0x20F0FF; // Classic yellow rally fog lamps in bumper
				const uint32_t colTurboScript  = 0x1E22D8; // Red "Turbo i.e." script on black side trim
				const uint32_t colInterior     = 0x282A2E; // Sporty dark interior
				const uint32_t colSeatPlaid    = 0x383A40; // 80s checkered sport bucket seats
				const uint32_t colTaillight    = 0x1616D0; // Rectangular ribbed rear lights
				const uint32_t colReverseLamp  = 0xE0E4E8; // White reverse light
				const uint32_t colChromeExhaust= 0xD0D6DC; // Twin sport exhaust tips

				auto SetVoxel = [&](int x, int y, int z, uint32_t col) {
					if (x >= 0 && x < w && y >= 0 && y < l && z >= 0 && z < h)
						vm->SetSolid(x, y, z, col);
				};

				// 1. Monocoque Floorpan (Z = 2..4, Y = 3..46)
				for (int y = 3; y <= 46; ++y) {
					for (int dx = -7; dx <= 7; ++dx) {
						SetVoxel(cx + dx, y, 3, 0x141416);
						if (std::abs(dx) <= 5)
							SetVoxel(cx + dx, y, 4, 0x141416);
					}
				}

				// 2. Chunky Matte Black Aerodynamic Front Bumper with Fog Lights (Y = 1..3, Z = 4..7)
				for (int x = 2; x <= 23; ++x) {
					SetVoxel(x, 1, 4, colBlackPlastic);
					SetVoxel(x, 2, 4, colBlackPlastic);
					SetVoxel(x, 1, 5, colBlackPlastic);
					SetVoxel(x, 2, 5, colBlackPlastic);
					SetVoxel(x, 1, 6, colBlackPlastic);
					SetVoxel(x, 2, 6, colBlackPlastic);
				}
				// Lower rally fog lamps in front bumper
				SetVoxel(5, 0, 4, colFogYellow);
				SetVoxel(6, 0, 4, colFogYellow);
				SetVoxel(19, 0, 4, colFogYellow);
				SetVoxel(20, 0, 4, colFogYellow);

				// 3. Slanted Front Grille with Iconic 5 DIAGONAL CHROME BARS (Y = 2..3, Z = 7..10)
				for (int x = 3; x <= 22; ++x) {
					for (int z = 7; z <= 10; ++z) {
						SetVoxel(x, 2, z, colBlackPlastic);
					}
				}
				// 5 Slanted Chrome Bars (Iconic 80s/90s Fiat Logo)
				for (int b = 0; b < 5; ++b) {
					int barX = 10 + b * 2;
					SetVoxel(barX, 1, 8, colGrilleBar);
					SetVoxel(barX + 1, 1, 9, colGrilleBar);
				}
				// "Turbo i.e." small badge on right side of grille
				SetVoxel(20, 1, 8, colTurboScript);
				SetVoxel(21, 1, 8, colTurboScript);

				// Rectangular Halogen Headlights with Corner Amber Blinkers
				for (int x = 4; x <= 7; ++x) {
					SetVoxel(x, 1, 8, colHeadlight);
					SetVoxel(x, 1, 9, colHeadlight);
				}
				SetVoxel(3, 1, 8, colAmber);
				SetVoxel(3, 1, 9, colAmber);
				for (int x = 18; x <= 21; ++x) {
					SetVoxel(x, 1, 8, colHeadlight);
					SetVoxel(x, 1, 9, colHeadlight);
				}
				SetVoxel(22, 1, 8, colAmber);
				SetVoxel(22, 1, 9, colAmber);

				// 4. Compact Wedge Hood with Cooling Slats & Monoblade Wiper (Y = 3..17, Z = 10..11)
				for (int y = 3; y <= 17; ++y) {
					for (int x = 2; x <= 23; ++x) {
						SetVoxel(x, y, 10, (x == 2 || x == 23) ? colRedShade : colFiatRed);
						SetVoxel(x, y, 9, colRedShade);
					}
				}
				// Asymmetric hood cooling louvers on driver side
				for (int lx = 5; lx <= 9; ++lx) {
					SetVoxel(lx, 10, 11, colBlackPlastic);
					SetVoxel(lx, 12, 11, colBlackPlastic);
				}
				// Single Monoblade Wiper on Cowl
				SetVoxel(13, 18, 11, 0x141416);
				SetVoxel(14, 19, 11, 0x141416);
				SetVoxel(15, 20, 11, 0x141416);

				// 5. PERFECTLY CENTERED WHEEL ARCHES & SPORT CLADDING:
				// Front Arch: centered at Y = 11 (Y - 25 = -14.0F), range y = 7..15 (9 voxels wide)
				// Rear Arch: centered at Y = 38 (Y - 25 = +13.0F), range y = 34..42 (9 voxels wide)
				for (int y = 4; y <= 45; ++y) {
					bool inFrontArch = (y >= 7 && y <= 15);
					bool inRearArch  = (y >= 34 && y <= 42);
					int zStart = (inFrontArch || inRearArch) ? 8 : 4;

					for (int z = zStart; z <= 10; ++z) {
						uint32_t c = colFiatRed;
						if (z == 4 || z == 5) c = colBlackPlastic; // Lower side skirt
						else if (z == 7) c = colBlackPlastic;      // Protective door bump strip
						else if (z == 8) c = colRedShade;
						SetVoxel(2, y, z, c);
						SetVoxel(3, y, z, c);
						SetVoxel(22, y, z, c);
						SetVoxel(23, y, z, c);
					}

					// Curved matte-black wheel-arch moulding flares
					if (inFrontArch || inRearArch) {
						SetVoxel(1, y, 8, colBlackPlastic);
						SetVoxel(24, y, 8, colBlackPlastic);
					}
				}

				// Black side mirrors on doors (Y = 19, Z = 11)
				SetVoxel(1, 19, 11, colBlackPlastic);
				SetVoxel(24, 19, 11, colBlackPlastic);

				// Red "Turbo i.e." script on lower door strips
				SetVoxel(2, 24, 6, colTurboScript);
				SetVoxel(2, 25, 6, colTurboScript);
				SetVoxel(23, 24, 6, colTurboScript);
				SetVoxel(23, 25, 6, colTurboScript);

				// 6. Sport Interior: Checkered bucket seats, dash, sport wheel
				for (int y = 18; y <= 35; ++y) {
					for (int x = 4; x <= 21; ++x) {
						SetVoxel(x, y, 5, colInterior);
					}
				}
				for (int y = 21; y <= 26; ++y) {
					for (int x = 5; x <= 10; ++x) { // Driver seat
						SetVoxel(x, y, 6, colSeatPlaid);
						SetVoxel(x, 26, 7, colSeatPlaid);
						SetVoxel(x, 26, 8, colSeatPlaid);
					}
					for (int x = 15; x <= 20; ++x) { // Passenger seat
						SetVoxel(x, y, 6, colSeatPlaid);
						SetVoxel(x, 26, 7, colSeatPlaid);
						SetVoxel(x, 26, 8, colSeatPlaid);
					}
				}
				for (int x = 4; x <= 21; ++x) {
					SetVoxel(x, 18, 9, colInterior);
					SetVoxel(x, 18, 10, colInterior);
				}
				for (int dx = -1; dx <= 1; ++dx) {
					for (int dz = -1; dz <= 1; ++dz) {
						if (std::abs(dx) == 1 || std::abs(dz) == 1)
							SetVoxel(7 + dx, 20, 10 + dz, 0x1A1A1A);
					}
				}

				// 7. Slanted A-Pillars, Open Windows & Giugiaro Hatchback Roof
				for (int step = 0; step < 4; ++step) {
					int py = 18 + step;
					int pz = 11 + step;
					SetVoxel(3, py, pz, colRedShade);
					SetVoxel(4, py, pz, colFiatRed);
					SetVoxel(21, py, pz, colFiatRed);
					SetVoxel(22, py, pz, colRedShade);
				}
				// Roof Panel (Y = 22..38, Z = 15) with sunroof outline
				for (int y = 22; y <= 38; ++y) {
					for (int x = 3; x <= 22; ++x) {
						bool isSunroof = (x >= 7 && x <= 18 && y >= 25 && y <= 31);
						SetVoxel(x, y, 15, isSunroof ? 0x222226 : ((x == 3 || x == 22 || y == 22 || y == 38) ? colRedShade : colFiatRed));
					}
				}
				// B-Pillar
				for (int z = 11; z <= 14; ++z) {
					SetVoxel(3, 28, z, colBlackPlastic);
					SetVoxel(22, 28, z, colBlackPlastic);
				}
				// C-Pillar sloping down into hatch
				for (int step = 0; step < 4; ++step) {
					int py = 39 + step;
					int pz = 14 - step;
					SetVoxel(3, py, pz, colFiatRed);
					SetVoxel(22, py, pz, colFiatRed);
				}

				// 8. Flat Tailgate, Black Roof Spoiler & Dual Exhaust
				for (int x = 3; x <= 22; ++x) {
					for (int z = 6; z <= 11; ++z) {
						SetVoxel(x, 43, z, (z >= 10) ? colBlackPlastic : colFiatRed);
					}
				}
				// Black sports spoiler integrated at the top of the rear hatch
				for (int x = 4; x <= 21; ++x) {
					SetVoxel(x, 39, 15, colBlackPlastic);
					SetVoxel(x, 40, 15, colBlackPlastic);
				}
				// Rear Vertical Ribbed Taillights
				for (int tz = 7; tz <= 10; ++tz) {
					SetVoxel(3, 44, tz, (tz == 8) ? colReverseLamp : colTaillight);
					SetVoxel(4, 44, tz, (tz == 8) ? colReverseLamp : colTaillight);
					SetVoxel(21, 44, tz, (tz == 8) ? colReverseLamp : colTaillight);
					SetVoxel(22, 44, tz, (tz == 8) ? colReverseLamp : colTaillight);
				}
				// Rear Black Plastic Bumper (Y = 44..45, Z = 4..7)
				for (int x = 2; x <= 23; ++x) {
					SetVoxel(x, 44, 4, colBlackPlastic);
					SetVoxel(x, 45, 4, colBlackPlastic);
					SetVoxel(x, 44, 5, colBlackPlastic);
					SetVoxel(x, 45, 5, colBlackPlastic);
				}
				// Dual polished chrome exhaust tips exiting on left
				SetVoxel(6, 46, 4, colChromeExhaust);
				SetVoxel(7, 46, 4, colChromeExhaust);

				return renderer.CreateModel(*vm);
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// =====================================================================
		// WHEEL MODELS FOR ALL 5 VEHICLES (HD SCULPTED)
		// =====================================================================

		// 1. Muscle Car Steelie Wheel (Chevelle)
		static Handle<IModel> GenerateChevelleWheel(IRenderer& renderer) {
			try {
				int const w = 8, l = 8, h = 8;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-4.0F, -4.0F, -4.0F));
				const uint32_t colTread    = 0x161719;
				const uint32_t colRimLip   = 0xC2C8D0;
				const uint32_t colSteelie  = 0x2A2C30;
				const uint32_t colHubcap   = 0xF0F4F8;
				const uint32_t colRimInner = 0x202224;

				for (int y = 0; y < l; ++y) {
					for (int z = 0; z < h; ++z) {
						float dy = (y - 3.5F), dz = (z - 3.5F);
						float r = sqrtf(dy * dy + dz * dz);
						if (r <= 3.8F) {
							for (int x = 1; x <= 5; ++x) {
								uint32_t c = colTread;
								if (x == 1) {
									if (r <= 1.4F) c = colHubcap;
									else if (r <= 2.8F) c = colSteelie;
									else if (r <= 3.4F) c = colRimLip;
									else c = colTread;
								} else if (x == 2) {
									c = (r <= 2.8F) ? colSteelie : colTread;
								} else {
									c = (r <= 2.4F) ? colRimInner : colTread;
								}
								vm->SetSolid(x, y, z, c);
							}
						}
					}
				}
				return renderer.CreateModel(*vm);
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// 2. Mud-Terrain Knobby Offroad Tire with Dark Rim (Toyota LC79)
		static Handle<IModel> GenerateMudTerrainWheel(IRenderer& renderer) {
			try {
				int const w = 8, l = 8, h = 8;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-4.0F, -4.0F, -4.0F));
				const uint32_t colTread    = 0x101012;
				const uint32_t colLugKnob  = 0x18181A;
				const uint32_t colGunmetal = 0x383A40;
				const uint32_t colCenter   = 0x585C64;

				for (int y = 0; y < l; ++y) {
					for (int z = 0; z < h; ++z) {
						float dy = (y - 3.5F), dz = (z - 3.5F);
						float r = sqrtf(dy * dy + dz * dz);
						if (r <= 3.9F) {
							for (int x = 1; x <= 6; ++x) {
								uint32_t c = colTread;
								if (x == 1) {
									if (r <= 1.2F) c = colCenter;
									else if (r <= 2.8F) {
										float angle = atan2f(dz, dy);
										int spoke = static_cast<int>((angle + 3.14159F) / (6.28318F / 5.0F)) % 2;
										c = (spoke == 0) ? colGunmetal : 0x1A1A1E;
									} else if (r >= 3.2F) {
										c = ((y + z) % 2 == 0) ? colLugKnob : colTread;
									}
								} else {
									c = (r <= 2.4F) ? 0x222226 : colTread;
								}
								vm->SetSolid(x, y, z, c);
							}
						}
					}
				}
				return renderer.CreateModel(*vm);
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// 3. Tactical Beadlock Military Wheel (ISV Buggy)
		static Handle<IModel> GenerateTacticalWheel(IRenderer& renderer) {
			try {
				int const w = 8, l = 8, h = 8;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-4.0F, -4.0F, -4.0F));
				const uint32_t colTread     = 0x141416;
				const uint32_t colBeadRing  = 0x282A2E;
				const uint32_t colBoltSilver= 0xD8DDE4;
				const uint32_t colCenterHub = 0x1E2022;

				for (int y = 0; y < l; ++y) {
					for (int z = 0; z < h; ++z) {
						float dy = (y - 3.5F), dz = (z - 3.5F);
						float r = sqrtf(dy * dy + dz * dz);
						if (r <= 3.8F) {
							for (int x = 1; x <= 5; ++x) {
								uint32_t c = colTread;
								if (x == 1) {
									if (r <= 1.5F) c = colCenterHub;
									else if (r <= 2.6F) c = 0x18181A;
									else if (r <= 3.3F) {
										c = ((y * 3 + z * 5) % 3 == 0) ? colBoltSilver : colBeadRing;
									}
								} else {
									c = (r <= 2.3F) ? 0x1C1C1E : colTread;
								}
								vm->SetSolid(x, y, z, c);
							}
						}
					}
				}
				return renderer.CreateModel(*vm);
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// 4. Polished Multi-Spoke Chrome Wheel (GMC Denali)
		static Handle<IModel> GenerateDenaliChromeWheel(IRenderer& renderer) {
			try {
				int const w = 8, l = 8, h = 8;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-4.0F, -4.0F, -4.0F));
				const uint32_t colTread       = 0x18181A;
				const uint32_t colChrome      = 0xF8FAFC;
				const uint32_t colChromeShade = 0xC8D0D8;
				const uint32_t colGMCBadge    = 0x1818C8;

				for (int y = 0; y < l; ++y) {
					for (int z = 0; z < h; ++z) {
						float dy = (y - 3.5F), dz = (z - 3.5F);
						float r = sqrtf(dy * dy + dz * dz);
						if (r <= 3.8F) {
							for (int x = 1; x <= 5; ++x) {
								uint32_t c = colTread;
								if (x == 1) {
									if (r <= 0.9F) c = colGMCBadge;
									else if (r <= 3.2F) {
										int spoke = static_cast<int>(atan2f(dz, dy) * 4.0F + 10.0F) % 2;
										c = (spoke == 0) ? colChrome : colChromeShade;
									} else {
										c = colTread;
									}
								} else if (x == 2) {
									c = (r <= 2.8F) ? colChromeShade : colTread;
								} else {
									c = (r <= 2.4F) ? 0x222226 : colTread;
								}
								vm->SetSolid(x, y, z, c);
							}
						}
					}
				}
				return renderer.CreateModel(*vm);
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// 5. Classic Turbine Alloy Wheel (Fiat Uno Turbo)
		static Handle<IModel> GenerateTurbineWheel(IRenderer& renderer) {
			try {
				int const w = 8, l = 8, h = 8;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-4.0F, -4.0F, -4.0F));
				const uint32_t colTread    = 0x161719;
				const uint32_t colTurbine  = 0xD8DEE4;
				const uint32_t colSlotDark = 0x26282C;
				const uint32_t colCenterCap= 0x18181A;

				for (int y = 0; y < l; ++y) {
					for (int z = 0; z < h; ++z) {
						float dy = (y - 3.5F), dz = (z - 3.5F);
						float r = sqrtf(dy * dy + dz * dz);
						if (r <= 3.7F) {
							for (int x = 1; x <= 5; ++x) {
								uint32_t c = colTread;
								if (x == 1) {
									if (r <= 1.0F) c = colCenterCap;
									else if (r <= 3.1F) {
										int fin = static_cast<int>(atan2f(dz, dy) * 6.0F + 10.0F) % 2;
										c = (fin == 0) ? colTurbine : colSlotDark;
									} else if (r <= 3.4F) {
										c = colTurbine;
									} else {
										c = colTread;
									}
								} else if (x == 2) {
									c = (r <= 2.8F) ? colTurbine : colTread;
								} else {
									c = (r <= 2.2F) ? 0x202224 : colTread;
								}
								vm->SetSolid(x, y, z, c);
							}
						}
					}
				}
				return renderer.CreateModel(*vm);
			} catch (...) {
				return Handle<IModel>();
			}
		}

		// =====================================================================
		// CENTRALIZED VEHICLE DISPATCHERS (1..5)
		// =====================================================================
		Handle<IModel> PlayerCosmetics::GetCarBodyModel(IRenderer& renderer, int carType) {
			if (carType < 1 || carType > 5) carType = 1;
			if (g_carBodyModels[carType])
				return g_carBodyModels[carType];

			Handle<IModel> m;
			switch (carType) {
				case 1: m = GenerateChevelleBody(renderer); break;
				case 2: m = GenerateToyotaLC79Body(renderer); break;
				case 3: m = GenerateMilitaryISVBody(renderer); break;
				case 4: m = GenerateGMCDenaliBody(renderer); break;
				case 5: m = GenerateFiatUnoTurboBody(renderer); break;
				default: m = GenerateChevelleBody(renderer); break;
			}
			g_carBodyModels[carType] = m;
			return m;
		}

		Handle<IModel> PlayerCosmetics::GetCarWheelModel(IRenderer& renderer, int carType) {
			if (carType < 1 || carType > 5) carType = 1;
			if (g_carWheelModels[carType])
				return g_carWheelModels[carType];

			Handle<IModel> m;
			switch (carType) {
				case 1: m = GenerateChevelleWheel(renderer); break;
				case 2: m = GenerateMudTerrainWheel(renderer); break;
				case 3: m = GenerateTacticalWheel(renderer); break;
				case 4: m = GenerateDenaliChromeWheel(renderer); break;
				case 5: m = GenerateTurbineWheel(renderer); break;
				default: m = GenerateChevelleWheel(renderer); break;
			}
			g_carWheelModels[carType] = m;
			return m;
		}

		Handle<IModel> PlayerCosmetics::GetClassicCarBodyModel(IRenderer& renderer) {
			return GetCarBodyModel(renderer, 1);
		}

		Handle<IModel> PlayerCosmetics::GetClassicCarWheelModel(IRenderer& renderer) {
			return GetCarWheelModel(renderer, 1);
		}

		Handle<IModel> PlayerCosmetics::GetHelicopterRotorModel(IRenderer& renderer) {
			if (g_heliRotorModel)
				return g_heliRotorModel;

			try {
				int const span = 70;
				int const h = 4;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(span, span, h);
				// Center at: X=35 (hub center), Y=35 (hub center), Z=0 (hub bottom)
				vm->SetOrigin(MakeVector3(-static_cast<float>(span) * 0.5F, -static_cast<float>(span) * 0.5F, 0.0F));

				const uint32_t colBladeBody   = 0x121314; // Carbon composite black
				const uint32_t colLeadingEdge = 0x60646A; // Titanium erosion strip
				const uint32_t colTipYellow   = 0x10D4F0; // High-visibility hazard yellow
				const uint32_t colTipBlack    = 0x101010; // Hazard black stripe
				const uint32_t colHubTitanium = 0x34363A; // Heavy rotor hub titanium

				int const c = span / 2; // 35

				// Heavy Rotor Mast Center Hub
				for (int dx = -3; dx <= 3; ++dx) {
					for (int dy = -3; dy <= 3; ++dy) {
						if (dx * dx + dy * dy <= 10) {
							vm->SetSolid(c + dx, c + dy, 0, colHubTitanium);
							vm->SetSolid(c + dx, c + dy, 1, colHubTitanium);
							vm->SetSolid(c + dx, c + dy, 2, colHubTitanium);
						}
					}
				}

				// 4 Aerodynamic Rotor Blades (70 voxels diameter)
				for (int r = 4; r < c; ++r) {
					uint32_t bladeCol = colBladeBody;
					if (r >= c - 6) {
						bladeCol = ((r / 2) % 2 == 0) ? colTipYellow : colTipBlack;
					}

					// Blade 1: Along +Y (Front)
					vm->SetSolid(c - 1, c + r, 1, colLeadingEdge);
					vm->SetSolid(c, c + r, 1, bladeCol);
					vm->SetSolid(c + 1, c + r, 1, bladeCol);

					// Blade 2: Along -Y (Aft)
					vm->SetSolid(c + 1, c - r, 1, colLeadingEdge);
					vm->SetSolid(c, c - r, 1, bladeCol);
					vm->SetSolid(c - 1, c - r, 1, bladeCol);

					// Blade 3: Along +X (Right)
					vm->SetSolid(c + r, c + 1, 1, colLeadingEdge);
					vm->SetSolid(c + r, c, 1, bladeCol);
					vm->SetSolid(c + r, c - 1, 1, bladeCol);

					// Blade 4: Along -X (Left)
					vm->SetSolid(c - r, c - 1, 1, colLeadingEdge);
					vm->SetSolid(c - r, c, 1, bladeCol);
					vm->SetSolid(c - r, c + 1, 1, bladeCol);
				}

				g_heliRotorModel = renderer.CreateModel(*vm);
				return g_heliRotorModel;
			} catch (...) {
				return Handle<IModel>();
			}
		}

		Handle<IModel> PlayerCosmetics::GetWarbirdFuselageModel(IRenderer& renderer, int team) {
			Handle<IModel>& cached = (team == 1) ? g_warbirdFuselageAxis : g_warbirdFuselageAllied;
			if (cached)
				return cached;

			try {
				int const w = 34;
				int const l = 36;
				int const h = 14;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-17.0F, -16.0F, -4.0F));

				const int cx = 17;
				const int cz = 4;

				// Colors (0xBBGGRR)
				const uint32_t colAlliedGreen  = 0x224824; // RAF Dark Green
				const uint32_t colAlliedGrey   = 0x484E54; // RAF Ocean Grey
				const uint32_t colAlliedBelly  = 0x90A0A8; // Sky Blue-Grey
				const uint32_t colRoundelBlue  = 0x902010; // Roundel Blue
				const uint32_t colRoundelWhite = 0xE0E0E0; // Roundel White
				const uint32_t colRoundelRed   = 0x1010D0; // Roundel Red

				const uint32_t colAxisSlate    = 0x585248; // Luftwaffe Slate
				const uint32_t colAxisMottle   = 0x324234; // RLM 71 Green
				const uint32_t colAxisBelly    = 0x989490; // Light Grey Underbelly
				const uint32_t colNoseYellow   = 0x10C0F8; // Bf 109 Bright Yellow Nose
				const uint32_t colBalkenCross  = 0x121212; // Balkenkreuz

				const uint32_t colGlass        = 0xF0D280; // Tinted Canopy Glass
				const uint32_t colFrame        = 0x141618; // Canopy Frame Mullions
				const uint32_t colPilotHelmet  = 0x182C44; // Leather Flight Helmet
				const uint32_t colGunMetal     = 0x202224; // Wing Machine Guns
				const uint32_t colExhaust      = 0x485058; // Engine Exhaust Stubs
				const uint32_t colHazardYellow = 0x10D4F0; // Prop / Wingtip Yellow

				auto SetV = [&](int x, int y, int z, uint32_t c) {
					if (x >= 0 && x < w && y >= 0 && y < l && z >= 0 && z < h)
						vm->SetSolid(x, y, z, c);
				};

				// 1. Nose & Engine Cowling (y = 0..6)
				for (int y = 0; y <= 6; ++y) {
					for (int dx = -2; dx <= 2; ++dx) {
						for (int dz = -2; dz <= 2; ++dz) {
							if (dx * dx + dz * dz <= 5) {
								uint32_t c = (team == 1) ? colNoseYellow : (((dx + y) % 3 == 0) ? colAlliedGrey : colAlliedGreen);
								SetV(cx + dx, y, cz + dz, c);
							}
						}
					}
					// Exhaust stubs on flanks
					if (y >= 3 && y <= 5) {
						SetV(cx - 3, y, cz, colExhaust);
						SetV(cx + 3, y, cz, colExhaust);
					}
				}

				// 2. Main Fuselage Monocoque (y = 7..26)
				for (int y = 7; y <= 26; ++y) {
					float taper = (y > 18) ? (y - 18) / 8.0F : 0.0F;
					int halfW = (taper > 0.5F) ? 1 : 2;
					for (int dx = -halfW; dx <= halfW; ++dx) {
						for (int dz = -1; dz <= 2; ++dz) {
							uint32_t c;
							if (dz == -1) {
								c = (team == 1) ? colAxisBelly : colAlliedBelly;
							} else {
								if (team == 1) {
									c = ((dx + y) % 4 == 0) ? colAxisMottle : colAxisSlate;
								} else {
									c = ((dx + y) % 4 == 0) ? colAlliedGrey : colAlliedGreen;
								}
							}
							SetV(cx + dx, y, cz + dz, c);
						}
					}
				}

				// 3. Cockpit Canopy & Pilot (y = 9..18, z = cz + 2 .. cz + 4)
				for (int y = 9; y <= 18; ++y) {
					for (int dx = -1; dx <= 1; ++dx) {
						bool isFrame = (y == 9 || y == 13 || y == 18 || std::abs(dx) == 1);
						uint32_t c = isFrame ? colFrame : colGlass;
						SetV(cx + dx, y, cz + 2, c);
						SetV(cx + dx, y, cz + 3, c);
					}
					SetV(cx, y, cz + 4, colFrame);
				}
				// Pilot inside cockpit
				SetV(cx, 14, cz + 2, colPilotHelmet);
				SetV(cx, 14, cz + 3, colPilotHelmet);

				// 4. Low-Mounted Cantilever Wings (x = 0..33, y = 11..21, z = cz - 1 .. cz)
				for (int x = 0; x < w; ++x) {
					int distCenter = std::abs(x - cx);
					int startY = 12 + distCenter / 6;
					int endY = 21 - distCenter / 5;
					int wingZ = cz;
					if (distCenter >= 13) wingZ += 1; // Dihedral rise at wingtips

					for (int y = startY; y <= endY; ++y) {
						uint32_t topCol;
						if (team == 1) {
							if (distCenter >= 15)
								topCol = colHazardYellow; // Yellow wingtips
							else if ((distCenter == 9 || distCenter == 10) && (y == 15 || y == 16))
								topCol = colBalkenCross; // Cross
							else
								topCol = ((x + y) % 4 == 0) ? colAxisMottle : colAxisSlate;
						} else {
							if (distCenter >= 15)
								topCol = colHazardYellow; // Yellow wingtips
							else if (distCenter == 9 && (y == 15 || y == 16))
								topCol = colRoundelBlue; // Roundel
							else
								topCol = ((x + y) % 4 == 0) ? colAlliedGrey : colAlliedGreen;
						}

						SetV(x, y, wingZ, topCol);
						SetV(x, y, wingZ - 1, (team == 1) ? colAxisBelly : colAlliedBelly);
					}
				}

				// Wing Machine Guns (Browning .303 / MG 151)
				for (int gunX : {cx - 8, cx + 8}) {
					for (int y = 7; y <= 12; ++y) {
						SetV(gunX, y, cz, colGunMetal);
					}
				}

				// 5. Tail Empennage (y = 26..35)
				// Horizontal stabilizers
				for (int dx = -6; dx <= 6; ++dx) {
					for (int y = 30; y <= 34; ++y) {
						uint32_t c = (team == 1) ? colAxisSlate : colAlliedGreen;
						SetV(cx + dx, y, cz + 1, c);
					}
				}
				// Swept Vertical Stabilizer Fin & Rudder
				for (int y = 27; y <= 35; ++y) {
					float finT = (y - 27) / 8.0F;
					int topFinZ = cz + 1 + static_cast<int>(finT * 7.0F);
					for (int z = cz + 1; z <= topFinZ; ++z) {
						uint32_t c = (y >= 33) ? colHazardYellow : ((team == 1) ? colAxisMottle : colAlliedGreen);
						SetV(cx, y, z, c);
					}
				}

				cached = renderer.CreateModel(*vm);
				return cached;
			} catch (...) {
				return Handle<IModel>();
			}
		}

		Handle<IModel> PlayerCosmetics::GetWarbirdPropellerModel(IRenderer& renderer) {
			if (g_warbirdPropModel)
				return g_warbirdPropModel;

			try {
				int const span = 20;
				int const h = 4;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(span, h, span);
				vm->SetOrigin(MakeVector3(-10.0F, 0.0F, -10.0F));

				const int cx = 10;
				const int cz = 10;
				const uint32_t colHub     = 0x141618;
				const uint32_t colBlade   = 0x18181A;
				const uint32_t colYellow  = 0x10D4F0;

				// Center bullet spinner
				for (int dx = -1; dx <= 1; ++dx) {
					for (int dz = -1; dz <= 1; ++dz) {
						vm->SetSolid(cx + dx, 0, cz + dz, colHub);
						vm->SetSolid(cx + dx, 1, cz + dz, colHub);
					}
				}
				vm->SetSolid(cx, 2, cz, colHub);

				// 3 Propeller Blades at 120-degree angles in X-Z plane
				// Blade 1: Straight UP along +Z
				for (int r = 2; r < 10; ++r) {
					uint32_t c = (r >= 7) ? colYellow : colBlade;
					vm->SetSolid(cx, 0, cz + r, c);
					vm->SetSolid(cx + 1, 0, cz + r, c);
				}
				// Blade 2: (+X, -Z)
				for (int r = 2; r < 10; ++r) {
					uint32_t c = (r >= 7) ? colYellow : colBlade;
					int bx = cx + static_cast<int>(r * 0.866F);
					int bz = cz - static_cast<int>(r * 0.500F);
					vm->SetSolid(bx, 0, bz, c);
					vm->SetSolid(bx, 0, bz + 1, c);
				}
				// Blade 3: (-X, -Z)
				for (int r = 2; r < 10; ++r) {
					uint32_t c = (r >= 7) ? colYellow : colBlade;
					int bx = cx - static_cast<int>(r * 0.866F);
					int bz = cz - static_cast<int>(r * 0.500F);
					vm->SetSolid(bx, 0, bz, c);
					vm->SetSolid(bx, 0, bz + 1, c);
				}

				g_warbirdPropModel = renderer.CreateModel(*vm);
				return g_warbirdPropModel;
			} catch (...) {
				return Handle<IModel>();
			}
		}

		void PlayerCosmetics::ClearCache() {
			g_capeCache.clear();
			g_accCache.clear();
			g_simpleToolCache.clear();
			g_heliFuselageModel = Handle<IModel>();
			g_heliRotorModel = Handle<IModel>();
			for (int i = 0; i < 6; ++i) {
				g_carBodyModels[i] = Handle<IModel>();
				g_carWheelModels[i] = Handle<IModel>();
			}
			g_warbirdFuselageAllied = Handle<IModel>();
			g_warbirdFuselageAxis = Handle<IModel>();
			g_warbirdPropModel = Handle<IModel>();
		}

		int PlayerCosmetics::GetVehicleMaxCapacity(int carType) {
			switch (carType) {
				case 2: return 4;
				case 3: return 6;
				case 4: return 6;
				case 5: return 2;
				default: return 2;
			}
		}

		void PlayerCosmetics::GetVehicleSeatOffset(int carType, int seatIdx, float& lat, float& lon, float& vert) {
			lat = 0.0F; lon = 0.0F; vert = 2.10F;
			switch (carType) {
				case 2: // Toyota Land Cruiser LC79 (2 Cab + 2 Flatbed)
					if (seatIdx == 0)      { lat = -0.88F; lon = -0.10F; vert = 2.50F; } // Seated driver
					else if (seatIdx == 1) { lat =  0.88F; lon = -0.10F; vert = 2.50F; } // Seated passenger
					else if (seatIdx == 2) { lat = -0.65F; lon = -3.35F; vert = 3.85F; } // Left standing flatbed gunner
					else if (seatIdx == 3) { lat =  0.65F; lon = -3.35F; vert = 3.85F; } // Right standing flatbed gunner
					break;
				case 3: // Military ISV Buggy (6 Tactical Seats: 3 rows of 2)
					if (seatIdx == 0)      { lat = -0.85F; lon =  0.95F; vert = 2.15F; } // Front Left (Driver)
					else if (seatIdx == 1) { lat =  0.85F; lon =  0.95F; vert = 2.15F; } // Front Right (Copilot)
					else if (seatIdx == 2) { lat = -0.85F; lon = -1.35F; vert = 2.15F; } // Mid Left
					else if (seatIdx == 3) { lat =  0.85F; lon = -1.35F; vert = 2.15F; } // Mid Right
					else if (seatIdx == 4) { lat = -0.80F; lon = -3.80F; vert = 2.25F; } // Rear Left
					else if (seatIdx == 5) { lat =  0.80F; lon = -3.80F; vert = 2.25F; } // Rear Right
					break;
				case 4: // GMC Sierra Denali Crew Cab (4 Cab + 2 Truck Bed)
					if (seatIdx == 0)      { lat = -0.92F; lon =  0.80F; vert = 2.55F; } // Front Left (Driver)
					else if (seatIdx == 1) { lat =  0.92F; lon =  0.80F; vert = 2.55F; } // Front Right (Copilot)
					else if (seatIdx == 2) { lat = -0.92F; lon = -1.60F; vert = 2.55F; } // Rear Cab Left
					else if (seatIdx == 3) { lat =  0.92F; lon = -1.60F; vert = 2.55F; } // Rear Cab Right
					else if (seatIdx == 4) { lat = -0.75F; lon = -4.40F; vert = 3.90F; } // Bed Left Standing Gunner
					else if (seatIdx == 5) { lat =  0.75F; lon = -4.40F; vert = 3.90F; } // Bed Right Standing Gunner
					break;
				case 5: // Fiat Uno Turbo (2 Seated)
					if (seatIdx == 0)      { lat = -0.68F; lon = -0.05F; vert = 2.05F; }
					else                   { lat =  0.68F; lon = -0.05F; vert = 2.05F; }
					break;
				default: // 1: Chevelle Muscle Coupe (2 Seated)
					if (seatIdx == 0)      { lat = -0.85F; lon = -0.35F; vert = 2.30F; }
					else                   { lat =  0.85F; lon = -0.35F; vert = 2.30F; }
					break;
			}
		}
	} // namespace client
} // namespace spades
