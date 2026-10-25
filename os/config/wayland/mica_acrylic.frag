#version 450 core
/*
 * Quantro OS - Fluent Mica & Acrylic Surface Shader (GLSL 450)
 * Mica: 120px Gaussian Blur + 85% Opacity Blend with Wallpaper
 * Acrylic: 30px Blur + 2% Procedural Noise Texture Overlay
 */

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_fragColor;

layout(binding = 0) uniform sampler2D u_desktopWallpaper;
layout(binding = 1) uniform sampler2D u_windowSurface;

uniform int   u_materialMode; // 0: Mica, 1: Acrylic, 2: Solid
uniform vec4  u_tintColor;    // Windows 11 Dark: rgba(32, 32, 32, 0.85)
uniform vec2  u_resolution;
uniform float u_noiseIntensity; // 0.02 for Acrylic

// Pseudo-random noise generator for 2% Acrylic texture
float random(vec2 uv) {
    return fract(sin(dot(uv.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Optimized 9-tap dual filter Gaussian blur approximation
vec4 gaussianBlur(sampler2D tex, vec2 uv, float radius) {
    vec4 sum = vec4(0.0);
    vec2 step = radius / u_resolution;

    sum += texture(tex, uv + vec2(-step.x, -step.y)) * 0.0625;
    sum += texture(tex, uv + vec2( 0.0,    -step.y)) * 0.1250;
    sum += texture(tex, uv + vec2( step.x, -step.y)) * 0.0625;

    sum += texture(tex, uv + vec2(-step.x,  0.0))    * 0.1250;
    sum += texture(tex, uv)                          * 0.2500;
    sum += texture(tex, uv + vec2( step.x,  0.0))    * 0.1250;

    sum += texture(tex, uv + vec2(-step.x,  step.y)) * 0.0625;
    sum += texture(tex, uv + vec2( 0.0,     step.y)) * 0.1250;
    sum += texture(tex, uv + vec2( step.x,  step.y)) * 0.0625;

    return sum;
}

void main() {
    vec2 screenUv = gl_FragCoord.xy / u_resolution;
    vec4 baseColor = texture(u_windowSurface, v_texCoord);

    if (u_materialMode == 0) {
        // MICA: 120px Gaussian Blur + 85% opacity
        vec4 blurredWallpaper = gaussianBlur(u_desktopWallpaper, screenUv, 120.0);
        vec4 micaTint = vec4(0.125, 0.125, 0.125, 0.85); // rgba(32, 32, 32, 0.85)
        vec4 mixedMica = mix(blurredWallpaper, micaTint, 0.85);
        o_fragColor = mix(mixedMica, baseColor, baseColor.a);
    } 
    else if (u_materialMode == 1) {
        // ACRYLIC: 30px Blur + 2% Noise
        vec4 blurredSurface = gaussianBlur(u_desktopWallpaper, screenUv, 30.0);
        float noise = (random(screenUv * 500.0) - 0.5) * 0.02;
        vec4 acrylicTint = vec4(0.16, 0.16, 0.16, 0.75); // rgba(40, 40, 40, 0.75)
        vec4 colored = mix(blurredSurface, acrylicTint, 0.75) + vec4(noise);
        o_fragColor = mix(colored, baseColor, baseColor.a);
    } 
    else {
        o_fragColor = baseColor;
    }
}
