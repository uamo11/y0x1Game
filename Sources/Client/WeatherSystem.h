/*
 * Copyright (c) 2013-2026 yvt / ySpades contributors
 *
 * Dynamic Synchronized Weather & Atmospheric Simulation System
 * Supports Clear, Sunny, Fog, Rain, Storm, Sunset, and Night with
 * real-time server synchronization, volumetric sky adaptation,
 * 60 FPS rain streak rendering, dynamic fog, and 3D spatial audio.
 */

#pragma once

#include <Core/Math.h>
#include <string>
#include <vector>

namespace spades {
	namespace client {
		class Client;
		class IRenderer;

		enum class WeatherType {
			Clear = 0,    // Día Claro (deep azure, high visibility)
			Sunny = 1,    // Día Soleado (radiant sun, white cumulus)
			Fog = 2,      // Neblina / Niebla (dense pale mist, low visibility)
			Rain = 3,     // Lluvia Tranquila (overcast slate, gentle drizzle)
			Storm = 4,    // Lluvia Fuerte / Tormenta (torrential rain, lightning & thunder)
			Sunset = 5,   // Atardecer (indigo to fiery orange twilight)
			Night = 6     // Noche Estrellada (dark celestial dome, twinkling stars)
		};

		struct WeatherPreset {
			WeatherType type;
			const char* name;
			const char* displayName;
			float fogDistance;
			Vector3 fogColor;
			Vector3 zenithColor;
			float cloudCoverage;
			Vector3 cloudColor;
			float rainIntensity;
			bool hasLightning;
			float windSpeed;
		};

		struct RainDrop {
			Vector3 pos;
			Vector3 vel;
			float length;
		};

		class WeatherSystem {
		private:
			WeatherType activeType;
			WeatherType serverType;
			bool isAuto;
			bool hasServerWeather;

			// Smoothly interpolated values for fluid atmospheric transitions
			float curFogDistance;
			Vector3 curFogColor;
			Vector3 curZenithColor;
			float curCloudCoverage;
			Vector3 curCloudColor;
			float curRainIntensity;
			float curWindSpeed;

			// Lightning & Thunder Simulation
			float lightningIntensity;
			float nextLightningTimer;
			float thunderDelayTimer;
			bool thunderPending;
			int pendingThunderSound;

			// Ambient audio timers
			float rainSoundTimer;
			float windSoundTimer;

			// Precipitation Particle Simulation
			static const size_t MAX_RAINDROPS = 340;
			std::vector<RainDrop> raindrops;
			bool raindropsInitialized;

			WeatherSystem();

		public:
			static WeatherSystem& GetInstance();

			void Update(float dt, const Vector3& playerPos, Client& client);
			void RenderWeatherEffects(IRenderer& renderer, const Vector3& viewOrigin);
			void OnServerMessage(const std::string& msg);
			void ResetServerWeather();
			bool IsServerWeatherActive() const { return hasServerWeather || !isAuto; }
			void SetWeather(WeatherType type, bool userOverride = false);
			void SetWeatherByName(const std::string& name, bool userOverride = false);

			WeatherType GetActiveType() const { return activeType; }
			float GetCurrentFogDistance() const { return curFogDistance; }
			Vector3 GetCurrentFogColor() const { return curFogColor; }
			Vector3 GetCurrentZenithColor() const { return curZenithColor; }
			float GetCurrentCloudCoverage() const { return curCloudCoverage; }
			Vector3 GetCurrentCloudColor() const { return curCloudColor; }
			float GetCurrentRainIntensity() const { return curRainIntensity; }
			float GetLightningIntensity() const { return lightningIntensity; }
			bool IsAuto() const { return isAuto; }
			void SetAuto(bool a) { isAuto = a; }

			static const WeatherPreset& GetPreset(WeatherType type);
			static WeatherType ParseWeatherType(const std::string& str);
			static std::string GetWeatherName(WeatherType type);
			static std::string GetWeatherDisplayName(WeatherType type);
		};
	}
}
