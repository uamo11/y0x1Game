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

				// Canopy glass & mullions
				for (int y = 12; y <= 38; ++y) {
					int topZ = 13 + static_cast<int>((y - 12) * 0.28F);
					if (topZ > 20) topZ = 20;

					for (int dx = -4; dx <= 4; ++dx) {
						bool isFrame = (std::abs(dx) == 4 || y == 12 || y == 22 || y == 34 || y == 38);
						uint32_t glassCol = isFrame ? colGlassFrame : colCockpitGlass;
						SetVoxel(cx + dx, y, topZ, glassCol);
					}
					for (int z = 11; z < topZ; ++z) {
						bool isSideFrame = (y == 12 || y == 22 || y == 34 || y == 38 || z == 11);
						uint32_t sideCol = isSideFrame ? colGlassFrame : colCockpitGlass;
						SetVoxel(cx - 4, y, z, sideCol);
						SetVoxel(cx + 4, y, z, sideCol);
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

		void PlayerCosmetics::ClearCache() {
			g_capeCache.clear();
			g_accCache.clear();
			g_simpleToolCache.clear();
			g_heliFuselageModel = Handle<IModel>();
			g_heliRotorModel = Handle<IModel>();
		}
	} // namespace client
} // namespace spades
