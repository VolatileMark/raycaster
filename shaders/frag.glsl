#version 430

#define SKY_COLOR vec4(0.25, 0.5, 1.0, 1.0)
#define FLOOR_COLOR vec4(0.1, 0.1, 0.1, 1.0)
#define WALL_COLOR vec3(1.0, 0.5, 0.0)

#define WALKING_AMPLITUDE 10.0
#define WALKING_FREQUENCY 2.0

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

struct Tile {
    int ceiling;
    int wall;
    int floor;
    int padding;
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

layout (std430, binding = 3) readonly restrict buffer MapData {
    int mapWidth;
    int mapHeight;
    Tile mapData[];
};

layout (std430, binding = 4) readonly restrict buffer FrameData {
    vec2 playerPosition;
    ivec2 playerMapCoords;
    vec2 playerTileCoords;
    vec2 playerDirection;
    vec2 cameraPlane;
};

uniform sampler2D tileMap;

void main() {
    // Get the size of the tile map in pixels
    ivec2 size = textureSize(tileMap, 0);
    // Get the pixel position of the fragment
    float xPosition = fragTexCoord.x * viewportWidth;
    float yPosition = fragTexCoord.y * viewportHeight;
    // Get the column in which this fragment (pixel) is located
    int column = int(xPosition);
    // Get the line height and half the line height
    float lineHeight = inputData[column].lineHeight;
    float halfLineHeight = (lineHeight / 2.0);
    // Check if the pixel is part of the wall, the ceiling or the floor
    bool isCeiling = yPosition < viewportHalfHeight - halfLineHeight;
    bool isFloor = yPosition >= viewportHalfHeight + halfLineHeight;
    if (isCeiling || isFloor) {
        // If it's a floor or ceiling pixel, compute it's texture coordinates
        // or fill it with the floor or sky color respectively
        // Flip the coordinate if the pixel belongs to the floor
        // for the rest of the calculation to work correctly
        float yCorrected = (isCeiling) ? yPosition : (viewportHeight - yPosition);
        // Compute the distance in pixels from horizon
        float pixelsFromHorizon = viewportHalfHeight - yCorrected;
        // Compute the distance of the pixel ray (range is from 1.0 to +inf)
        float distance = viewportHalfHeight / pixelsFromHorizon;
        // Compute the left and right segments of the camera frustum
        vec2 cameraPlaneLeft = playerDirection - cameraPlane;
        vec2 cameraPlaneRight = playerDirection + cameraPlane;
        // Compute the step for each pixel in the scanline
        vec2 step = (cameraPlaneRight - cameraPlaneLeft) * (distance / viewportWidth);
        // Compute the hit position for this pixel in the scanline
        vec2 position = playerPosition + (cameraPlaneLeft * distance) + (step * xPosition);
        // Get the coordinates of the cell that was hit by the ray
        ivec2 cell = ivec2(position);
        // Check if the cell is valid
        int cellOffset = cell.y * mapWidth + cell.x;
        if (cellOffset >= 0 && cellOffset < mapWidth * mapHeight) {
            // Get the id of the cell and check if it's not empty
            int cellId = (isCeiling) ? mapData[cellOffset].ceiling : mapData[cellOffset].floor;
            if (cellId != 0) {
                // Get the texture id
                int textureId = cellId - 1;
                // Get the texture coordinates in the tile map
                ivec2 tileCoords = ivec2(textureId % tileMapSize.x, int(textureId / tileMapSize.x));
                // Compute texture coordinates
                vec2 texCoords = (position - cell) + tileCoords;
                // Get the brightness of the pixel
                float brightness = (isCeiling) ? 0.85 : 1.0;
                // Sample the tile map
                vec4 color = texture(tileMap, texCoords * tileSize / size);
                // Adjust the brightness
                finalColor = vec4(color.xyz * brightness, color.w);
                return;
            }
        }
        // If the cell is not valid draw the sky or floor color, accordingly
        finalColor = (isCeiling) ? SKY_COLOR : FLOOR_COLOR;
    } else if (lineHeight > 0) {
        // If the height of the column is not zero, set
        // the pixel to the color of the texture in the tilemap with id textureId
        float brightness = inputData[column].brightness;
        // Get the column texture id
        int textureId = inputData[column].textureId;
        // If the texture id is not valid make the pixel the default color
        // for a wall
        if (textureId < 0 || textureId >= tileMapSize.x * tileMapSize.y) {
            finalColor = vec4(WALL_COLOR * brightness, 1.0);
            return;
        }
        // Calculate the percentage of the column shown currently shown on screen
        float columnShownPerc = (lineHeight / (lineHeight + inputData[column].lineOffset));
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
        vec2 texCoords = (tileCoords + vec2(texX, texY));
        // Sample the texture
        vec4 color = texture(tileMap, texCoords * tileSize / size);
        // Adjust the brightness of the color accordingly
        finalColor = vec4(color.xyz * brightness, color.w);
    } else {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
