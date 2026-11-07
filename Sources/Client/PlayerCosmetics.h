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

#pragma once

#include <string>
#include <Core/RefCountedObject.h>
#include <Core/Math.h>

namespace spades {
	namespace client {
		class IRenderer;
		class IModel;

		struct HelicopterState {
			bool active = false;
			int id = 1;
			Vector3 position = MakeVector3(0.0F, 0.0F, 0.0F);
			float yaw = 0.0F;
			float pitch = 0.0F;
			float roll = 0.0F;
			int pilotId = -1;
			bool isFlying = false;
			float rotorAngle = 0.0F;
			float soundTimer = 0.0F;
		};

		/**
		 * Centralized in-game cosmetic system for ySpades.
		 * Handles procedural voxel generation, caching, and color mapping for
		 * capes, accessories (crowns, helmets), custom player suits, and vehicles.
		 */
		class PlayerCosmetics {
		public:
			/** Global helicopter vehicle state */
			static HelicopterState& GetGlobalHelicopter();
			/** Returns uniform tint color based on chosen skinId or team color fallback */
			static Vector3 GetSkinColor(const std::string& skinId, const Vector3& defaultColor);

			/** Returns (and caches) procedural 3D voxel cape model */
			static Handle<IModel> GetCapeModel(IRenderer& renderer, const std::string& capeId);

			/** Returns (and caches) procedural 3D head accessory model (crown, helmet) */
			static Handle<IModel> GetAccessoryModel(IRenderer& renderer, const std::string& accId);

			/** Returns (and caches) procedural lightweight LOD tool/weapon model for high FPS */
			static Handle<IModel> GetSimpleToolModel(IRenderer& renderer, int toolType, int weaponType);

			/** Returns (and caches) procedural 3D military helicopter fuselage model */
			static Handle<IModel> GetHelicopterFuselageModel(IRenderer& renderer);

			/** Returns (and caches) procedural 3D helicopter 4-blade rotor model */
			static Handle<IModel> GetHelicopterRotorModel(IRenderer& renderer);

			/** Clears cached cosmetic models (e.g. on renderer reset) */
			static void ClearCache();

			/** Loads and samples custom skin color from custom.png */
			static void ReloadCustomSkin();
			static Vector3 GetCustomSkinColor();
		};
	} // namespace client
} // namespace spades
