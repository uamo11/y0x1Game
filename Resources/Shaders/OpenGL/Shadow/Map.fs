/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

uniform sampler2D mapShadowTexture;
uniform float realisticShadows;
varying vec3 mapShadowCoord;

float EvaluateMapShadow() {
	vec2 baseUV = mapShadowCoord.xy;
	float z = mapShadowCoord.z - 0.00015;

	if (realisticShadows > 0.5) {
		const float stepUV = 0.45 / 512.0;
		float s0 = texture2D(mapShadowTexture, baseUV + vec2(-stepUV, -stepUV)).w;
		float s1 = texture2D(mapShadowTexture, baseUV + vec2( stepUV, -stepUV)).w;
		float s2 = texture2D(mapShadowTexture, baseUV + vec2(-stepUV,  stepUV)).w;
		float s3 = texture2D(mapShadowTexture, baseUV + vec2( stepUV,  stepUV)).w;

		float v0 = (s0 < z) ? 0.0 : 1.0;
		float v1 = (s1 < z) ? 0.0 : 1.0;
		float v2 = (s2 < z) ? 0.0 : 1.0;
		float v3 = (s3 < z) ? 0.0 : 1.0;

		return (v0 + v1 + v2 + v3) * 0.25;
	}

	float val = texture2D(mapShadowTexture, baseUV).w;
	return (val < z) ? 0.0 : 1.0;
}