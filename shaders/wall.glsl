#version 430

#define PI 3.14159265358979323846

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct Column {
    int textureId;
    float brightness;
    float lineHeight;
    float lineOffset;
    float textureColumnOffset;
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
    ivec3 mapData[];
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
    
    ivec2 mapCoords = ivec2(playerPosition);
    vec2 tileCoords = playerPosition - mapCoords;

    float cameraX = (2.0f * (float(n) / float(viewportWidth))) - 1.0f;
    
    float angle = columnAngleStart + n * columnAngleStep;
    while (angle < 0.0) {
        angle += 2 * PI;
    }
    while (angle > 2 * PI) {
        angle -= 2 * PI;
    }

    vec2 rayDirection = playerPosition + cameraPlane * cameraX;

    float yDeltaDistance = (rayDirection.x == 0) ? 1e30 : abs(1.0 / rayDirection.x);
    float xDeltaDistance = (rayDirection.y == 0) ? 1e30 : abs(1.0 / rayDirection.y);

    float xDistance = (rayDirection.x > 0.0) ? (1.0 - tileCoords.x) : tileCoords.x;
    float yDistance = (rayDirection.y > 0.0) ? (1.0 - tileCoords.y) : tileCoords.y;

    float yIntersectionDistance = yDeltaDistance * xDistance;
    float xIntersectionDistance = xDeltaDistance * yDistance;

    ivec2 step = ivec2(sign(rayDirection));
    
    float lineOffset = float(step.x);
    
    int cellId = 0;
    bool vertical = false;
    for (int i = 0; i < depthOfField; i++) {
        int mapOffset = mapCoords.y * mapWidth + mapCoords.x;
        if (mapOffset >= 0 && mapOffset < (mapWidth * mapHeight) && (cellId = mapData[mapOffset].y) != 0) {
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

    if (cellId == 0) {
        return;
    }

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
    //float lineOffset = 0.0;
    //if (lineHeight > viewportHeight) {
    //    lineOffset = (lineHeight - viewportHeight) / 2.0;
    //    lineHeight = viewportHeight;
    //}

    outputData[n].textureId = cellId - 1;
    outputData[n].brightness = brightness;
    outputData[n].lineHeight = lineHeight;
    outputData[n].lineOffset = lineOffset;
    outputData[n].textureColumnOffset = textureColumnOffset;
}
