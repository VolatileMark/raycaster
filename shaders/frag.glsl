#version 430

in vec2 fragTexCoord;

out vec4 finalColor;

struct Column {
    int padding;
    int textureId;
    float brightness;
    float lineHeight;
    float lineOffset;
    float textureColumnOffset;
};

layout (std430, binding = 1) readonly buffer Columns {
    Column inputData[];
};

layout (std430, binding = 2) readonly buffer Constants {
    int depthOfField;
    int viewportWidth;
    int viewportHeight;
    float viewportHalfHeight;
    float columnAngleStart;
    float columnAngleStep;
};

void main() {
    int column = int(fragTexCoord.x * viewportWidth);
    float yPosition = fragTexCoord.y * viewportHeight;
    float halfLineHeight = inputData[column].lineHeight / 2.0;
    if (yPosition < viewportHalfHeight - halfLineHeight) {
        finalColor = vec4(0.0, 0.0, 1.0, 1.0);
    } else if (yPosition >= viewportHalfHeight + halfLineHeight) {
        finalColor = vec4(0.1, 0.1, 0.1, 1.0);
    } else {
        vec3 color = vec3(1.0, 0.5, 0.0) * inputData[column].brightness;
        finalColor = vec4(color, 1.0);
    }
}
