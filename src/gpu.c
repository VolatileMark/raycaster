#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stddef.h>

// Defaults
#define DEFAULT_WINDOW_TITLE        "RECOIL"
#define DEFAULT_VIEWPORT_WIDTH      1366
#define DEFAULT_VIEWPORT_HEIGHT     768
#define DEFAULT_VIEWPORT_DOF        32
#define DEFAULT_VIEWPORT_FOV        (66.0f * DEG2RAD)

typedef struct {
    int width;
    int height;
    int dof;
    float fov;
} Viewport;

typedef struct {
    float prevTime;
    float viewportHalfHeight;
    float cameraPlaneHalfWidth;
    float columnAngleStart;
    float columnAngleStep;
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
    int width;
    int height;
    int data[];
} TileTexture;

typedef struct {
    int ceiling;
    int wall;
    int floor;
    int padding;
} Tile;

typedef struct {
    int width;
    int height;
    Tile data[];
} Map;

typedef struct {
    int padding;
    int textureId;
    float brightness;
    float lineHeight;
    float lineOffset;
    float textureColumnOffset;
} Column;

typedef struct {
    int depthOfField;
    int viewportWidth;
    int viewportHeight;
    float viewportHalfHeight;
    float columnAngleStart;
    float columnAngleStep;
    int tileSize[2];
    int tileMapSize[2];
} Constants;

typedef struct {
    Vector2 playerPosition;
    int playerMapCoords[2];
    Vector2 playerTileCoords;
    Vector2 playerDirection;
    Vector2 cameraPlane;
} FrameData;

typedef struct {
    int tileMapLocation;
    unsigned int ssboColumnsData;
    unsigned int ssboConstants;
    unsigned int ssboMapData;
    unsigned int ssboFrameData;
    unsigned int wallCompute;
    Shader renderPipeline;
    RenderTexture2D renderTexture;
    Texture2D tileMapTexture;
} Graphics;

// Test data
static Map TestMap = {
    .width = 10,
    .height = 20,
    .data = {
        {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0}, {0, 0, 2, 0}, {0, 0, 2, 0}, {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0}, {0, 0, 2, 0}, {0, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {0, 0, 2, 0}, {0, 0, 2, 0}, {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0}, {0, 0, 2, 0}, {0, 0, 2, 0}, {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 0, 2, 0}, {2, 1, 2, 0},
        {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0}, {2, 1, 2, 0},
    }
};

// Singletons
static Graphics G = {0};
static Computed C = {0};
static PlayerInput I = {false};
static Map *M = &TestMap;
static Viewport V = {
    .width = DEFAULT_VIEWPORT_WIDTH,
    .height = DEFAULT_VIEWPORT_HEIGHT,
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
#define MAPSZ (M->width * M->height)
#define HALF_PI (PI / 2.0f)

static void OnResize(void) {
    // Update viewport size
    V.width = GetRenderWidth();
    V.height = GetRenderHeight();
    // Compute the vertical center of the viewport
    C.viewportHalfHeight = V.height / 2.0f;
    // Compute half the width of the camera plane
    C.cameraPlaneHalfWidth = 1.0f / (2.0f * tanf(V.fov / 2.0f));
    // Calculate the angle increment for each column ray
    C.columnAngleStep = V.fov / V.width;
    // Calculate the angle for the first column ray
    C.columnAngleStart = P.rotation - V.fov / 2.0f;
    // Recreate columns shader buffer
    if (G.ssboColumnsData) {
        rlUnloadShaderBuffer(G.ssboColumnsData);
    }
    G.ssboColumnsData = rlLoadShaderBuffer(sizeof(Column) * V.width, NULL, RL_DYNAMIC_COPY);
    // Update shader buffers
    rlUpdateShaderBufferElements(G.ssboConstants, &(Constants) {
        .depthOfField = V.dof,
        .viewportWidth = V.width,
        .viewportHeight = V.height,
        .viewportHalfHeight = C.viewportHalfHeight,
        .columnAngleStart = C.columnAngleStart,
        .columnAngleStep = C.columnAngleStep,
        .tileSize = { 16, 16 },
        .tileMapSize = { 16, 16 }
    }, sizeof(Constants), 0);
    // Recreate the render texture
    if (G.renderTexture.id) {
        UnloadRenderTexture(G.renderTexture);
    }
    G.renderTexture = LoadRenderTexture(V.width, V.height);
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
        OnResize();
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
    // Compute the starting map coordinates
    Vector2 worldCoords = { (int) P.position.x, (int) P.position.y };
    // Compute the player's coordinates inside the tile
    Vector2 tileCoords = Vector2Subtract(P.position, worldCoords);

    rlUpdateShaderBufferElements(G.ssboFrameData, &(FrameData) {
        .playerPosition = P.position,
        .playerMapCoords = { (int) worldCoords.x, (int) worldCoords.y },
        .playerTileCoords = tileCoords,
        .playerDirection = C.playerDirection,
        .cameraPlane = C.cameraPlane,
    }, sizeof(FrameData), 0);

    rlBindShaderBuffer(G.ssboColumnsData, 1);
    rlBindShaderBuffer(G.ssboConstants, 2);
    rlBindShaderBuffer(G.ssboMapData, 3);
    rlBindShaderBuffer(G.ssboFrameData, 4);

    // Compute shader
    rlEnableShader(G.wallCompute);
    rlComputeShaderDispatch((unsigned int) ceilf((float) V.width / 256), 1, 1);
    rlDisableShader();
    // Fragment shader
    BeginShaderMode(G.renderPipeline);
    SetShaderValueTexture(G.renderPipeline, G.tileMapLocation, G.tileMapTexture);
    DrawTexture(G.renderTexture.texture, 0, 0, WHITE);
    EndShaderMode();

    //#define COLUMNS 256
    //Column col[COLUMNS];
    //rlReadShaderBufferElements(S.ssboColumnsData, &col, sizeof(Column) * COLUMNS, 0);
    //for (int y = 0; y < M->height; y++) {
    //    for (int x = 0; x < M->width; x++) {
    //        if (M->data[y * M->width + x].wall) {
    //            DrawRectangle(x * 64, y * 64, 64 - 1, 64 - 1, GRAY);
    //        } else {
    //            DrawRectangle(x * 64, y * 64, 64 - 1, 64 - 1, WHITE);
    //        }
    //    }
    //}
    //for (int i = 0; i < COLUMNS - 1; i++) {
    //    DrawLineV(Vector2Scale(P.position, 64), Vector2Scale(col[i].intersection, 64), GREEN);
    //}
    //DrawLineEx(Vector2Scale(P.position, 64), Vector2Scale(col[COLUMNS - 1].intersection, 64), 4.0f, BLACK);
    //DrawRectangle(P.position.x * 64 - 4, P.position.y * 64 - 4, 8, 8, RED);
    //DrawLine(P.position.x * 64, P.position.y * 64, (P.position.x + C.playerDirection.x) * 64, (P.position.y + C.playerDirection.y) * 64, DARKBLUE);
    //DrawLine((P.position.x + C.playerDirection.x) * 64, (P.position.y + C.playerDirection.y) * 64,(P.position.x + C.playerDirection.x + C.cameraPlane.x) * 64, (P.position.y + C.playerDirection.y + C.cameraPlane.y) * 64, GOLD);
    //DrawLine((P.position.x + C.playerDirection.x) * 64, (P.position.y + C.playerDirection.y) * 64,(P.position.x + C.playerDirection.x - C.cameraPlane.x) * 64, (P.position.y + C.playerDirection.y - C.cameraPlane.y) * 64, GOLD);

    DrawFPS(10, 10);
}

static void Init(void) {
    // Create window
    InitWindow(
        V.width, 
        V.height, 
        DEFAULT_WINDOW_TITLE
    );
    // Make window resizable
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    // Load tilemap
    G.tileMapTexture = LoadTexture("assets/textures/tilemap.png");
    // Load fragment shader
    G.renderPipeline = LoadShader(NULL, "shaders/frag.glsl");
    // Load wall compute shader
    char *wallComputeSource = LoadFileText("shaders/wall.glsl");
    unsigned int wallComputeShader = rlCompileShader(wallComputeSource, RL_COMPUTE_SHADER);
    G.wallCompute = rlLoadComputeShaderProgram(wallComputeShader);
    UnloadFileText(wallComputeSource);
    // Create shader buffers (S.ssboColumnData is created in OnResize())
    G.ssboMapData = rlLoadShaderBuffer(sizeof(Map) + MAPSZ * sizeof(Tile), NULL, RL_STATIC_DRAW);
    G.ssboConstants = rlLoadShaderBuffer(sizeof(Constants), NULL, RL_STATIC_DRAW);
    G.ssboFrameData = rlLoadShaderBuffer(sizeof(FrameData), NULL, RL_DYNAMIC_DRAW);
    // Get tilemap uniform location
    G.tileMapLocation = GetShaderLocation(G.renderPipeline, "tileMap");
    // Initialize computed values
    OnResize();
    // Initialize buffers (S.ssboConstants is initialized in OnResize())
    rlUpdateShaderBufferElements(G.ssboMapData, M, sizeof(Map) + MAPSZ * sizeof(Tile), 0);
    // Capture mouse
    DisableCursor();
    // Initialize previous frame time to now
    C.prevTime = GetTime();
}

static void Shutdown(void) {
    rlUnloadShaderBuffer(G.ssboFrameData);
    rlUnloadShaderBuffer(G.ssboMapData);
    rlUnloadShaderBuffer(G.ssboConstants);
    rlUnloadShaderBuffer(G.ssboColumnsData);
    rlUnloadShaderProgram(G.wallCompute);
    UnloadShader(G.renderPipeline);
    UnloadRenderTexture(G.renderTexture);
    UnloadTexture(G.tileMapTexture);
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
