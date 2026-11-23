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

uniform sampler2DShadow shadowMapTexture1;
uniform sampler2DShadow shadowMapTexture2;
uniform sampler2DShadow shadowMapTexture3;

varying vec4 shadowMapCoord1;
varying vec4 shadowMapCoord2;
varying vec4 shadowMapCoord3;

uniform float cascadeSplit1;
uniform float cascadeSplit2;
uniform float realisticShadows;

#define shadowMapViewPosZ shadowMapCoord1.w

float EvaluteModelShadow() {
	vec4 scoord;
	if (realisticShadows > 0.5) {
		const vec2 texel = vec2(1.0 / 2048.0);
		if (shadowMapViewPosZ > cascadeSplit1) {
			scoord = shadowMapCoord1;
			float s0 = shadow2D(shadowMapTexture1, vec3(scoord.xy + vec2(-0.5, -0.5) * texel, scoord.z)).x;
			float s1 = shadow2D(shadowMapTexture1, vec3(scoord.xy + vec2( 0.5, -0.5) * texel, scoord.z)).x;
			float s2 = shadow2D(shadowMapTexture1, vec3(scoord.xy + vec2(-0.5,  0.5) * texel, scoord.z)).x;
			float s3 = shadow2D(shadowMapTexture1, vec3(scoord.xy + vec2( 0.5,  0.5) * texel, scoord.z)).x;
			return (s0 + s1 + s2 + s3) * 0.25;
		} else if (shadowMapViewPosZ > cascadeSplit2) {
			scoord = shadowMapCoord2;
			float s0 = shadow2D(shadowMapTexture2, vec3(scoord.xy + vec2(-0.5, -0.5) * texel, scoord.z)).x;
			float s1 = shadow2D(shadowMapTexture2, vec3(scoord.xy + vec2( 0.5, -0.5) * texel, scoord.z)).x;
			float s2 = shadow2D(shadowMapTexture2, vec3(scoord.xy + vec2(-0.5,  0.5) * texel, scoord.z)).x;
			float s3 = shadow2D(shadowMapTexture2, vec3(scoord.xy + vec2( 0.5,  0.5) * texel, scoord.z)).x;
			return (s0 + s1 + s2 + s3) * 0.25;
		} else {
			scoord = shadowMapCoord3;
			float s0 = shadow2D(shadowMapTexture3, vec3(scoord.xy + vec2(-0.5, -0.5) * texel, scoord.z)).x;
			float s1 = shadow2D(shadowMapTexture3, vec3(scoord.xy + vec2( 0.5, -0.5) * texel, scoord.z)).x;
			float s2 = shadow2D(shadowMapTexture3, vec3(scoord.xy + vec2(-0.5,  0.5) * texel, scoord.z)).x;
			float s3 = shadow2D(shadowMapTexture3, vec3(scoord.xy + vec2( 0.5,  0.5) * texel, scoord.z)).x;
			return (s0 + s1 + s2 + s3) * 0.25;
		}
	}

	if (shadowMapViewPosZ > cascadeSplit1) {
		scoord = shadowMapCoord1.xyzw;
		return shadow2D(shadowMapTexture1, scoord.xyz).x;
	} else if (shadowMapViewPosZ > cascadeSplit2) {
		scoord = shadowMapCoord2.xyzw;
		return shadow2D(shadowMapTexture2, scoord.xyz).x;
	} else {
		scoord = shadowMapCoord3.xyzw;
		return shadow2D(shadowMapTexture3, scoord.xyz).x;
	}
}