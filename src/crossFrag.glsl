#version 330 core
out vec4 FragColor;

uniform vec4 crosshairColor;
uniform float innerRadius;
uniform float outerRadius;

void main()
{
    float dist = length(gl_FragCoord.xy - vec2(gl_FragCoord.z * gl_FragCoord.w * 0.5));
    float distance = length(gl_PointCoord - vec2(0.5));
    if (distance > 0.5 || distance < innerRadius) {
        discard;
    }
    FragColor = crosshairColor;
}