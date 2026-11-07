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

		Handle<IModel> PlayerCosmetics::GetHelicopterFuselageModel(IRenderer& renderer) {
			if (g_heliFuselageModel)
				return g_heliFuselageModel;

			try {
				int const w = 18;
				int const l = 42;
				int const h = 18;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(w, l, h);
				vm->SetOrigin(MakeVector3(-static_cast<float>(w) * 0.5F, -14.0F, -static_cast<float>(h) * 0.5F));

				// Little-endian RGB: 0xBBGGRR
				const uint32_t colHull     = 0x244028; // Camouflage olive green
				const uint32_t colHullDark = 0x1A301E; // Shaded green
				const uint32_t colSkid     = 0x18181A; // Heavy steel black
				const uint32_t colGlass    = 0xF0C070; // Tinted cockpit glass cyan
				const uint32_t colLight    = 0x80FFFF; // Searchlight / radar
				const uint32_t colTailWarn = 0x1818EE; // Tail hazard red

				// 1. Landing Skids (z = 0, 1)
				for (int y = 4; y <= 24; ++y) {
					vm->SetSolid(2, y, 0, colSkid);
					vm->SetSolid(w - 3, y, 0, colSkid);
				}
				// Skid struts
				for (int z = 1; z <= 2; ++z) {
					for (int y : {8, 20}) {
						vm->SetSolid(3, y, z, colSkid);
						vm->SetSolid(w - 4, y, z, colSkid);
					}
				}

				// 2. Cockpit & Cabin Floor (z = 2, 3)
				for (int x = 4; x < w - 4; ++x) {
					for (int y = 6; y <= 26; ++y) {
						vm->SetSolid(x, y, 3, colHullDark);
					}
				}

				// 3. Cabin Walls & Roof (z = 4 to 10)
				for (int z = 4; z <= 10; ++z) {
					for (int y = 8; y <= 26; ++y) {
						vm->SetSolid(4, y, z, colHull);
						vm->SetSolid(w - 5, y, z, colHull);
					}
				}
				// Roof
				for (int x = 4; x < w - 4; ++x) {
					for (int y = 8; y <= 26; ++y) {
						vm->SetSolid(x, y, 10, colHull);
					}
				}

				// 4. Cockpit Windshield (front canopy, y = 4 to 8, z = 4 to 8)
				for (int x = 5; x < w - 5; ++x) {
					vm->SetSolid(x, 4, 4, colLight); // Nose radar beacon
					vm->SetSolid(x, 5, 5, colGlass);
					vm->SetSolid(x, 6, 6, colGlass);
					vm->SetSolid(x, 7, 7, colGlass);
				}

				// 5. Tail Boom (y = 26 to 40, centered x, z = 7, 8)
				int const midX = w / 2;
				for (int y = 26; y < l - 2; ++y) {
					vm->SetSolid(midX - 1, y, 7, colHull);
					vm->SetSolid(midX, y, 7, colHull);
					vm->SetSolid(midX - 1, y, 8, colHull);
					vm->SetSolid(midX, y, 8, colHull);
				}

				// 6. Vertical Stabilizer Fin & Tail Rotor (y = l - 3, l - 2, z = 7 to 14)
				for (int z = 7; z <= 14; ++z) {
					vm->SetSolid(midX, l - 3, z, colHull);
				}
				// Tail hazard beacon & tail rotor
				vm->SetSolid(midX, l - 3, 14, colTailWarn);
				vm->SetSolid(midX + 1, l - 3, 12, colSkid);
				vm->SetSolid(midX + 1, l - 3, 13, colTailWarn);
				vm->SetSolid(midX + 1, l - 3, 11, colTailWarn);

				// 7. Main Rotor Mast (z = 11, 12)
				vm->SetSolid(midX - 1, 16, 11, colSkid);
				vm->SetSolid(midX, 16, 11, colSkid);
				vm->SetSolid(midX - 1, 16, 12, colSkid);
				vm->SetSolid(midX, 16, 12, colSkid);

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
				int const span = 46;
				int const h = 3;
				Handle<VoxelModel> vm = Handle<VoxelModel>::New(span, span, h);
				vm->SetOrigin(MakeVector3(-static_cast<float>(span) * 0.5F, -static_cast<float>(span) * 0.5F, -1.0F));

				const uint32_t colBlade = 0x18181A; // Carbon blade iron black
				const uint32_t colTip   = 0x10E0FF; // Safety hazard yellow
				const uint32_t colHub   = 0x303035; // Center titanium hub

				int const c = span / 2;

				// Center hub
				for (int dx = -2; dx <= 2; ++dx) {
					for (int dy = -2; dy <= 2; ++dy) {
						vm->SetSolid(c + dx, c + dy, 1, colHub);
					}
				}

				// 4 Aerodynamic Blades with Safety Yellow Tips
				for (int r = 3; r < c - 1; ++r) {
					uint32_t col = (r >= c - 4) ? colTip : colBlade;
					// Blade +Y
					vm->SetSolid(c - 1, c + r, 1, col);
					vm->SetSolid(c, c + r, 1, col);
					// Blade -Y
					vm->SetSolid(c - 1, c - r, 1, col);
					vm->SetSolid(c, c - r, 1, col);
					// Blade +X
					vm->SetSolid(c + r, c - 1, 1, col);
					vm->SetSolid(c + r, c, 1, col);
					// Blade -X
					vm->SetSolid(c - r, c - 1, 1, col);
					vm->SetSolid(c - r, c, 1, col);
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
