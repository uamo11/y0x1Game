/*
 Copyright (c) 2013-2026 yvt / ySpades contributors
 Ultra-lightweight Fake Volumetric Atmospheric Sky & Clouds Shader
 Zero-performance cost: Single-pass analytical pseudo-volumetric atmosphere
*/

attribute vec2 positionAttribute;

uniform vec3 viewOrigin;
uniform vec3 viewAxisUp, viewAxisSide, viewAxisFront;
uniform vec2 fov;

varying vec2 texCoord;
varying vec3 viewDir;

void main() {
	vec2 pos = positionAttribute;
	vec2 scrPos = pos * 2.0 - 1.0;

	// Rendered at the far clipping plane (z = 1.0 in NDC)
	// Hardware depth test (GL_LEQUAL) discards every pixel covered by world blocks/models!
	gl_Position = vec4(scrPos, 1.0, 1.0);
	texCoord = pos;

	vec2 viewTan = mix(-fov, fov, pos);
	viewDir = viewAxisUp * viewTan.y + viewAxisSide * viewTan.x + viewAxisFront;
}
