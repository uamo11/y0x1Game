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
			Vector3 targetPosition = MakeVector3(0.0F, 0.0F, 0.0F);
			Vector3 velocity = MakeVector3(0.0F, 0.0F, 0.0F);
			float yaw = 0.0F;
			float targetYaw = 0.0F;
			float pitch = 0.0F;
			float targetPitch = 0.0F;
			float roll = 0.0F;
			float targetRoll = 0.0F;
			int pilotId = -1;
			int pass1Id = -1;
			int pass2Id = -1;
			bool isFlying = false;
			float rotorAngle = 0.0F;
			float rotorSpeed = 0.0F;
			float soundTimer = 0.0F;
			int health = 300;
			int maxHealth = 300;
			int healthTier = 3;
		};

		struct WarbirdPlane {
			bool active = false;
			int team = 0; // 0 = Allied (Spitfire/Mustang), 1 = Axis (Bf 109)
			Vector3 position = MakeVector3(0.0F, 0.0F, 0.0F);
			Vector3 velocity = MakeVector3(0.0F, 0.0F, 0.0F);
			float yaw = 0.0F;
			float pitch = 0.0F;
			float roll = 0.0F;
			float propAngle = 0.0F;
			float speed = 40.0F;
			float health = 100.0F;
			int state = 0; // 0: Cruise, 1: Chase, 2: Evade, 3: Dive Flyby, 4: Spiral Death, 5: Respawn
			float stateTimer = 0.0F;
			float burstTimer = 0.0F;
			float engineSoundTimer = 0.0F;
			float smokeTimer = 0.0F;
			float flybySoundTimer = 0.0F;
		};

		struct AirBattleState {
			bool active = false;
			WarbirdPlane planes[2];
			float globalTimer = 0.0F;
			int hunterIndex = 0;
			float roleSwitchTimer = 14.0F;
			float flybyTimer = 18.0F;
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

			/** Global WWII Air Dogfight state */
			static AirBattleState& GetGlobalAirBattle();

			/** Returns (and caches) procedural 3D WWII warbird fighter plane model */
			static Handle<IModel> GetWarbirdFuselageModel(IRenderer& renderer, int team);

			/** Returns (and caches) procedural 3D WWII warbird 3-blade propeller model */
			static Handle<IModel> GetWarbirdPropellerModel(IRenderer& renderer);
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
