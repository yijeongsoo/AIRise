#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include <vector>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Entity.h"

#define FLOOR_COUNT 11
#define UNDERLING_COUNT 4
#define MIDBOSS_COUNT 1


struct GameState {
    Entity *player;
    Entity* laser;
    Entity* floors;
    Entity* underlings;
    Entity* midboss;
    Entity* boss;
    Entity* text;
};

GameState state;

Mix_Music* music;

SDL_Window* displayWindow;
bool gameIsRunning = true;
bool stopGame = false;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);
    
    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);
    return textureID;
}

void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position) {
    float width = 1.0f / 16.0f;
    float height = 1.0f / 16.0f;
    std::vector<float> vertices;
    std::vector<float> texCoords;
    for (int i = 0; i < text.size(); i++) {
        int index = (int)text[i];
        float offset = (size + spacing) * i;
        float u = (float)(index % 16) / 16.0f;
        float v = (float)(index / 16) / 16.0f;
        vertices.insert(vertices.end(), {
            offset + (-0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
            });
        texCoords.insert(texCoords.end(), {
            u, v,
            u, v + height,
            u + width, v,
            u + width, v + height,
            u + width, v,
            u, v + height,
            });
    } // end of for loop

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    program->SetModelMatrix(modelMatrix);

    glUseProgram(program->programID);

    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);

    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
    glEnableVertexAttribArray(program->texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    displayWindow = SDL_CreateWindow("AIRise!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(0, 0, 640, 480);
    
    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4996);
    music = Mix_LoadMUS("dooblydoo.mp3");
    Mix_PlayMusic(music, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 3);
    
    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    
    glUseProgram(program.programID);
    
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
    
    //Initialize text
    state.text = new Entity();
    GLuint textTextureID = LoadTexture("font.png");
    state.text->textureID = textTextureID;

    // Initialize floors
    state.floors = new Entity[FLOOR_COUNT];

    GLuint floorTextureID = LoadTexture("floorTile.png");

    for (int i = 0; i < FLOOR_COUNT; i++) {
        state.floors[i].entityType = FLOOR;
        state.floors[i].textureID = floorTextureID;
        state.floors[i].position = glm::vec3(-5 + i, -3.25f, 0);
    }

    //Platforms will only be updated once
    for (int i = 0; i < FLOOR_COUNT; i++) {
        state.floors[i].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0, NULL);
    }
   
    // Initialize Game Objects
    
    // Initialize Player
    state.player = new Entity();
    state.player->entityType = PLAYER;
    state.player->position = glm::vec3(0,0,0); // Start from middle
    state.player->movement = glm::vec3(0);
    state.player->acceleration = glm::vec3(0, -9.81, 0); //Setting Acceleration to Gravity
    state.player->speed = 1.5f;
    state.player->jumpPower = 5.0f;
    state.player->textureID = LoadTexture("player.png");
    
    state.player->animRight = new int[4] {3, 7, 11, 15};
    state.player->animLeft = new int[4] {1, 5, 9, 13};
    state.player->animUp = new int[4] {2, 6, 10, 14};
    state.player->animDown = new int[4] {0, 4, 8, 12};

    state.player->height = 0.8f;
    state.player->width = 0.6f;

    state.player->animIndices = state.player->animRight;
    state.player->animFrames = 4;
    state.player->animIndex = 0;
    state.player->animTime = 0;
    state.player->animCols = 4;
    state.player->animRows = 4;

    // Initialize Underling
    state.underlings = new Entity[UNDERLING_COUNT];
    GLuint underlingTextureID = LoadTexture("enemies.png");

    //Underling - White Ghost

        //Features common for all underlings
    for (int i = 0; i < UNDERLING_COUNT; i++) {
            //Image Selection
        if (state.underlings[i].aiState == ATTACKING) {
            state.underlings[i].animRight = new int[3]{ 28, 29, 30 };
            state.underlings[i].animLeft = new int[3]{ 16, 17, 18 };
            state.underlings[i].animUp = new int[3]{ 40, 41, 42 };
            state.underlings[i].animDown = new int[3]{ 4, 5, 6 };
        }
        else {
            state.underlings[i].animRight = new int[3]{ 25, 26, 27 };
            state.underlings[i].animLeft = new int[3]{ 13, 14, 15 };
            state.underlings[i].animUp = new int[3]{ 37, 38, 39 };
            state.underlings[i].animDown = new int[3]{ 1, 2, 3 };
        }

        state.underlings[i].entityType = UNDERLING;
        state.underlings[i].aiType = UNDERLINGAI;
        state.underlings[i].aiState = IDLE;
        state.underlings[i].textureID = underlingTextureID;
        state.underlings[i].speed = 0.1f;
        state.underlings[i].acceleration = glm::vec3(0, -1.0f, 0); //Setting Acceleration to Gravity
        state.underlings[i].animIndices = state.underlings[i].animDown;
        state.underlings[i].animFrames = 3;
        state.underlings[i].animIndex = 0;
        state.underlings[i].animTime = 0;
        state.underlings[i].animCols = 12;
        state.underlings[i].animRows = 8;
        state.underlings[i].width = 0.8f;


    }

    //Starting Position
    state.underlings[0].position = glm::vec3(-2.5f, 2.25f, 0);
    state.underlings[1].position = glm::vec3(-4.5f, 2.25f, 0);
    state.underlings[2].position = glm::vec3(2.5f, 2.25f, 0);
    state.underlings[3].position = glm::vec3(4.5f, 2.25f, 0);

    // Initialize Mid-Boss
    state.midboss = new Entity[MIDBOSS_COUNT];
    GLuint midbossTextureID = LoadTexture("enemies.png");

    //Mid-Boss - Green Ghost
    for (int i = 0; i < MIDBOSS_COUNT; i++) {
        //Image Selection
        if (state.midboss[i].aiState == ATTACKING) {
            state.midboss[i].animRight = new int[3]{ 34, 35, 36 };
            state.midboss[i].animLeft = new int[3]{ 22, 23, 24 };
            state.midboss[i].animUp = new int[3]{ 46, 47, 48 };
            state.midboss[i].animDown = new int[3]{ 10, 11, 12 };
        }
        else {
            state.midboss[i].animRight = new int[3]{ 31, 32, 33 };
            state.midboss[i].animLeft = new int[3]{ 19, 20, 21 };
            state.midboss[i].animUp = new int[3]{ 43, 44, 45 };
            state.midboss[i].animDown = new int[3]{ 7, 8, 9 };
        }

        state.midboss[i].entityType = MIDBOSS;
        state.midboss[i].aiType = MIDBOSSAI;
        state.midboss[i].aiState = IDLE;
        state.midboss[i].isActive = false;
        state.midboss[i].textureID = midbossTextureID;
        state.midboss[i].speed = 0.2f;
        state.midboss[i].jumpPower = 1.5f;
        state.midboss[i].acceleration = glm::vec3(0, -5.0, 0); //Setting Acceleration to Gravity
        state.midboss[i].animIndices = state.midboss[i].animDown;
        state.midboss[i].animFrames = 3;
        state.midboss[i].animIndex = 0;
        state.midboss[i].animTime = 0;
        state.midboss[i].animCols = 12;
        state.midboss[i].animRows = 8;
        state.midboss[i].width = 0.8f;


    }

    //Starting Position
    state.midboss[0].position = glm::vec3(4.5f, 5.0f, 0);

    //Boss - Black Ghost
    // Initialize Boss
    state.boss = new Entity();
    GLuint bossTextureID = LoadTexture("enemies.png");
    state.boss->entityType = BOSS;
    state.boss->aiType = BOSSAI;
    state.boss->aiState = IDLE;
    state.boss->textureID = bossTextureID;
    state.boss->isActive = false;
    state.boss->speed = 0.3f;
    state.boss->jumpPower = 2.0f;
    state.boss->acceleration = glm::vec3(0, -5.0, 0); //Setting Acceleration to Gravity
    state.boss->animFrames = 3;
    state.boss->animIndex = 0;
    state.boss->animTime = 0;
    state.boss->animCols = 12;
    state.boss->animRows = 8;
    state.boss->width = 0.8f;

        //Image Selection
    if (state.boss->aiState == ATTACKING) {
        state.boss->animRight = new int[3]{ 82, 83, 84 };
        state.boss->animLeft = new int[3]{ 70, 71, 72 };
        state.boss->animUp = new int[3]{ 94, 95, 96 };
        state.boss->animDown = new int[3]{ 58, 59, 60 };
    }
    else {
        state.boss->animRight = new int[3]{ 79, 80, 81 };
        state.boss->animLeft = new int[3]{ 67, 68, 69 };
        state.boss->animUp = new int[3]{ 91, 92, 93 };
        state.boss->animDown = new int[3]{ 55, 56, 57 };
    }

    state.boss->animIndices = state.boss->animDown;

        //Starting Position
    state.boss->position = glm::vec3(4.25f, 10.0f, 0);
}

void ProcessInput() {
    
    state.player->movement = glm::vec3(0);
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                gameIsRunning = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                        // Move the player left
                        break;
                        
                    case SDLK_RIGHT:
                        // Move the player right
                        break;
                         
                    case SDLK_SPACE:
                        if (state.player->collidedBottom) {
                            state.player->jump = true;
                        }
                        break;
                }
                break; // SDL_KEYDOWN
        }
    }
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (stopGame == false) {
        if (keys[SDL_SCANCODE_LEFT]) {
            state.player->movement.x = -1.0f;
            state.player->animIndices = state.player->animLeft;
        }
        else if (keys[SDL_SCANCODE_RIGHT]) {
            state.player->movement.x = 1.0f;
            state.player->animIndices = state.player->animRight;
        }

        //if (keys[SDL_SCANCODE_UP]) {
        //   state.player->movement.y = 1.0f;
        //   state.player->animIndices = state.player->animUp;
        //}
        //else if (keys[SDL_SCANCODE_DOWN]) {
        //   state.player->movement.y = -1.0f;
        //   state.player->animIndices = state.player->animDown;
        //}

        if (glm::length(state.player->movement) > 1.0f) {
            state.player->movement = glm::normalize(state.player->movement);
        }
    }
}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    deltaTime += accumulator;
    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }
    while (deltaTime >= FIXED_TIMESTEP && stopGame == false) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime

        state.player->Update(FIXED_TIMESTEP, state.player, state.floors, FLOOR_COUNT, state.underlings, UNDERLING_COUNT, state.midboss, MIDBOSS_COUNT, state.boss);

        for (int i = 0; i < UNDERLING_COUNT; i++) {
            state.underlings[i].Update(FIXED_TIMESTEP, state.player, state.floors, FLOOR_COUNT, state.underlings, UNDERLING_COUNT, state.midboss, MIDBOSS_COUNT, state.boss);

        }

        for (int i = 0; i < MIDBOSS_COUNT; i++) {
            state.midboss[i].Update(FIXED_TIMESTEP, state.player, state.floors, FLOOR_COUNT, state.underlings, UNDERLING_COUNT, state.midboss, MIDBOSS_COUNT, state.boss);
            if (state.player->lastCollision == MIDBOSS && state.player->collidedBottom == true) {
                state.player->jump = true;
                state.midboss[i].aiState = DEAD;
            }
        }

        state.boss->Update(FIXED_TIMESTEP, state.player, state.floors, FLOOR_COUNT, state.underlings, UNDERLING_COUNT, state.midboss, MIDBOSS_COUNT, state.boss);
        if (state.player->lastCollision == BOSS && state.player->collidedBottom == true) {
            state.player->jump = true;
            state.boss->aiState = DEAD;
        }

        deltaTime -= FIXED_TIMESTEP;
    }
    accumulator = deltaTime;
}


void Render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    for (int i = 0; i < FLOOR_COUNT; i++) {
        state.floors[i].Render(&program);
    }

    state.player->Render(&program);

    for (int i = 0; i < UNDERLING_COUNT; i++) {
        state.underlings[i].Render(&program);
    }

    for (int i = 0; i < MIDBOSS_COUNT; i++) {
        state.midboss[i].Render(&program);
    }

    state.boss->Render(&program);

    if ((state.player->lastCollision == UNDERLING || state.player->lastCollision == MIDBOSS || state.player->lastCollision == BOSS) && (state.player->collidedLeft == true || state.player->collidedRight == true)) {
        DrawText(&program, state.text->textureID, "GAME OVER", 0.7f, -0.25f, glm::vec3(-2.0, 0, 0));
        stopGame = true;
    }
    else if (state.boss->aiState == DEAD) {
        state.boss->isActive = false;
        DrawText(&program, state.text->textureID, "LEVEL COMPLETE", 0.7f, -0.25f, glm::vec3(-2.0f, 0, 0));
        stopGame = true;
    }


    SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();
    
    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }
    
    Shutdown();
    return 0;
}
