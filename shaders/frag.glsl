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
    ivec2 tileSize;
    ivec2 tileMapSize;
};

uniform sampler2D tileMap;

void main() {
    // Get the column in which this fragment (pixel) is located
    int column = int(fragTexCoord.x * viewportWidth);
    // Get the column texture id
    int textureId = inputData[column].textureId;
    // Get the vertical pixel position of the fragment
    float yPosition = fragTexCoord.y * viewportHeight;
    float lineHeight = inputData[column].lineHeight;
    float halfLineHeight = (lineHeight / 2.0);
    if (yPosition < viewportHalfHeight - halfLineHeight) {
        // If the pixel is closer to the top of the screen than the
        // highest pixel in the column, make the pixel the color of the sky
        finalColor = vec4(0.25, 0.5, 1.0, 1.0);
    } else if (yPosition >= viewportHalfHeight + halfLineHeight) {
        // If the pixel is further from the top of the screen than the
        // lowest pixel in the column, make the pixel the color of the ground
        finalColor = vec4(0.1, 0.1, 0.1, 1.0);
    } else if (halfLineHeight > 0 && textureId >= 0 && textureId < tileMapSize.x * tileMapSize.y) {
        // If the height of the column is not zero and the texture id is valid, set
        // the pixel to the color of the texture in the tilemap with id textureId
        // Calculate the percentage of the column shown currently shown on screen
        float columnShownPerc = (lineHeight / (lineHeight + inputData[column].lineOffset));
        // Get the size of the tile map in pixels
        ivec2 size = textureSize(tileMap, 0);
        // Get the texture coordinates in the tile map
        ivec2 tileCoords = ivec2(textureId % tileMapSize.x, int(textureId / tileMapSize.x));
        // Get the texture X coordinate already computed in the compute shader
        float texX = inputData[column].textureColumnOffset;
        // Calculate the texture Y coordinate range (from 0.0 to 1.0)
        float texYRange = ((yPosition - (viewportHalfHeight - halfLineHeight)) / lineHeight);
        // Adjust the texture Y coordinate to account for the part
        // of the column not shown on screen
        float texY = (texYRange * columnShownPerc) + (1.0 - columnShownPerc) / 2.0;
        // Compute the texture coordinates for the tile map
        vec2 texCoords = (tileCoords + vec2(texX, texY)) * tileSize / size;
        // Sample the texture
        vec4 color = texture(tileMap, texCoords);
        // Adjust the brightness of the color accordingly
        finalColor = vec4(color.xyz * inputData[column].brightness, color.w);
    } else {
        // The pixel is invalid
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
