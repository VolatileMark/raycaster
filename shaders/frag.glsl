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
    // Get the column in which this fragment (pixel) is located
    int column = int(fragTexCoord.x * viewportWidth);
    // Get the vertical pixel position of the fragment
    float yPosition = fragTexCoord.y * viewportHeight;
    float halfLineHeight = inputData[column].lineHeight / 2.0;
    if (yPosition < viewportHalfHeight - halfLineHeight) {
        // If the pixel is closer to the top of the screen than the
        // highest pixel in the column, make the pixel the color of the sky
        finalColor = vec4(0.0, 0.0, 1.0, 1.0);
    } else if (yPosition > viewportHalfHeight + halfLineHeight) {
        // If the pixel is further from the top of the screen than the
        // lowest pixel in the column, make the pixel the color of the ground
        finalColor = vec4(0.1, 0.1, 0.1, 1.0);
    } else {
        // If the pixel in between the highest and the lowest (inclusive) pixel
        // in the column, make the pixel the color of the wall * the brightness
        vec3 color = vec3(1.0, 0.5, 0.0) * inputData[column].brightness;
        finalColor = vec4(color, 1.0);
    }
}
