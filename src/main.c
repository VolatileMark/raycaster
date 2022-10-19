#include <raylib.h>
#include <raymath.h>

// Defaults
#define DEFAULT_WINDOW_TITLE    "RECOIL"
#define DEFAULT_VIEW_WIDTH      1366
#define DEFAULT_VIEW_HEIGHT     768
#define DEFAULT_VIEW_RAYS       1000
#define DEFAULT_VIEW_DOF        16
#define DEFAULT_VIEW_FOV        (66.0f * DEG2RAD)

typedef struct {
    int width;
    int height;
    int rays;
    int dof;
    float fov;
} View;

typedef struct {
    float prevTime;
    float halfHeight;
    float cameraPlaneHalfWidth;
    Vector2 playerDirection;
    Vector2 cameraPlane;
} Computed;

typedef struct {
    Vector2 position;
    float rotation;
    float movementSpeed;
    float rotationSpeed;
} Player;

typedef struct {
    int textureId;
    float textureColumnOffset;
    float shade;
    float distance;
    Vector2 coordinates;
} HitDetails;

typedef struct {
    int forward;
    int right;
    float xMouseDelta;
} PlayerInput;

typedef struct {
    int ceiling;
    int wall;
    int floor;
} Tile;

typedef struct {
    int width;
    int height;
    int data[];
} TileTexture;

typedef struct {
    int width;
    int height;
    float maxWallHeight;
    Tile data[];
} Map;

// Test data
static Map TestMap = {
    .width = 10,
    .height = 12,
    .maxWallHeight = 800.0f,
    .data = {
        {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0},
    }
};

static TileTexture smiley = {
    .width = 8,
    .height = 8,
    .data = {
        0, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 0, 1, 1, 0, 1, 1,
        1, 1, 0, 1, 1, 0, 1, 1,
        1, 1, 0, 1, 1, 0, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 1, 1, 1, 1, 0, 1,
        1, 1, 0, 0, 0, 0, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
    }
};
static TileTexture *T[] = {
    &smiley
};

// Singletons
static Computed C = {0};
static PlayerInput I = {false};
static Map *M = &TestMap;
static View V = {
    .width = DEFAULT_VIEW_WIDTH,
    .height = DEFAULT_VIEW_HEIGHT,
    .rays = DEFAULT_VIEW_RAYS,
    .dof = DEFAULT_VIEW_DOF,
    .fov = DEFAULT_VIEW_FOV,
};
static Player P = {
    .position = {
        .x = 1.5f,
        .y = 1.5f
    },
    .rotation = 0.0f,
    .movementSpeed = 2.5f,
    .rotationSpeed = PI
};

// Useful defines
#define MAPSZ (M->width * M->height)
#define HALF_PI (PI / 2.0f)

static float absf(float f) {
    if (f < 0.0f) {
        return -f;
    }
    return f;
}

static HitDetails TraceHorizontalRay(Vector2 *worldCoords, Vector2 *tileCoords, float angle, float planeX) {
    // Clamp angle between 0 and 2PI
    if (angle < 0.0f) {
        angle += 2 * PI;
    } else if (angle > 2 * PI) {
        angle -= 2 * PI;
    }

    // Compute the ray direction
    Vector2 rayDirection = Vector2Add(C.playerDirection, Vector2Scale(C.cameraPlane, planeX));
    
    // Compute the distance along the ray direction to the next intersection
    // with the y-axis
    float yDeltaDistance = absf(1.0f / rayDirection.x);
    // Compute the distance along the ray direction to the next intersection
    // with the x-axis
    float xDeltaDistance = absf(1.0f / rayDirection.y);

    // Compute the distance in the horizontal direction of the ray to
    // the border of the cell
    float xDistance = (rayDirection.x > 0.0f) ? (1.0f - tileCoords->x) : tileCoords->x;
    // Compute the distance in the vertical direction of the ray to
    // the border of the cell
    float yDistance = (rayDirection.y > 0.0f) ? (1.0f - tileCoords->y) : tileCoords->y;

    // Compute the distance along the ray direction to the first intersection
    // with the y-axis
    float yIntersectionDistance = yDeltaDistance * xDistance;
    // Compute the distance along the ray direction to the first intersection
    // with the x-axis
    float xIntersectionDistance = xDeltaDistance * yDistance;

    // Find the direction we are moving in the map
    int stepX = (rayDirection.x < 0.0f) ? -1 : +1;
    int stepY = (rayDirection.y < 0.0f) ? -1 : +1;

    // Find map coordinates
    int mapX = (int) worldCoords->x;
    int mapY = (int) worldCoords->y;

    // Step the rays until one hits
    int textureId = 0;
    bool vertical = true;
    for (int i = 0; i < V.dof; i++) {
        int mapOffset = mapY * M->width + mapX;
        if (mapOffset > 0 && mapOffset < MAPSZ && (textureId = M->data[mapOffset].wall)) {
            break;
        }
        if (yIntersectionDistance < xIntersectionDistance) {
            yIntersectionDistance += yDeltaDistance;
            mapX += stepX;
            vertical = true;
        } else {
            xIntersectionDistance += xDeltaDistance;
            mapY += stepY;
            vertical = false;
        }
    }

    // Return the hit information for the first ray
    // to hit a wall
    HitDetails hit;
    hit.textureId = textureId;
    if (vertical) {
        // yDeltaDistance is subracted from the total to
        // obtain the distance projected onto the camera direction
        // This is done to fix the fisheye effect
        hit.distance = yIntersectionDistance - yDeltaDistance;
        hit.shade = 1.0f;
    } else {
        hit.distance = xIntersectionDistance - xDeltaDistance;
        hit.shade = 0.75f;
    }
    hit.coordinates.x = P.position.x + rayDirection.x * hit.distance;
    hit.coordinates.y = P.position.y + rayDirection.y * hit.distance;
    if (vertical) {
        float textureColumnOffset = hit.coordinates.y - (float) mapY;
        hit.textureColumnOffset = (stepX > 0) ? textureColumnOffset : (1.0f - textureColumnOffset);
    } else {
        float textureColumnOffset = hit.coordinates.x - (float) mapX;
        hit.textureColumnOffset = (stepY < 0) ? textureColumnOffset : (1.0f - textureColumnOffset);
    }

    return hit;
}

static void ProcessInput(void) {
    I.forward = IsKeyDown(KEY_W) - IsKeyDown(KEY_S);
    I.right = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
    I.xMouseDelta = GetMouseDelta().x;
    
    switch (GetKeyPressed()) {
        case KEY_F:
            ToggleFullscreen();
            break;
        case KEY_E:
            if (IsCursorHidden()) {
                EnableCursor();
            } else {
                DisableCursor();
            }
            break;
        default:
            break;
    }
}

static void Update(void) {
    // Calculate frame delta time
    float nowTime = GetTime();
    float delta = nowTime - C.prevTime;
    C.prevTime = nowTime;

    // Resize viewport and recalculate halfHeight and planeDistance
    if (IsWindowResized()) {
        V.width = GetRenderWidth();
        V.height = GetRenderHeight();
        C.halfHeight = V.height / 2.0f;
    }

    // Camera horizontal rotation
    P.rotation += I.xMouseDelta * P.rotationSpeed * delta;
    if (P.rotation < 0.0f) {
        P.rotation += 2.0f * PI;
    } else if (P.rotation > 2.0f * PI) {
        P.rotation -= 2.0f * PI;
    }
    // Compute player direction
    C.playerDirection.x = cosf(P.rotation);
    C.playerDirection.y = sinf(P.rotation);
    // Compute camera plane offset
    C.cameraPlane.x = -C.playerDirection.y * C.cameraPlaneHalfWidth;
    C.cameraPlane.y = +C.playerDirection.x * C.cameraPlaneHalfWidth;

    // Calculate the sign of the direction along which we are moving
    // on the x and y axis respectively
    float xSign = (P.rotation <= HALF_PI || P.rotation > 3 * HALF_PI) ? +1.0f : -1.0f;
    float ySign = (P.rotation >= 0.0f && P.rotation < PI) ? +1.0f : -1.0f;
    // Calculate the padding necessary to not slam the player into a wall
    float xPad = 0.125f * xSign;
    float yPad = 0.125f * ySign;
    // Calculate the distance delta on the x and y axis
    float x = C.playerDirection.x * delta * P.movementSpeed;
    float y = C.playerDirection.y * delta * P.movementSpeed;
    // Player movement
    Vector2 newPosition = P.position;
    if (I.forward) {
        // Remember to add the padding to the distance delta
        //                              vvvvvvvv
        newPosition.x = P.position.x + (x + xPad) * I.forward;
        newPosition.y = P.position.y + (y + yPad) * I.forward;
        // Check if the newPosition on the x-axis is inside the map
        if ((int) newPosition.x < M->width) {
            // Check for player longitudinal collision on the x-axis
            while (M->data[((int) P.position.y) * M->width + ((int) newPosition.x)].wall) {
                newPosition.x -= x * I.forward;
            }
            // Remember to subtract the padding once we are done
            // computing the corrected position along the x-axis
            P.position.x = newPosition.x - (xPad * I.forward);
        }
        // Check if the newPosition on the y-axis is inside the map
        if ((int) newPosition.y < M->height) {
            // Check for player longitudinal collision on the y-axis
            while (M->data[((int) newPosition.y) * M->width + ((int) P.position.x)].wall) {
                newPosition.y -= y * I.forward;
            }
            // Same as above, we have to subtract the padding once
            // we are done correcting the position along the y-axis
            P.position.y = newPosition.y - (yPad * I.forward);
        }
    }
    if (I.right) {
        // Remember to add the padding to the distance delta
        //                              vvvvvvvv
        newPosition.x = P.position.x - (y + yPad) * I.right;
        newPosition.y = P.position.y + (x + xPad) * I.right;
        // Check if the newPosition on the x-axis is inside the map
        if ((int) newPosition.x < M->width) {
            // Check for player lateral collision on the x-axis
            while (M->data[((int) P.position.y) * M->width + ((int) newPosition.x)].wall) {
                newPosition.x += y * I.right;
            }
            // Remember to subtract the padding once we are done
            // computing the corrected position along the x-axis
            P.position.x = newPosition.x + (yPad * I.right);
        }
        // Check if the newPosition on the y-axis is inside the map
        if ((int) newPosition.y < M->height) {
            // Check for player lateral collision on the y-axis
            while (M->data[((int) newPosition.y) * M->width + ((int) P.position.x)].wall) {
                newPosition.y -= x * I.right;
            }
            // Same as above, we have to subtract the padding once
            // we are done correcting the position along the y-axis
            P.position.y = newPosition.y - (xPad * I.right);
        }
    }
}

static void Render(void) {
    ClearBackground(BLACK);
    // Draw the sky
    DrawRectangle(0, 0, V.width, V.height / 2, BLUE);

    float halfHeight = V.height / 2.0f;
    // Compute the cell coordinates
    Vector2 worldCoords = { .x = (int) P.position.x, .y = (int) P.position.y };
    // Compute the coordinates inside the cell
    Vector2 tileCoords = Vector2Subtract(P.position, worldCoords);
    // Calculate the angle increment for each ray
    float angleStep = V.fov / V.rays;
    // Calculate the angle for the first ray
    float angleStart = P.rotation - V.fov / 2.0f;
    // Calculate the width of each ray
    float thickness = (float) V.width / V.rays;
    // Check if we need an extra ray to cover the entire screen
    int rays = (V.width % V.rays) ? (V.rays + 1) : V.rays;
    for (int n = 0; n < rays; n++) {
        // Compute the angle of the n-th ray
        float angle = angleStart + n * angleStep;
        // planeX = coordinate (between -1 and 1) of the ray on the x-axis of the plane
        float planeX = (2.0f * ((float) n / rays)) - 1.0f; 
        // Trace the ray
        HitDetails hit = TraceHorizontalRay(&worldCoords, &tileCoords, angle, planeX);
        // Check if the ray hit a non empty cell
        if (hit.textureId) {
            // If we did, get the cell's texture
            TileTexture *texture = T[hit.textureId - 1];
            // Calculate the height of the pixel column
            float lineHeight = M->maxWallHeight / hit.distance;
            // Calculate the height of each pixel in the column
            float dotHeight = (float) lineHeight / texture->height;
            // Find the corresponding texture column
            int textureColumn = hit.textureColumnOffset * texture->width;
            // Clip the height of the column if it's higher than the
            // viewport's height and offset the texture 
            float textureOffset = 0.0f;
            if (lineHeight > V.height) {
                textureOffset = (lineHeight - V.height) / 2.0f;
                lineHeight = V.height;
            }
            // Compute the starting point of the column
            float xStart = n * thickness;
            float yStart = halfHeight - lineHeight / 2.0f - textureOffset;
            // Draw each pixel (of size thickness x dotHeight)
            // in the texture column
            for (int i = 0; i < texture->height; i++) {
                Color color = (texture->data[i * texture->width + textureColumn]) ? GOLD : WHITE;
                // Shade the color accordingly
                color.r *= hit.shade;
                color.g *= hit.shade;
                color.b *= hit.shade;
                // Draw the i-th pixel
                DrawRectangle(xStart, yStart + roundf(i * dotHeight), ceilf(thickness), ceilf(dotHeight), color);
            }
        }
    }

    // Draw 2D view (debug only!!!)
    //{
    //    for (int y = 0; y < M->height; y++) {
    //        for (int x = 0; x < M->width; x++) {
    //            int v = M->data[y * M->width + x].wall;
    //            DrawRectangle(x * 64, y * 64, 64 - 1, 64 - 1, (v) ? GRAY : WHITE);
    //        }
    //    }
    //    for (int i = 0; i < rays; i++) {
    //        float angle = angleStart + i * angleStep;
    //        // planeX = coordinate (between -1 and 1) of the ray on the x-axis of the plane
    //        float planeX = (2.0f * ((float) i / rays)) - 1.0f; 
    //        HitDetails hit = TraceHorizontalRay(&worldCoords, &tileCoords, angle, planeX);
    //        DrawLine(P.position.x * 64, P.position.y * 64, hit.coordinates.x * 64, hit.coordinates.y * 64, GREEN);
    //    }
    //    DrawLine(P.position.x * 64, P.position.y * 64, (P.position.x + C.playerDirection.x) * 64, (P.position.y + C.playerDirection.y) * 64, BLUE);
    //    DrawLine(P.position.x * 64, P.position.y * 64, (P.position.x + C.playerDirection.x + C.cameraPlane.x) * 64, (P.position.y + C.playerDirection.y + C.cameraPlane.y) * 64, BLACK);
    //    DrawLine(P.position.x * 64, P.position.y * 64, (P.position.x + C.playerDirection.x - C.cameraPlane.x) * 64, (P.position.y + C.playerDirection.y - C.cameraPlane.y) * 64, BLACK);
    //    DrawLine((P.position.x + C.playerDirection.x) * 64, (P.position.y + C.playerDirection.y) * 64, (P.position.x + C.playerDirection.x + C.cameraPlane.x) * 64, (P.position.y + C.playerDirection.y + C.cameraPlane.y) * 64, DARKGREEN);
    //    DrawLine((P.position.x + C.playerDirection.x) * 64, (P.position.y + C.playerDirection.y) * 64, (P.position.x + C.playerDirection.x - C.cameraPlane.x) * 64, (P.position.y + C.playerDirection.y - C.cameraPlane.y) * 64, DARKGREEN);
    //    DrawRectangle(P.position.x * 64 - 4, P.position.y * 64 - 4, 8, 8, RED);
    //}

    DrawFPS(10, 10);
}

static void Init(void) {
    InitWindow(
        V.width, 
        V.height, 
        DEFAULT_WINDOW_TITLE
    );
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    DisableCursor();
    C.halfHeight = V.height / 2.0f;
    C.cameraPlaneHalfWidth = 1.0f / (2.0f * tanf(V.fov / 2.0f));
    C.prevTime = GetTime();
}

static void Shutdown(void) {
    CloseWindow();
}

int main(int argc, char **argv, char **envp) {
    Init();
    while (!WindowShouldClose()) {
        BeginDrawing();
        ProcessInput();
        Update();
        Render();
        EndDrawing();
    }
    Shutdown();
    return 0;
}
