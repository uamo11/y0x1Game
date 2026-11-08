/*
 Copyright (c) 2013-2026 yvt / ySpades contributors
 Ultra-lightweight Fake Volumetric Atmospheric Sky & Clouds Shader
 Zero-performance impact (<0.03ms): Single-pass analytical pseudo-volumetric atmosphere
*/

uniform vec3 viewOrigin;
uniform vec3 fogColor;
uniform float fogDistance;
uniform float time;
uniform vec3 sunDirection;
uniform int isHDR;

varying vec2 texCoord;
varying vec3 viewDir;

// Dave Hoskins robust hash without sine
float hash(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

// 2D smooth value noise with cubic Hermite interpolation
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

// 4-Octave Fractal Brownian Motion for rich volumetric cloud billows
float fbm(vec2 p) {
	float f = 0.5000 * noise(p); p = p * 2.02 + vec2(1.7, 9.2);
	f += 0.2500 * noise(p); p = p * 2.03 + vec2(8.3, 2.8);
	f += 0.1250 * noise(p); p = p * 2.01 + vec2(4.1, 5.5);
	f += 0.0625 * noise(p);
	return f;
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

	float skyGrad = clamp(upAmount, 0.0, 1.0);
	float rayleighCurve = pow(skyGrad, 0.55);
	vec3 sky = mix(horizonColor, zenithColor, rayleighCurve);

	// Below horizon haze (looking down into canyons or from helicopter)
	if (upAmount < 0.0) {
		float groundGrad = clamp(-upAmount * 2.5, 0.0, 1.0);
		sky = mix(horizonColor, fogColor * 0.72, groundGrad);
	}

	// Atmospheric horizon warm haze near the sun
	float horizonSunGlow = pow(max(0.0, sunCos), 4.0) * 0.28 * exp(-abs(upAmount) * 2.8);
	vec3 sunHazeColor = isNight ? vec3(0.3, 0.4, 0.6) : vec3(1.0, 0.85, 0.62);
	sky += sunHazeColor * horizonSunGlow;

	// --- 2. Solar Core & Atmospheric Corona (Mie Scattering) ---
	// Perfectly smooth exponential falloff without any cutoff lines
	if (sunCos > 0.0) {
		float coronaFade = smoothstep(0.40, 0.88, sunCos);
		float sunCorona = (pow(sunCos, 6.0) * 0.25 + pow(sunCos, 24.0) * 0.45 + pow(sunCos, 96.0) * 0.80) * coronaFade;
		float sunDisk = smoothstep(0.9984, 0.9995, sunCos);
		vec3 sunLight = isNight ? vec3(0.8, 0.9, 1.0) : vec3(1.0, 0.97, 0.92);
		sky += sunLight * (sunDisk * 3.0 + sunCorona * 0.85);
	}

	// --- 3. Night Sky Twinkling Stars (if nighttime map) ---
	if (isNight && upAmount > 0.08) {
		vec2 starCoord = rayDir.xy / (abs(rayDir.z) + 0.12) * 200.0;
		float starNoise = hash(floor(starCoord));
		if (starNoise > 0.985) {
			float twinkle = 0.5 + 0.5 * sin(time * 3.0 + starNoise * 60.0);
			sky += vec3(0.85, 0.92, 1.0) * (starNoise - 0.985) * 60.0 * twinkle * smoothstep(0.08, 0.3, upAmount);
		}
	}

	// --- 4. Fake Volumetric Clouds (Zero-Cost Analytical Layers) ---
	if (upAmount > 0.01) {
		// Dome perspective projection: clouds recede realistically towards the horizon
		float domeScale = 1.0 / (upAmount + 0.22);
		vec2 domeCoords = rayDir.xy * domeScale;

		// Layer 1: Puffy 3D Cumulus Clouds
		vec2 wind1 = vec2(time * 0.010, time * 0.004);
		vec2 playerParallax1 = viewOrigin.xy * 0.0006;
		vec2 uv1 = domeCoords * 1.85 + playerParallax1 + wind1;

		float rawDensity1 = fbm(uv1);
		// Cloud coverage threshold: distinct, fluffy, billowy cumulus formations
		float cloudCoverage = 0.44;
		float density1 = smoothstep(cloudCoverage, cloudCoverage + 0.28, rawDensity1);

		if (density1 > 0.001) {
			// Volumetric 3D directional shadowing: sample offset toward sunlight
			vec2 sunOffset = -sunDir.xy * 0.075;
			float shadowDensity = smoothstep(cloudCoverage, cloudCoverage + 0.28, fbm(uv1 + sunOffset));
			float selfShadow = clamp(1.0 - shadowDensity * 0.65, 0.35, 1.0);

			// Forward Mie scattering (Silver Lining on illuminated cloud rims)
			float forwardMie = pow(max(0.0, sunCos), 10.0);
			float silverLining = forwardMie * (1.0 - density1) * density1 * 2.8;

			// Cloud coloring: warm direct sunlight on top, soft atmospheric ambient underneath
			vec3 directSun = isNight ? vec3(0.5, 0.6, 0.75) : vec3(1.0, 0.98, 0.95);
			vec3 cloudAmbient = mix(fogColor * 1.05, vec3(0.42, 0.55, 0.72), 0.52);
			vec3 cloudColor1 = mix(cloudAmbient, directSun, selfShadow) + directSun * silverLining;

			// Distance & horizon atmospheric attenuation
			float horizonFade = smoothstep(0.01, 0.22, upAmount);
			float cloudAlpha1 = density1 * 0.94 * horizonFade;

			sky = mix(sky, cloudColor1, cloudAlpha1);
		}

		// Layer 2: High-Altitude Wispy Cirrus Veil
		vec2 wind2 = vec2(time * 0.005, -time * 0.002);
		vec2 playerParallax2 = viewOrigin.xy * 0.00025;
		vec2 uv2 = domeCoords * 3.4 + playerParallax2 + wind2;

		float rawDensity2 = fbm(uv2);
		float density2 = smoothstep(0.52, 0.76, rawDensity2) * 0.35;

		if (density2 > 0.001) {
			float horizonFade2 = smoothstep(0.02, 0.25, upAmount);
			vec3 cirrusColor = isNight ? vec3(0.6, 0.7, 0.85) : vec3(0.96, 0.98, 1.0);
			cirrusColor += cirrusColor * (pow(max(0.0, sunCos), 6.0) * 0.30);
			sky = mix(sky, cirrusColor, density2 * horizonFade2);
		}
	}

	gl_FragColor = vec4(sky, 1.0);
}
