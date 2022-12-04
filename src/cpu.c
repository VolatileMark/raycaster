#include <raylib.h>
#include <raymath.h>

// Defaults
#define DEFAULT_WINDOW_TITLE        "RECOIL"
#define DEFAULT_VIEWPORT_WIDTH      1366
#define DEFAULT_VIEWPORT_HEIGHT     768
#define DEFAULT_VIEWPORT_DOF        16
#define DEFAULT_VIEWPORT_FOV        (66.0f * DEG2RAD)
#define DEFAULT_VIEWPORT_SCALING    0.25f

typedef struct {
    int width;
    int height;
    int dof;
    float fov;
    float scaling;
} Viewport;

typedef struct {
    int columns;
    int rows;
    float prevTime;
    float viewportHalfHeight;
    float cameraPlaneHalfWidth;
    float columnAngleStart;
    float columnAngleStep;
    float columnPixelWidth;
    float rowPixelHeight;
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
    float wallHeight;
    Tile data[];
} Map;

// Test data
static Map TestMap = {
    .width = 10,
    .height = 12,
    .wallHeight = 800.0f,
    .data = {
        {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0},
        {0, 1, 0}, {1, 0, 1}, {1, 0, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 0, 1}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 0, 1}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 0, 0}, {1, 0, 1}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {1, 0, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
        {0, 1, 0}, {1, 0, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
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
static Viewport V = {
    .width = DEFAULT_VIEWPORT_WIDTH,
    .height = DEFAULT_VIEWPORT_HEIGHT,
    .scaling = DEFAULT_VIEWPORT_SCALING,
    .dof = DEFAULT_VIEWPORT_DOF,
    .fov = DEFAULT_VIEWPORT_FOV,
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
#define MAPSZ   (M->width * M->height)
#define HALF_PI (PI / 2.0f)
#define absf(x) ((x < 0.0f) ? -x : x)

static void RecomputeValues(void) {
    C.viewportHalfHeight = V.height / 2.0f;
    // Compute the number of rays and scanlines necessary to
    // draw the full screen
    int rays = (int) (V.width * V.scaling);
    int scanlines = (int) (V.height * V.scaling);
    C.columns = (V.width % rays) ? (rays + 1) : rays;
    C.rows = (V.height % scanlines) ? (scanlines + 1) : scanlines;
    // Compute half the width of the camera plane
    C.cameraPlaneHalfWidth = 1.0f / (2.0f * tanf(V.fov / 2.0f));
    // Calculate the angle increment for each column ray
    C.columnAngleStep = V.fov / C.columns;
    // Calculate the angle for the first column ray
    C.columnAngleStart = P.rotation - V.fov / 2.0f;
    // Calculate the width of each pixel in a column
    C.columnPixelWidth = (float) V.width / C.columns;
    // Calculate the width of each pixel in a row
    C.rowPixelHeight = (float) V.height / C.rows;
}

static void DrawRow(Vector2 cameraPlaneLeft, Vector2 cameraPlaneRight, int n) {
    // Calculate the row's y pixel position on the screen
    float y = n * C.rowPixelHeight;
    // Calculate how many pixel aways the row is from the horizon
    int pixelsFromHorizon = C.viewportHalfHeight - y;
    // Compute the distance of the pixel from the camera plane
    // (the further the row is from the center of the screen,
    // the closer it should be to the camera)
    // distance = +1 <== pixelsFromHorizon = C.viewportHalfHeight <== y = 0
    // distance = +inf <== pixelsFromHorizon = 0 <== y = C.viewportHalfHeight
    float distance = C.viewportHalfHeight / pixelsFromHorizon;
    // Compute the step for each pixel in the row
    Vector2 step = Vector2Scale(Vector2Subtract(cameraPlaneRight, cameraPlaneLeft), distance / C.columns);
    // Compute the starting position
    Vector2 position = Vector2Add(P.position, Vector2Scale(cameraPlaneLeft, distance));
    for (int x = 0; x < C.columns; x++) {
        // Get the current cell position
        int cellX = (int) position.x;
        int cellY = (int) position.y;
        // Compute the map offset and check if it's
        // in inside the map
        int cellOffset = cellY * M->width + cellX;
        if (cellOffset >= 0 && cellOffset < MAPSZ) {
            // Draw the ceiling first and the the floor
            for (int floor = 0; floor < 2; floor++) {
                // Get the current cell id
                int cellId = (floor) ? M->data[cellOffset].floor : M->data[cellOffset].ceiling;
                // If the cell is empty, skip the draw call
                if (!cellId) {
                    continue;
                }
                // If it's not, get the texture
                TileTexture *texture = T[cellId - 1];
                // Compute the texture coordinates
                int textureX = texture->width * (position.x - (float) cellX);
                int textureY = texture->height * (position.y - (float) cellY);
                // Compute the offset inside the texture and check if it's in bounds
                int textureOffset = textureY * texture->width + textureX;
                if (textureOffset < 0 || textureOffset >= texture->width * texture->height) {
                    continue;
                }
                // Sample the texture
                Color color = (texture->data[textureOffset]) ? RED : GREEN;
                // Draw the pixel onto the screen
                int xStart = x * C.columnPixelWidth;
                int yStart = (floor) ? (V.height - y) : y;
                DrawRectangle(xStart, yStart, ceilf(C.columnPixelWidth), ceilf(C.rowPixelHeight), color);
            }
        }
        // Step the ray
        position.x += step.x;
        position.y += step.y;
    }
}

static void DrawColumn(Vector2 *worldCoords, Vector2 *tileCoords, int n) {
    // Compute the angle of the ray
    float angle = C.columnAngleStart + n * C.columnAngleStep;
    // cameraX = coordinate (between -1 and 1) of the ray on the x-axis
    //           of the camera plane
    float cameraX = (2.0f * ((float) n / C.columns)) - 1.0f; 
    
    // Clamp angle between 0 and 2*PI
    if (angle < 0.0f) {
        angle += 2 * PI;
    } else if (angle > 2 * PI) {
        angle -= 2 * PI;
    }

    // Compute the ray direction
    Vector2 rayDirection = Vector2Add(C.playerDirection, Vector2Scale(C.cameraPlane, cameraX));
    
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
    int cellId = 0;
    bool vertical = true;
    for (int i = 0; i < V.dof; i++) {
        int mapOffset = mapY * M->width + mapX;
        if (mapOffset >= 0 && mapOffset < MAPSZ && (cellId = M->data[mapOffset].wall)) {
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

    // Check if the ray hit an empty cell
    if (!cellId) {
        return;
    }
    // If we didn't, get that cell's texture
    TileTexture *texture = T[cellId - 1];

    // Get the hit information for the first ray
    // to hit a wall
    float rayDistance, colorBrightness;
    if (vertical) {
        // yDeltaDistance is subracted from the total to
        // obtain the distance projected onto the camera direction
        // This is done to fix the fisheye effect
        rayDistance = yIntersectionDistance - yDeltaDistance;
        colorBrightness = 1.0f;
    } else {
        rayDistance = xIntersectionDistance - xDeltaDistance;
        colorBrightness = 0.75f;
    }
    Vector2 coordinates = Vector2Add(P.position, Vector2Scale(rayDirection, rayDistance));
    float textureColumnOffset;
    // Compute the correct offset for the column in the texture
    if (vertical) {
        textureColumnOffset = coordinates.y - (float) mapY;
        textureColumnOffset = (stepX > 0) ? textureColumnOffset : (1.0f - textureColumnOffset);
    } else {
        textureColumnOffset = coordinates.x - (float) mapX;
        textureColumnOffset = (stepY < 0) ? textureColumnOffset : (1.0f - textureColumnOffset);
    }

    // Calculate the height of the pixel column
    float lineHeight = M->wallHeight / rayDistance;
    // Calculate the height of each pixel in the column
    float dotHeight = (float) lineHeight / texture->height;
    // Find the corresponding texture column
    int textureColumn = textureColumnOffset * texture->width;
    // Clip the height of the column if it's higher than the
    // viewport's height and offset the texture 
    float textureOffset = 0.0f;
    if (lineHeight > V.height) {
        textureOffset = (lineHeight - V.height) / 2.0f;
        lineHeight = V.height;
    }
    // Compute the starting point of the column
    float xStart = n * C.columnPixelWidth;
    float yStart = C.viewportHalfHeight - lineHeight / 2.0f - textureOffset;
    // Draw each pixel (of size C.columnPixelWidth x dotHeight)
    // in the texture column
    for (int i = 0; i < texture->height; i++) {
        Color color = (texture->data[i * texture->width + textureColumn]) ? GOLD : WHITE;
        // Shade the color accordingly
        color.r *= colorBrightness;
        color.g *= colorBrightness;
        color.b *= colorBrightness;
        // Draw the i-th pixel
        DrawRectangle(xStart, yStart + roundf(i * dotHeight), ceilf(C.columnPixelWidth), ceilf(dotHeight), color);
    }
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
        RecomputeValues();
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

    // Compute left most pixel position
    Vector2 cameraPlaneLeft = Vector2Subtract(C.playerDirection, C.cameraPlane);
    // Compute right most pixel position
    Vector2 cameraPlaneRight = Vector2Add(C.playerDirection, C.cameraPlane);
    // Compute the cell coordinates
    Vector2 worldCoords = { .x = (int) P.position.x, .y = (int) P.position.y };
    // Compute the coordinates inside the cell
    Vector2 tileCoords = Vector2Subtract(P.position, worldCoords);
    // Draw floors and ceilings
    for (int r = 0; r < C.rows / 2; r++) {
        DrawRow(cameraPlaneLeft, cameraPlaneRight, r);
    }
    // Draw walls
    for (int c = 0; c < C.columns; c++) {
        DrawColumn(&worldCoords, &tileCoords, c);
    }

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
    RecomputeValues();
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
