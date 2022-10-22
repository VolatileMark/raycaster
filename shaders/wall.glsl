#version 430

#define PI 3.14159265358979323846

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

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

layout (std430, binding = 1) writeonly restrict buffer Columns {
    Column outputData[];
};

layout (std430, binding = 2) readonly restrict buffer Constants {
    int depthOfField;
    int viewportWidth;
    int viewportHeight;
    float viewportHalfHeight;
    float columnAngleStart;
    float columnAngleStep;
};

layout (std430, binding = 3) readonly restrict buffer MapData {
    int mapWidth;
    int mapHeight;
    float maxWallHeight;
    Tile mapData[];
};

layout (std430, binding = 4) readonly restrict buffer FrameData {
    vec2 playerPosition;
    vec2 playerDirection;
    vec2 cameraPlane;
};

void main() {
    uint n = gl_GlobalInvocationID.x;
    if (n >= viewportWidth) {
        return;
    }
    
    // Compute the starting map coordinates
    ivec2 mapCoords = ivec2(playerPosition);
    // Compute the player's coordinates inside the tile
    vec2 tileCoords = playerPosition - mapCoords;

    // cameraX = coordinate (between -1 and 1) of the ray on the x-axis
    //           of the camera plane
    float cameraX = (2.0f * (float(n) / viewportWidth)) - 1.0f;
    
    // Compute the angle of the ray
    float angle = columnAngleStart + n * columnAngleStep;
    // Clamp the angle between 0 and 2*PI
    while (angle > 2 * PI) {
        angle -= 2 * PI;
    }
    while (angle < 0.0) {
        angle += 2 * PI;
    }

    // Compute the ray direction
    vec2 rayDirection = playerDirection + cameraPlane * cameraX;

    // Compute the distance along the ray direction to the next intersection
    // with the y-axis
    float yDeltaDistance = (rayDirection.x == 0) ? 1e30 : abs(1.0 / rayDirection.x);
    // Compute the distance along the ray direction to the next intersection
    // with the x-axis
    float xDeltaDistance = (rayDirection.y == 0) ? 1e30 : abs(1.0 / rayDirection.y);

    // Compute the distance in the horizontal direction of the ray to
    // the border of the cell
    float xDistance = (rayDirection.x > 0.0) ? (1.0 - tileCoords.x) : tileCoords.x;
    // Compute the distance in the vertical direction of the ray to
    // the border of the cell
    float yDistance = (rayDirection.y > 0.0) ? (1.0 - tileCoords.y) : tileCoords.y;

    // Compute the distance along the ray direction to the first intersection
    // with the y-axis
    float yIntersectionDistance = yDeltaDistance * xDistance;
    // Compute the distance along the ray direction to the first intersection
    // with the x-axis
    float xIntersectionDistance = xDeltaDistance * yDistance;

    // Find the direction we are moving in the map
    ivec2 step = ivec2(sign(rayDirection));

    int cellId = 0;
    bool vertical = false;
    // Step the rays until one hits
    for (int i = 0; i < depthOfField; i++) {
        int mapOffset = mapCoords.y * mapWidth + mapCoords.x;
        if (mapOffset >= 0 && mapOffset < (mapWidth * mapHeight) && (cellId = mapData[mapOffset].wall) != 0) {
            break;
        }
        if (yIntersectionDistance < xIntersectionDistance) {
            yIntersectionDistance += yDeltaDistance;
            mapCoords.x += step.x;
            vertical = true;
        } else {
            xIntersectionDistance += xDeltaDistance;
            mapCoords.y += step.y;
            vertical = false;
        }
    }

    // Check if the ray hit an empty cell
    if (cellId == 0) {
        return;
    }

    // If we didn't, store the hit information for the first
    // ray to hit a wall
    float rayDistance, brightness;
    if (vertical) {
        rayDistance = yIntersectionDistance - yDeltaDistance;
        brightness = 1.0;
    } else {
        rayDistance = xIntersectionDistance - xDeltaDistance;
        brightness = 0.85;
    }
    vec2 coordinates = playerPosition + rayDirection * rayDistance;
    float textureColumnOffset;
    if (vertical) {
        textureColumnOffset = coordinates.y - mapCoords.y;
        textureColumnOffset = (step.x > 0) ? textureColumnOffset : (1.0 - textureColumnOffset);
    } else {
        textureColumnOffset = coordinates.x - mapCoords.x;
        textureColumnOffset = (step.y < 0) ? textureColumnOffset : (1.0 - textureColumnOffset);
    }

    float lineHeight = maxWallHeight / rayDistance;
    float lineOffset = 0.0;
    if (lineHeight > viewportHeight) {
        lineOffset = (lineHeight - viewportHeight) / 2.0;
        lineHeight = viewportHeight;
    }

    outputData[n].textureId = cellId - 1;
    outputData[n].brightness = brightness;
    outputData[n].lineHeight = lineHeight;
    outputData[n].lineOffset = mapData[0].ceiling;
    outputData[n].textureColumnOffset = textureColumnOffset;
}
