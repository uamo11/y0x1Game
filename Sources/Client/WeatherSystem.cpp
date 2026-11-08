/*
 * Copyright (c) 2013-2026 yvt / ySpades contributors
 *
 * Dynamic Synchronized Weather & Atmospheric Simulation System
 */

#include "WeatherSystem.h"
#include "Client.h"
#include "IRenderer.h"
#include "IAudioDevice.h"
#include "IAudioChunk.h"
#include <Core/Settings.h>
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace spades {
	namespace client {

		SPADES_SETTING(cg_weather);
		SPADES_SETTING(cg_weatherRain);
		SPADES_SETTING(cg_weatherLightning);
		SPADES_SETTING(cg_weatherAudio);
		SPADES_SETTING(cg_weatherFogDistance);

		static const WeatherPreset s_presets[] = {
			// Clear (Día Claro)
			{
				WeatherType::Clear,
				"clear",
				"Día Claro",
				240.0F,
				MakeVector3(0.55F, 0.75F, 0.95F),
				MakeVector3(0.10F, 0.35F, 0.85F),
				0.20F,
				MakeVector3(1.0F, 1.0F, 1.0F),
				0.0F,
				false,
				0.5F
			},
			// Sunny (Día Soleado)
			{
				WeatherType::Sunny,
				"sunny",
				"Día Soleado",
				180.0F,
				MakeVector3(0.60F, 0.78F, 0.98F),
				MakeVector3(0.18F, 0.46F, 0.88F),
				0.44F,
				MakeVector3(1.0F, 0.98F, 0.95F),
				0.0F,
				false,
				0.8F
			},
			// Fog (Neblina / Niebla)
			{
				WeatherType::Fog,
				"fog",
				"Neblina",
				45.0F,
				MakeVector3(0.72F, 0.75F, 0.78F),
				MakeVector3(0.65F, 0.68F, 0.72F),
				0.85F,
				MakeVector3(0.70F, 0.72F, 0.75F),
				0.0F,
				false,
				0.3F
			},
			// Rain (Lluvia Tranquila)
			{
				WeatherType::Rain,
				"rain",
				"Lluvia Tranquila",
				88.0F,
				MakeVector3(0.42F, 0.47F, 0.53F),
				MakeVector3(0.28F, 0.32F, 0.40F),
				0.78F,
				MakeVector3(0.38F, 0.42F, 0.48F),
				0.45F,
				false,
				1.2F
			},
			// Storm (Lluvia Fuerte / Tormenta)
			{
				WeatherType::Storm,
				"storm",
				"Lluvia Fuerte",
				56.0F,
				MakeVector3(0.24F, 0.26F, 0.32F),
				MakeVector3(0.14F, 0.16F, 0.22F),
				0.94F,
				MakeVector3(0.20F, 0.22F, 0.26F),
				1.0F,
				true,
				2.8F
			},
			// Sunset (Atardecer)
			{
				WeatherType::Sunset,
				"sunset",
				"Atardecer",
				160.0F,
				MakeVector3(0.85F, 0.42F, 0.22F),
				MakeVector3(0.20F, 0.14F, 0.38F),
				0.48F,
				MakeVector3(0.95F, 0.55F, 0.30F),
				0.0F,
				false,
				0.6F
			},
			// Night (Noche Estrellada)
			{
				WeatherType::Night,
				"night",
				"Noche Estrellada",
				110.0F,
				MakeVector3(0.04F, 0.05F, 0.09F),
				MakeVector3(0.015F, 0.02F, 0.04F),
				0.30F,
				MakeVector3(0.12F, 0.14F, 0.18F),
				0.0F,
				false,
				0.4F
			}
		};

		WeatherSystem::WeatherSystem()
		    : activeType(WeatherType::Sunny),
		      serverType(WeatherType::Sunny),
		      isAuto(true),
		      curFogDistance(180.0F),
		      curFogColor(MakeVector3(0.60F, 0.78F, 0.98F)),
		      curZenithColor(MakeVector3(0.18F, 0.46F, 0.88F)),
		      curCloudCoverage(0.44F),
		      curCloudColor(MakeVector3(1.0F, 0.98F, 0.95F)),
		      curRainIntensity(0.0F),
		      curWindSpeed(0.8F),
		      lightningIntensity(0.0F),
		      nextLightningTimer(6.0F),
		      thunderDelayTimer(0.0F),
		      thunderPending(false),
		      pendingThunderSound(0),
		      rainSoundTimer(0.5F),
		      windSoundTimer(1.0F),
		      raindropsInitialized(false) {}

		WeatherSystem& WeatherSystem::GetInstance() {
			static WeatherSystem instance;
			return instance;
		}

		const WeatherPreset& WeatherSystem::GetPreset(WeatherType type) {
			int idx = static_cast<int>(type);
			if (idx >= 0 && idx < 7) {
				return s_presets[idx];
			}
			return s_presets[1]; // default Sunny
		}

		WeatherType WeatherSystem::ParseWeatherType(const std::string& str) {
			std::string lower;
			for (char c : str) lower += static_cast<char>(tolower(c));

			if (lower == "clear" || lower == "claro" || lower == "despejado") return WeatherType::Clear;
			if (lower == "sunny" || lower == "soleado" || lower == "sol" || lower == "dia") return WeatherType::Sunny;
			if (lower == "fog" || lower == "neblina" || lower == "niebla" || lower == "mist") return WeatherType::Fog;
			if (lower == "rain" || lower == "lluvia" || lower == "llovizna" || lower == "drizzle" || lower == "tranquila") return WeatherType::Rain;
			if (lower == "storm" || lower == "tormenta" || lower == "fuerte" || lower == "temporal") return WeatherType::Storm;
			if (lower == "sunset" || lower == "atardecer" || lower == "crepusculo" || lower == "tarde") return WeatherType::Sunset;
			if (lower == "night" || lower == "noche" || lower == "oscuro") return WeatherType::Night;

			return WeatherType::Sunny;
		}

		std::string WeatherSystem::GetWeatherName(WeatherType type) {
			return GetPreset(type).name;
		}

		std::string WeatherSystem::GetWeatherDisplayName(WeatherType type) {
			return GetPreset(type).displayName;
		}

		void WeatherSystem::SetWeather(WeatherType type, bool userOverride) {
			activeType = type;
			if (userOverride) {
				isAuto = false;
			}
		}

		void WeatherSystem::SetWeatherByName(const std::string& name, bool userOverride) {
			std::string lower;
			for (char c : name) lower += static_cast<char>(tolower(c));
			if (lower == "auto") {
				isAuto = true;
				activeType = serverType;
				return;
			}
			SetWeather(ParseWeatherType(name), userOverride);
		}

		void WeatherSystem::OnServerMessage(const std::string& msg) {
			size_t tagPos = msg.find("[WEATHER]");
			if (tagPos == std::string::npos)
				return;

			std::string token = msg.substr(tagPos + 9);
			// Trim leading spaces or "type="
			size_t start = token.find_first_not_of(" =");
			if (start != std::string::npos) {
				token = token.substr(start);
			}
			if (token.substr(0, 5) == "type=") {
				token = token.substr(5);
			}
			size_t end = token.find_first_of(" \r\n]");
			if (end != std::string::npos) {
				token = token.substr(0, end);
			}

			serverType = ParseWeatherType(token);
			if (isAuto) {
				activeType = serverType;
			}
		}

		void WeatherSystem::Update(float dt, const Vector3& playerPos, Client& client) {
			// Check CVar override
			std::string cvarVal = (std::string)cg_weather;
			if (!cvarVal.empty() && cvarVal != "auto") {
				activeType = ParseWeatherType(cvarVal);
				isAuto = false;
			} else {
				isAuto = true;
				activeType = serverType;
			}

			const WeatherPreset& target = GetPreset(activeType);

			// Smooth fluid atmospheric transitions (over ~2.5 - 3.5 seconds)
			float blendSpeed = std::min(1.0F, 1.4F * dt);
			curFogDistance += (target.fogDistance - curFogDistance) * blendSpeed;
			curFogColor += (target.fogColor - curFogColor) * blendSpeed;
			curZenithColor += (target.zenithColor - curZenithColor) * blendSpeed;
			curCloudCoverage += (target.cloudCoverage - curCloudCoverage) * blendSpeed;
			curCloudColor += (target.cloudColor - curCloudColor) * blendSpeed;
			curRainIntensity += (target.rainIntensity - curRainIntensity) * blendSpeed;
			curWindSpeed += (target.windSpeed - curWindSpeed) * blendSpeed;

			// Initialize Raindrops
			if (!raindropsInitialized) {
				raindrops.resize(MAX_RAINDROPS);
				for (auto& drop : raindrops) {
					drop.pos.x = playerPos.x + static_cast<float>((rand() % 360) - 180) * 0.1F;
					drop.pos.y = playerPos.y + static_cast<float>((rand() % 360) - 180) * 0.1F;
					drop.pos.z = playerPos.z - 12.0F + static_cast<float>((rand() % 200)) * 0.1F;
					drop.vel.x = curWindSpeed * 4.5F + static_cast<float>((rand() % 40) - 20) * 0.05F;
					drop.vel.y = curWindSpeed * 2.5F + static_cast<float>((rand() % 40) - 20) * 0.05F;
					drop.vel.z = 36.0F + static_cast<float>(rand() % 80) * 0.1F;
					drop.length = 0.8F + static_cast<float>(rand() % 50) * 0.01F;
				}
				raindropsInitialized = true;
			}

			// Update Precipitation Simulation
			if (curRainIntensity > 0.02F && cg_weatherRain) {
				for (auto& drop : raindrops) {
					drop.pos.x += drop.vel.x * dt;
					drop.pos.y += drop.vel.y * dt;
					drop.pos.z += drop.vel.z * dt;

					// Relative boundary wrap around player cylinder
					float dx = drop.pos.x - playerPos.x;
					float dy = drop.pos.y - playerPos.y;
					float distSq = dx * dx + dy * dy;

					if (drop.pos.z > playerPos.z + 7.0F || distSq > (22.0F * 22.0F)) {
						drop.pos.x = playerPos.x + static_cast<float>((rand() % 360) - 180) * 0.1F;
						drop.pos.y = playerPos.y + static_cast<float>((rand() % 360) - 180) * 0.1F;
						drop.pos.z = playerPos.z - 13.0F - static_cast<float>(rand() % 40) * 0.1F;
						drop.vel.x = curWindSpeed * 4.5F + static_cast<float>((rand() % 40) - 20) * 0.05F;
						drop.vel.y = curWindSpeed * 2.5F + static_cast<float>((rand() % 40) - 20) * 0.05F;
						drop.vel.z = 36.0F + static_cast<float>(rand() % 80) * 0.1F;
					}
				}
			}

			// Lightning & Thunder Simulation (Active in Storms)
			if (target.hasLightning && cg_weatherLightning) {
				nextLightningTimer -= dt;
				if (nextLightningTimer <= 0.0F) {
					// Strike!
					lightningIntensity = 1.95F + static_cast<float>(rand() % 100) * 0.01F;
					nextLightningTimer = 6.5F + static_cast<float>(rand() % 120) * 0.1F;
					thunderPending = true;
					thunderDelayTimer = 0.28F + static_cast<float>(rand() % 80) * 0.01F;
					pendingThunderSound = rand() % 3;
				}
			}

			// Exponential decay of lightning flash
			if (lightningIntensity > 0.0F) {
				lightningIntensity = std::max(0.0F, lightningIntensity - 4.2F * dt);
			}

			// Delayed Thunderclap (Speed of Sound)
			if (thunderPending) {
				thunderDelayTimer -= dt;
				if (thunderDelayTimer <= 0.0F) {
					thunderPending = false;
					if (cg_weatherAudio && !client.IsMuted()) {
						static const char* const thunderSounds[3] = {
							"Sounds/Weather/Thunder1.wav",
							"Sounds/Weather/Thunder2.wav",
							"Sounds/Weather/Thunder3.wav"
						};
						Handle<IAudioChunk> th = client.GetAudioDevice().RegisterSound(thunderSounds[pendingThunderSound]);
						if (th) {
							AudioParam tp;
							tp.volume = 4.2F;
							client.GetAudioDevice().PlayLocal(th.GetPointerOrNull(), tp);
						}
					}
				}
			}

			// Ambient Rain & Wind Audio Loops
			if (cg_weatherAudio && !client.IsMuted()) {
				if (curRainIntensity > 0.06F) {
					rainSoundTimer -= dt;
					if (rainSoundTimer <= 0.0F) {
						rainSoundTimer = 3.8F;
						const char* rSnd = (curRainIntensity > 0.65F)
						    ? "Sounds/Weather/RainStorm.wav"
						    : "Sounds/Weather/RainGentle.wav";
						Handle<IAudioChunk> rChunk = client.GetAudioDevice().RegisterSound(rSnd);
						if (rChunk) {
							AudioParam rp;
							rp.volume = std::min(1.4F, curRainIntensity * 1.5F);
							client.GetAudioDevice().PlayLocal(rChunk.GetPointerOrNull(), rp);
						}
					}
				}

				if (activeType == WeatherType::Fog) {
					windSoundTimer -= dt;
					if (windSoundTimer <= 0.0F) {
						windSoundTimer = 3.8F;
						Handle<IAudioChunk> wChunk = client.GetAudioDevice().RegisterSound("Sounds/Weather/WindBreeze.wav");
						if (wChunk) {
							AudioParam wp;
							wp.volume = 1.1F;
							client.GetAudioDevice().PlayLocal(wChunk.GetPointerOrNull(), wp);
						}
					}
				}
			}
		}

		void WeatherSystem::RenderWeatherEffects(IRenderer& renderer, const Vector3& viewOrigin) {
			if (curRainIntensity <= 0.02F || !cg_weatherRain)
				return;

			float alpha = std::min(0.55F, curRainIntensity * 0.42F);
			Vector4 dropCol = MakeVector4(0.72F, 0.82F, 0.95F, alpha);

			for (const auto& drop : raindrops) {
				Vector3 p1 = drop.pos;
				Vector3 p2 = drop.pos - drop.vel * (0.028F * drop.length);
				renderer.AddDebugLine(p1, p2, dropCol);
			}
		}

	}
}
