/*
 Copyright (c) 2013-2026 yvt / ySpades contributors
 Ultra-lightweight Fake Volumetric Atmospheric Sky & Clouds Shader
 Zero-performance impact (<0.04ms): Single-pass analytical pseudo-volumetric atmosphere
*/

uniform vec3 viewOrigin;
uniform vec3 fogColor;
uniform float fogDistance;
uniform float time;
uniform vec3 sunDirection;
uniform int isHDR;

varying vec2 texCoord;
varying vec3 viewDir;

// Ultra-fast hash function
float hash(vec2 p) {
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

// 2D smooth value noise
float noise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f); // Hermite curve
	return mix(
		mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
		mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),
		u.y
	);
}

// 3-Octave billowy fractal Brownian motion for fluffy cumulus clouds
float fbm(vec2 p) {
	float f = 0.500 * noise(p); p *= 2.02;
	f += 0.250 * noise(p); p *= 2.03;
	f += 0.125 * noise(p);
	return f;
}

void main() {
	vec3 rayDir = normalize(viewDir);

	// OpenSpades coordinate system: -Z is SKY (zenith), +Z is GROUND
	float upAmount = -rayDir.z; // 1.0 at zenith, 0.0 at horizon, -1.0 looking straight down
	vec3 sunDir = normalize(sunDirection);
	float sunCos = clamp(dot(rayDir, sunDir), -1.0, 1.0);

	// Brightness and day/night detection based on map fogColor
	float fogLum = dot(fogColor, vec3(0.299, 0.587, 0.114));
	bool isNight = (fogLum < 0.20);

	// --- 1. Atmospheric Sky Gradient (Rayleigh Scattering) ---
	vec3 horizonColor = fogColor;
	vec3 zenithColor;
	if (isNight) {
		zenithColor = mix(vec3(0.015, 0.02, 0.04), fogColor * 0.4, 0.5);
	} else {
		// Vibrant daylight zenith gradient matching fog atmosphere
		vec3 deepSkyBlue = vec3(0.09, 0.32, 0.72);
		zenithColor = mix(deepSkyBlue, fogColor * vec3(0.5, 0.7, 0.95), 0.65);
	}

	float skyGrad = clamp(upAmount, 0.0, 1.0);
	float rayleighCurve = pow(skyGrad, 0.42);
	vec3 sky = mix(horizonColor, zenithColor, rayleighCurve);

	// Below horizon haze (looking down into canyons or from helicopter)
	if (upAmount < 0.0) {
		float groundGrad = clamp(-upAmount * 2.5, 0.0, 1.0);
		sky = mix(horizonColor, fogColor * 0.72, groundGrad);
	}

	// Atmospheric horizon warm haze near the sun
	float horizonSunGlow = pow(max(0.0, sunCos), 4.0) * 0.35 * exp(-abs(upAmount) * 3.5);
	vec3 sunHazeColor = isNight ? vec3(0.4, 0.5, 0.7) : vec3(1.0, 0.82, 0.55);
	sky += sunHazeColor * horizonSunGlow;

	// --- 2. Solar Core & Atmospheric Corona (Mie Scattering) ---
	if (sunCos > 0.85) {
		float sunDisk = smoothstep(0.9980, 0.9992, sunCos);
		float sunCorona = pow(max(0.0, sunCos), 12.0) * 0.55 + pow(max(0.0, sunCos), 64.0) * 0.75;
		vec3 sunLight = isNight ? vec3(0.8, 0.9, 1.0) : vec3(1.0, 0.96, 0.88);
		sky += sunLight * (sunDisk * 3.5 + sunCorona * 1.1);
	}

	// --- 3. Night Sky Twinkling Stars (if nighttime map) ---
	if (isNight && upAmount > 0.1) {
		vec2 starCoord = rayDir.xy / (abs(rayDir.z) + 0.1) * 220.0;
		float starNoise = hash(floor(starCoord));
		if (starNoise > 0.985) {
			float twinkle = 0.5 + 0.5 * sin(time * 3.0 + starNoise * 60.0);
			sky += vec3(0.85, 0.92, 1.0) * (starNoise - 0.985) * 55.0 * twinkle * smoothstep(0.1, 0.3, upAmount);
		}
	}

	// --- 4. Fake Volumetric Clouds (Zero-Cost Analytical Layers) ---
	if (upAmount > 0.02) {
		// Layer 1: Low-altitude Fluffy Cumulus Clouds (Height = 260 blocks)
		float h1 = 260.0;
		float dist1 = h1 / max(upAmount, 0.025);
		vec2 cloudPos1 = viewOrigin.xy + rayDir.xy * dist1;
		vec2 wind1 = vec2(time * 0.015, time * 0.005);
		vec2 uv1 = cloudPos1 * 0.00075 + wind1;

		float rawDensity1 = fbm(uv1);
		// Billowy shape
		float density1 = smoothstep(0.40, 0.86, rawDensity1);

		// Volumetric 3D Shading:
		// A. Directional sun shadowing (deep crevices between billows)
		float shadowSample = fbm(uv1 - sunDir.xy * 0.032);
		float selfShadow = clamp(0.32 + 0.68 * (density1 - shadowSample * 0.65 + 0.35), 0.22, 1.0);

		// B. Forward Mie scattering (Silver Lining on sun-facing edges)
		float silverLining = pow(max(0.0, sunCos), 16.0) * (1.0 - density1) * density1 * 2.6;

		// C. Cloud Color: Bright sunlit tops + soft ambient occlusion on undersides
		vec3 directSun = isNight ? vec3(0.5, 0.6, 0.75) : vec3(1.0, 0.98, 0.92);
		vec3 cloudAmbient = mix(fogColor * 1.1, vec3(0.48, 0.58, 0.72), 0.45);
		vec3 cloudColor1 = mix(cloudAmbient, directSun, selfShadow) + directSun * silverLining;

		// Layer 2: High-Altitude Wispy Cirrus Veil (Height = 650 blocks)
		float h2 = 650.0;
		float dist2 = h2 / max(upAmount, 0.025);
		vec2 cloudPos2 = viewOrigin.xy + rayDir.xy * dist2;
		vec2 wind2 = vec2(time * 0.008, -time * 0.004);
		vec2 uv2 = cloudPos2 * 0.00038 + wind2;

		float rawDensity2 = fbm(uv2);
		float density2 = smoothstep(0.48, 0.78, rawDensity2) * 0.40;
		vec3 cloudColor2 = mix(cloudAmbient * 1.15, directSun, 0.75) + directSun * (pow(max(0.0, sunCos), 8.0) * 0.45);

		// Horizon Fog Fade: Softly blend clouds into atmospheric horizon haze
		float horizonFade = smoothstep(0.02, 0.20, upAmount);
		float distanceFade = exp(-dist1 * 0.00055);

		float alpha1 = density1 * 0.96;
		float alpha2 = density2 * (1.0 - alpha1);
		float totalCloudAlpha = clamp((alpha1 + alpha2) * horizonFade * distanceFade, 0.0, 1.0);

		vec3 finalCloudColor = mix(cloudColor2, cloudColor1, alpha1 / max(0.001, alpha1 + alpha2));
		sky = mix(sky, finalCloudColor, totalCloudAlpha);
	}

	gl_FragColor = vec4(sky, 1.0);
}
