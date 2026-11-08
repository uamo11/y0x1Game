/*
 Copyright (c) 2013-2026 yvt / ySpades contributors
 Ultra-lightweight Fake Volumetric Atmospheric Sky & Clouds Shader
 Dynamic Weather Compatible Edition (Solid 60 FPS Target)
 - High-speed trigonometric hash (compiled to single GPU ALU ops)
 - Smooth 2-octave continuous FBM (zero grid artifacts)
 - Analytical solar directional lighting (zero extra noise samples)
 - Dynamic weather uniforms: Zenith Color, Cloud Color, Cloud Coverage, Rain & Lightning
*/

uniform vec3 viewOrigin;
uniform vec3 fogColor;
uniform float fogDistance;
uniform float time;
uniform vec3 sunDirection;
uniform int isHDR;

// Dynamic Weather Uniforms
uniform vec3 weatherZenithColor;
uniform vec3 weatherCloudColor;
uniform float weatherCloudCoverage;
uniform float weatherRainIntensity;
uniform float weatherLightning;

varying vec2 texCoord;
varying vec3 viewDir;

// Fast hardware hash
float hash(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// 2D smooth continuous value noise with cubic Hermite curve
float noise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(
		mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
		mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),
		u.y
	);
}

// 2-Octave Continuous FBM
float fbm(vec2 p) {
	return 0.65 * noise(p) + 0.35 * noise(p * 2.25 + vec2(1.6, 3.8));
}

void main() {
	vec3 rayDir = normalize(viewDir);

	// OpenSpades coordinate system: -Z is SKY (zenith), +Z is GROUND
	float upAmount = -rayDir.z; // 1.0 at zenith, 0.0 at horizon, -1.0 looking straight down
	vec3 sunDir = normalize(sunDirection);
	float sunCos = clamp(dot(rayDir, sunDir), -1.0, 1.0);

	// Map fog luminance for day / night detection
	float fogLum = dot(fogColor, vec3(0.299, 0.587, 0.114));
	bool isNight = (fogLum < 0.20);

	// --- 1. Atmospheric Sky Gradient (Rayleigh Scattering) ---
	vec3 horizonColor = fogColor;
	vec3 zenithColor;
	if (isNight) {
		zenithColor = mix(vec3(0.015, 0.02, 0.04), fogColor * 0.35, 0.5);
	} else {
		// Deep rich celestial blue zenith fading seamlessly into warm horizon fog
		vec3 deepSkyBlue = vec3(0.12, 0.38, 0.82);
		zenithColor = mix(deepSkyBlue, fogColor * vec3(0.55, 0.75, 1.0), 0.55);
	}

	// Adapt to dynamic weather zenith color if supplied
	float weatherZenithLen = dot(weatherZenithColor, vec3(1.0));
	if (weatherZenithLen > 0.01) {
		zenithColor = mix(zenithColor, weatherZenithColor, 0.75);
	}

	float skyGrad = clamp(upAmount, 0.0, 1.0);
	float rayleighCurve = pow(skyGrad, 0.55);
	vec3 sky = mix(horizonColor, zenithColor, rayleighCurve);

	// Below horizon haze (looking down into canyons or from helicopter)
	if (upAmount < 0.0) {
		float groundGrad = clamp(-upAmount * 2.5, 0.0, 1.0);
		sky = mix(horizonColor, fogColor * 0.72, groundGrad);
	}

	// Atmospheric horizon warm haze near the sun
	float sunVisibility = clamp(1.0 - weatherRainIntensity * 0.80, 0.05, 1.0);
	if (sunCos > 0.0) {
		float horizonSunGlow = pow(sunCos, 4.0) * 0.28 * exp(-abs(upAmount) * 2.8) * sunVisibility;
		vec3 sunHazeColor = isNight ? vec3(0.3, 0.4, 0.6) : vec3(1.0, 0.85, 0.62);
		sky += sunHazeColor * horizonSunGlow;
	}

	// --- 2. Solar Core & Atmospheric Corona (Mie Scattering) ---
	if (sunCos > 0.35) {
		float coronaFade = smoothstep(0.35, 0.85, sunCos);
		float sunCorona = (pow(sunCos, 6.0) * 0.25 + pow(sunCos, 24.0) * 0.45 + pow(sunCos, 96.0) * 0.80) * coronaFade;
		float sunDisk = smoothstep(0.9984, 0.9995, sunCos);
		vec3 sunLight = isNight ? vec3(0.8, 0.9, 1.0) : vec3(1.0, 0.97, 0.92);
		sky += sunLight * (sunDisk * 3.0 + sunCorona * 0.85) * sunVisibility;
	}

	// --- 3. Night Sky Twinkling Stars (if nighttime map / weather) ---
	if (isNight && upAmount > 0.08) {
		vec2 starCoord = rayDir.xy / (abs(rayDir.z) + 0.12) * 200.0;
		float starNoise = hash(floor(starCoord));
		if (starNoise > 0.985) {
			float twinkle = 0.5 + 0.5 * sin(time * 3.0 + starNoise * 60.0);
			sky += vec3(0.85, 0.92, 1.0) * (starNoise - 0.985) * 60.0 * twinkle * smoothstep(0.08, 0.3, upAmount);
		}
	}

	// --- 4. Fake Volumetric Clouds (Zero-Cost Analytical Layers) ---
	if (upAmount > 0.02) {
		float domeScale = 1.0 / (upAmount + 0.22);
		vec2 domeCoords = rayDir.xy * domeScale;

		// Layer 1: Puffy 3D Cumulus / Storm Clouds
		vec2 wind1 = vec2(time * 0.010, time * 0.004);
		vec2 playerParallax1 = viewOrigin.xy * 0.0006;
		vec2 uv1 = domeCoords * 1.85 + playerParallax1 + wind1;

		float rawDensity1 = fbm(uv1);
		float cloudCoverage = (weatherCloudCoverage > 0.05) ? weatherCloudCoverage : 0.44;
		float density1 = smoothstep(cloudCoverage, cloudCoverage + 0.28, rawDensity1);

		if (density1 > 0.001) {
			// Zero-cost directional shading: uses sun angle and density gradient
			float sunIllumination = clamp(0.55 + 0.45 * sunCos, 0.40, 1.0);
			float selfShadow = mix(0.38, 1.0, (1.0 - density1 * 0.5) * sunIllumination);

			// Forward Mie scattering (Silver Lining on illuminated cloud rims)
			float forwardMie = pow(max(0.0, sunCos), 10.0);
			float silverLining = forwardMie * (1.0 - density1) * density1 * 2.8 * sunVisibility;

			// Cloud coloring: warm direct sunlight on top, soft atmospheric ambient underneath
			vec3 directSun = isNight ? vec3(0.5, 0.6, 0.75) : vec3(1.0, 0.98, 0.95);
			vec3 cloudAmbient = mix(fogColor * 1.05, vec3(0.42, 0.55, 0.72), 0.52);
			vec3 baseCloudColor = mix(cloudAmbient, directSun, selfShadow) + directSun * silverLining;

			// Blend with weather cloud color (dark grey for storm, golden for sunset, etc.)
			float weatherCloudLen = dot(weatherCloudColor, vec3(1.0));
			vec3 cloudColor1 = (weatherCloudLen > 0.01)
			    ? mix(baseCloudColor, weatherCloudColor * (selfShadow * 0.7 + 0.3), 0.65)
			    : baseCloudColor;

			// Distance & horizon atmospheric attenuation
			float horizonFade = smoothstep(0.02, 0.22, upAmount);
			float cloudAlpha1 = density1 * 0.94 * horizonFade;

			// Lightning flash illumination on cloud layer
			if (weatherLightning > 0.02) {
				vec3 lightningGlow = vec3(1.4, 1.6, 2.2) * weatherLightning;
				cloudColor1 += lightningGlow * 1.25;
			}

			sky = mix(sky, cloudColor1, cloudAlpha1);
		}

		// Layer 2: High-Altitude Wispy Cirrus (Single-octave, only at higher elevation in clear/sunny)
		if (upAmount > 0.12 && weatherRainIntensity < 0.3) {
			vec2 wind2 = vec2(time * 0.005, -time * 0.002);
			vec2 playerParallax2 = viewOrigin.xy * 0.00025;
			vec2 uv2 = domeCoords * 3.4 + playerParallax2 + wind2;

			float rawDensity2 = noise(uv2);
			float density2 = smoothstep(0.56, 0.82, rawDensity2) * 0.28;

			if (density2 > 0.001) {
				float horizonFade2 = smoothstep(0.12, 0.30, upAmount);
				vec3 cirrusColor = isNight ? vec3(0.6, 0.7, 0.85) : vec3(0.96, 0.98, 1.0);
				cirrusColor += cirrusColor * (pow(max(0.0, sunCos), 6.0) * 0.30);
				sky = mix(sky, cirrusColor, density2 * horizonFade2);
			}
		}
	}

	// --- 5. Lightning Flash across Sky Dome ---
	if (weatherLightning > 0.02) {
		vec3 skyLightning = vec3(1.4, 1.6, 2.2) * weatherLightning;
		sky += skyLightning * 0.55;
	}

	gl_FragColor = vec4(sky, 1.0);
}
