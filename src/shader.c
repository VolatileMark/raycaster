#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stddef.h>

// Defaults
#define DEFAULT_WINDOW_TITLE        "RECOIL"
#define DEFAULT_VIEWPORT_WIDTH      1366
#define DEFAULT_VIEWPORT_HEIGHT     768
#define DEFAULT_VIEWPORT_DOF        16
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

struct Tile {
    int ceiling;
    int wall;
    int floor;
} __attribute__((packed));
typedef struct Tile Tile;

struct Map {
    int width;
    int height;
    float maxWallHeight;
    Tile data[];
} __attribute__((packed));
typedef struct Map Map;

struct Column {
    int textureId;
    float brightness;
    float lineHeight;
    float lineOffset;
    float textureColumnOffset;
} __attribute__((packed));
typedef struct Column Column;

struct Constants {
    int depthOfField;
    int viewportWidth;
    int viewportHeight;
    float viewportHalfHeight;
    float columnAngleStart;
    float columnAngleStep;
} __attribute__((packed));
typedef struct Constants Constants;

struct FrameData {
    Vector2 playerPosition;
    Vector2 playerDirection;
    Vector2 cameraPlane;
} __attribute__((packed));
typedef struct FrameData FrameData;

typedef struct {
    unsigned int ssboColumnsData;
    unsigned int ssboConstants;
    unsigned int ssboMapData;
    unsigned int ssboFrameData;
    unsigned int wallCompute;
    Shader renderPipeline;
    RenderTexture2D renderTexture;
} Shaders;

// Test data
static Map TestMap = {
    .width = 10,
    .height = 12,
    .maxWallHeight = 800.0f,
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
static Shaders S = {0};
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
#define absf(x) ((x < 0.0f) ? -x : x)

static void OnResize(void) {
    C.viewportHalfHeight = V.height / 2.0f;
    // Compute half the width of the camera plane
    C.cameraPlaneHalfWidth = 1.0f / (2.0f * tanf(V.fov / 2.0f));
    // Calculate the angle increment for each column ray
    C.columnAngleStep = V.fov / V.width;
    // Calculate the angle for the first column ray
    C.columnAngleStart = P.rotation - V.fov / 2.0f;
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
    ClearBackground(BLACK);
    // Draw the sky
    //DrawRectangle(0, 0, V.width, V.height / 2, BLUE);

    rlUpdateShaderBufferElements(S.ssboFrameData, &(FrameData) {
        .playerPosition = P.position,
        .playerDirection = C.playerDirection,
        .cameraPlane = C.cameraPlane
    }, sizeof(FrameData), 0);
    rlEnableShader(S.wallCompute);
    rlBindShaderBuffer(S.ssboColumnsData, 1);
    rlBindShaderBuffer(S.ssboConstants, 2);
    rlBindShaderBuffer(S.ssboMapData, 3);
    rlBindShaderBuffer(S.ssboFrameData, 4);
    rlComputeShaderDispatch((unsigned int) ceilf((float) V.width / 256), 1, 1);
    rlDisableShader();

    //Constants c;
    //rlReadShaderBufferElements(S.ssboConstants, &c, sizeof(Constants), 0);
    //TraceLog(LOG_INFO, "angleStart:%f angleStep:%f", c.columnAngleStart, c.columnAngleStep);
    //Column col[200];
    //rlReadShaderBufferElements(S.ssboColumnsData, &col, sizeof(Column) * 200, 0);
    //TraceLog(LOG_INFO, "(%f %f)", col[0].lineOffset, col[199].lineOffset);

    rlBindShaderBuffer(S.ssboColumnsData, 1);
    rlBindShaderBuffer(S.ssboConstants, 2);
    BeginShaderMode(S.renderPipeline);
    DrawTexture(S.renderTexture.texture, 0, 0, WHITE);
    EndShaderMode();

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
    // Initialize computed values
    OnResize();
    // Load fragment shader
    S.renderPipeline = LoadShader(NULL, "shaders/frag.glsl");
    // Load wall compute shader
    char *wallComputeSource = LoadFileText("shaders/wall.glsl");
    unsigned int wallComputeShader = rlCompileShader(wallComputeSource, RL_COMPUTE_SHADER);
    S.wallCompute = rlLoadComputeShaderProgram(wallComputeShader);
    UnloadFileText(wallComputeSource);
    // Create shader buffers
    S.ssboColumnsData = rlLoadShaderBuffer(sizeof(Column) * V.width, NULL, RL_DYNAMIC_COPY);
    S.ssboMapData = rlLoadShaderBuffer(sizeof(Map) + MAPSZ * sizeof(Tile), NULL, RL_STATIC_DRAW);
    S.ssboConstants = rlLoadShaderBuffer(sizeof(Constants), NULL, RL_STATIC_DRAW);
    S.ssboFrameData = rlLoadShaderBuffer(sizeof(FrameData), NULL, RL_DYNAMIC_DRAW);
    // Initialize buffers
    rlUpdateShaderBufferElements(S.ssboConstants, &(Constants) {
        .depthOfField = V.dof,
        .viewportWidth = V.width,
        .viewportHeight = V.height,
        .viewportHalfHeight = C.viewportHalfHeight,
        .columnAngleStart = C.columnAngleStart,
        .columnAngleStep = C.columnAngleStep,
    }, sizeof(Constants), 0);
    rlUpdateShaderBufferElements(S.ssboMapData, M, sizeof(Map) + MAPSZ * sizeof(Tile), 0);
    // Create render texture
    S.renderTexture = LoadRenderTexture(V.width, V.height);
    // Capture mouse
    DisableCursor();
    // Initialize previous frame time to now
    C.prevTime = GetTime();
}

static void Shutdown(void) {
    rlUnloadShaderBuffer(S.ssboFrameData);
    rlUnloadShaderBuffer(S.ssboMapData);
    rlUnloadShaderBuffer(S.ssboConstants);
    rlUnloadShaderBuffer(S.ssboColumnsData);
    rlUnloadShaderProgram(S.wallCompute);
    UnloadShader(S.renderPipeline);
    UnloadRenderTexture(S.renderTexture);
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
