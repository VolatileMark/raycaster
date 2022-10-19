#include <raylib.h>
#include <raymath.h>

// Defaults
#define DEFAULT_WINDOW_TITLE    "RECOIL"
#define DEFAULT_VIEW_WIDTH      1366
#define DEFAULT_VIEW_HEIGHT     768
#define DEFAULT_VIEW_RAYS       480
#define DEFAULT_VIEW_DOF        12
#define DEFAULT_VIEW_FOV        (70.0f * DEG2RAD)

typedef struct {
    int width;
    int height;
    int rays;
    int dof;
    float fov;
} Viewport;

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
    int width;
    int height;
    float maxWallHeight;
    int data[];
} Map;

typedef struct {
    int width;
    int height;
    int data[];
} RaycasterTexture;

// Test data
static Map TestMap = {
    .width = 10,
    .height = 12,
    .maxWallHeight = 900.0f,
    .data = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 1, 1, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 0, 1, 1, 1, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    }
};

static RaycasterTexture testTexture = {
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
static RaycasterTexture *Textures[] = {
    &testTexture
};

// Singletons
static float prevTime = 0.0f;
static PlayerInput I = {false};
static Map *M = &TestMap;
static Viewport V = {
    .width = DEFAULT_VIEW_WIDTH,
    .height = DEFAULT_VIEW_HEIGHT,
    .rays = DEFAULT_VIEW_RAYS,
    .dof = DEFAULT_VIEW_DOF,
    .fov = DEFAULT_VIEW_FOV
};
static Player P = {
    .position = {
        .x = 4.35f,
        .y = 3.6f
    },
    .rotation = 0.0f,
    .movementSpeed = 2.5f,
    .rotationSpeed = PI
};

// Useful defines
#define MAPSZ (M->width * M->height)
#define HALF_PI (PI / 2.0f)

static int CheckRayHit(Vector2 *ray, float xStep, float yStep) {
    int mapX = (int) (ray->x + 0.00001f * xStep);
    int mapY = (int) (ray->y + 0.00001f * yStep);
    int mapPointer = mapY * M->width + mapX;
    if (mapPointer >= 0 && mapPointer < MAPSZ) {
        return M->data[mapPointer];
    }
    return 0;
}

static HitDetails TraceHorizontalRay(Vector2 *worldCoords, Vector2 *tileCoords, float angle) {
    // Clamp angle between 0 and 2PI
    if (angle < 0.0f) {
        angle += 2 * PI;
    } else if (angle > 2 * PI) {
        angle -= 2 * PI;
    }
    // Calculate the directional distance in which we have to step the ray
    // along the x-axis to get to the next intersection with y-axis
    float xStep = (angle <= HALF_PI || angle > 3.0f *  HALF_PI) ? +1.0f : -1.0f;
    // Calculate the directional distance in which we have to step the ray
    // along the y-axis to get to the next intersection with x-axis
    float yStep = (angle >= 0.0f && angle < PI) ? +1.0f : -1.0f;

    // Calculate the directional distance on the x-axis
    // to the cell border, according to where the player is pointing
    float xDistance = (xStep > 0.0f) ? (1.0f - tileCoords->x) : -tileCoords->x;
    // Calculate the directional distance on the y-axis
    // to the cell border, according to where the player is pointing
    float yDistance = (yStep > 0.0f) ? (1.0f - tileCoords->y) : -tileCoords->y;

    float cos = cosf(angle);
    float sin = sinf(angle);
    float tan = sin / cos;

    // Calculate directional distance we have to step the ray along the
    // y-axis to get to the next intersection with the y-axis
    float yDelta = (sin < 0.001f && sin > -0.001f) ? 0.0f : tan;
    // Calculate directional distance we have to step the ray along the
    // x-axis to get to the next intersection with the x-axis
    float xDelta = (cos < 0.001f && cos > -0.001f) ? 0.0f : 1.0f / tan;

    // Calculate the directional distance to the first intersection with the y-axis
    float yIntersection = xDistance * yDelta;
    // Calculate the directional distance to the first intersection with the x-axis
    float xIntersection = yDistance * xDelta;

    Vector2 yIntersectionVector = {
        .x = xDistance,
        .y = yIntersection
    };

    Vector2 xIntersectionVector = {
        .x = xIntersection,
        .y = yDistance
    };

    // Build the ray that steps along the x-axis and intersects the y-axis
    Vector2 xRay = Vector2Add(P.position, yIntersectionVector);
    // Build the ray that steps along the y-axis and intersects the x-axis
    Vector2 yRay = Vector2Add(P.position, xIntersectionVector);

    // Step the rays and check what they hit (if they hit anything)
    int xHit = 0, yHit = 0;
    for (int dof = 0; dof < V.dof; dof++) {
        if ((xHit = CheckRayHit(&xRay, xStep, yStep))) {
            break;
        } else {
            xRay.x += xStep;
            xRay.y += yDelta * xStep;
        }
    }
    for (int dof = 0; dof < V.dof; dof++) {
        if ((yHit = CheckRayHit(&yRay, xStep, yStep))) {
            break;
        } else {
            yRay.x += xDelta * yStep;
            yRay.y += yStep;
        }
    }

    // Calculate the length of each ray
    float xLen = Vector2Length(Vector2Subtract(xRay, P.position));
    float yLen = Vector2Length(Vector2Subtract(yRay, P.position));

    // Calculate the texture column offset
    float xTextureColumnOffset = (xRay.y - (int) xRay.y);
    float yTextureColumnOffset = (yRay.x - (int) yRay.x);
    // Calculate the corrected texture column offset
    float xTextureColumnCorrectedOffset = (xStep > 0.0f) ? xTextureColumnOffset : (1.0f - xTextureColumnOffset);
    float yTextureColumnCorrectedOffset = (yStep < 0.0f) ? yTextureColumnOffset : (1.0f - yTextureColumnOffset);

    // Return the data for shortest ray
    HitDetails hit = {
        .textureId = (xLen < yLen) ? xHit : yHit,
        .textureColumnOffset = (xLen < yLen) ? xTextureColumnCorrectedOffset : yTextureColumnCorrectedOffset,
        .shade = (xLen < yLen) ? 1.0f : 0.75f,
        .distance = (xLen < yLen) ? xLen : yLen,
        .coordinates = (xLen < yLen) ? xRay : yRay,
    };
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
    float delta = nowTime - prevTime;
    prevTime = nowTime;

    // Resize viewport
    if (IsWindowResized()) {
        V.width = GetRenderWidth();
        V.height = GetRenderHeight();
    }

    // Calculate the sign of the direction along which we are moving
    // on the x and y axis respectively
    float xSign = (P.rotation <= HALF_PI || P.rotation > 3 * HALF_PI) ? +1.0f : -1.0f;
    float ySign = (P.rotation >= 0.0f && P.rotation < PI) ? +1.0f : -1.0f;
    // Calculate the padding necessary to not slam the player into a wall
    float xPad = 0.125f * xSign;
    float yPad = 0.125f * ySign;
    // Calculate the distance delta on the x and y axis
    float x = cosf(P.rotation) * delta * P.movementSpeed;
    float y = sinf(P.rotation) * delta * P.movementSpeed;
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
            while (M->data[((int) P.position.y) * M->width + ((int) newPosition.x)]) {
                newPosition.x -= x * I.forward;
            }
            // Remember to subtract the padding once we are done
            // computing the corrected position along the x-axis
            P.position.x = newPosition.x - (xPad * I.forward);
        }
        // Check if the newPosition on the y-axis is inside the map
        if ((int) newPosition.y < M->height) {
            // Check for player longitudinal collision on the y-axis
            while (M->data[((int) newPosition.y) * M->width + ((int) P.position.x)]) {
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
            while (M->data[((int) P.position.y) * M->width + ((int) newPosition.x)]) {
                newPosition.x += y * I.right;
            }
            // Remember to subtract the padding once we are done
            // computing the corrected position along the x-axis
            P.position.x = newPosition.x + (yPad * I.right);
        }
        // Check if the newPosition on the y-axis is inside the map
        if ((int) newPosition.y < M->height) {
            // Check for player lateral collision on the y-axis
            while (M->data[((int) newPosition.y) * M->width + ((int) P.position.x)]) {
                newPosition.y -= x * I.right;
            }
            // Same as above, we have to subtract the padding once
            // we are done correcting the position along the y-axis
            P.position.y = newPosition.y - (xPad * I.right);
        }
    }

    // Camera horizontal rotation
    P.rotation += I.xMouseDelta * P.rotationSpeed * delta;
    if (P.rotation < 0.0f) {
        P.rotation += 2.0f * PI;
    } else if (P.rotation > 2.0f * PI) {
        P.rotation -= 2.0f * PI;
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
        // Trace the ray
        HitDetails hit = TraceHorizontalRay(&worldCoords, &tileCoords, angle);
        // Check if the ray hit a non empty cell
        if (hit.textureId) {
            // If we did, get the cell's texture
            RaycasterTexture *texture = Textures[hit.textureId - 1];
            // Calculate the height of the pixel column
            float lineHeight = M->maxWallHeight / (hit.distance * cosf(angle - P.rotation));
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
    //            int v = M->data[y * M->width + x];
    //            DrawRectangle(x * 64, y * 64, 64 - 1, 64 - 1, (v) ? GRAY : WHITE);
    //        }
    //    }
    //    for (int i = 0; i < rays; i++) {
    //        float angle = angleStart + i * angleStep;
    //        HitDetails hit = TraceHorizontalRay(&worldCoords, &tileCoords, angle);
    //        DrawLine(P.position.x * 64, P.position.y * 64, hit.coordinates.x * 64, hit.coordinates.y * 64, GREEN);
    //    }
    //    DrawLine(P.position.x * 64, P.position.y * 64, P.position.x * 64 + cosf(P.rotation) * 16, P.position.y * 64 + sinf(P.rotation) * 16, GREEN);
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
    prevTime = GetTime();
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
