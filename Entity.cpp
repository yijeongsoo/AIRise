#include "Entity.h"

Entity::Entity()
{
    position = glm::vec3(0);
    movement = glm::vec3(0);
    acceleration = glm::vec3(0);
    velocity = glm::vec3(0);
    speed = 0;
    
    modelMatrix = glm::mat4(1.0f);
}

bool Entity::CheckCollision(Entity* other) {
    if (isActive == false || other->isActive == false) return false;

    float xdist = fabs(position.x - other->position.x) - ((width + other->width) / 2.0f);
    float ydist = fabs(position.y - other->position.y) - ((height + other->height) / 2.0f);

    if (xdist < 0 && ydist < 0) {
        lastCollision = other->entityType;
        return true;
    } 
    return false;
}


void Entity::CheckCollisionsY(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity* object = &objects[i];
        if (CheckCollision(object))
        {
            float ydist = fabs(position.y - object->position.y);
            float penetrationY = fabs(ydist - (height / 2.0f) - (object->height / 2.0f));
            if (velocity.y > 0) {
                position.y -= penetrationY;
                velocity.y = 0;
                collidedTop = true;
            }
            else if (velocity.y < 0) {
                position.y += penetrationY;
                velocity.y = 0;
                collidedBottom = true;
                if (this->entityType == PLAYER && (objects[i].entityType == UNDERLING || objects[i].entityType == MIDBOSS || objects[i].entityType == BOSS)) {
                    objects[i].aiState = DEAD;
                }
            }
        }
    }
}

void Entity::CheckCollisionsX(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity* object = &objects[i];
        if (CheckCollision(object))
        {
            float xdist = fabs(position.x - object->position.x);
            float penetrationX = fabs(xdist - (width / 2.0f) - (object->width / 2.0f));
            if (velocity.x > 0) {
                position.x -= penetrationX;
                velocity.x = 0;
                collidedRight = true;
            }
            else if (velocity.x < 0) {
                position.x += penetrationX;
                velocity.x = 0;
                collidedLeft = true;
            }
        }
    }
}

void Entity::AIUnderling(Entity* player, Entity* floors, int floorCount) {
    switch (aiState) {
        case DEAD:
            isActive = false;
            position.y = -10.0f;
            break;

        case IDLE:
            if (glm::distance(position, player->position) < 3.0f) {
                aiState = ATTACKING;
            }
            break;

        case ATTACKING:
            if (player->position.x < position.x) {
                movement.x = -1;
            }
            else {
                movement.x = 1;
            }
            break;

    }
}

void Entity::AIMidboss(Entity* player, Entity* floors, int floorCount) {
    switch (aiState) {
        case DEAD:
            isActive = false;
            position.y = -10.0f;
            break;
        
        case IDLE:
            if (glm::distance(position, player->position) < 4.0f) {
                aiState = ATTACKING;

            }
            break;

        case ATTACKING:
            if (player->position.x < position.x) {
                movement.x = -1;
            }
            else {
                movement.x = 1;
            }
            break;
    }
}

void Entity::AIBoss(Entity* player, Entity* floors, int floorCount) {
    switch (aiState) {
        case DEAD:
            isActive = false;
            position.y = -10.0f;
            break;

        case IDLE:
            if (glm::distance(position, player->position) < 5.0f) {
                aiState = ATTACKING;
          
            }
            break;

        case ATTACKING:
            if (player->position.x < position.x) {
                movement.x = -1;
            }
            else {
                movement.x = 1;
            }
            break;
    }
}

void Entity::AI(Entity* player, Entity* floors, int floorCount) {
    switch (aiType) {
        case UNDERLINGAI:
            AIUnderling(player, floors, floorCount);
            break;

        case MIDBOSSAI:
            AIMidboss(player, floors, floorCount);
            break;

        case BOSSAI:
            AIBoss(player, floors, floorCount);
            break;
    }
}



void Entity::Update(float deltaTime, Entity* player, Entity* floors, int floorCount, Entity* underlings, int underlingsCount, Entity* midboss, int midbossCount, Entity* boss)
{
    if (entityType == MIDBOSS) {
        int deadCount = 0;
        for (int i = 0; i < underlingsCount; i++) {
            if (underlings[i].aiState == DEAD) {
                deadCount++;
            }
        }
        if (deadCount == underlingsCount) {
            isActive = true;
        }
    }

    if (entityType == BOSS) {
        int deadCount = 0;
        for (int i = 0; i < midbossCount; i++) {
            if (midboss[i].aiState == DEAD) {
                deadCount++;
            }
        }
        if (deadCount == midbossCount) {
            isActive = true;
        }
    }

    if (isActive == false) return;

    collidedTop = false;
    collidedBottom = false;
    collidedLeft = false;
    collidedRight = false;

    if (entityType == UNDERLING || entityType == BOSS || entityType == MIDBOSS) {
        AI(player, floors, floorCount);
    }

    if (animIndices != NULL) {
        if (glm::length(movement) != 0) {
            animTime += deltaTime;

            if (animTime >= 0.25f)
            {
                animTime = 0.0f;
                animIndex++;
                if (animIndex >= animFrames)
                {
                    animIndex = 0;
                }
            }
        } else {
            animIndex = 0;
        }
    }

    if (jump) {
        jump = false;
        velocity.y += jumpPower;
    }


    velocity.x = movement.x * speed;
    velocity += acceleration * deltaTime;

    position.y += velocity.y * deltaTime; // Move on Y
    CheckCollisionsY(floors, floorCount);// Fix if needed
    if ((entityType == UNDERLING || entityType == MIDBOSS || entityType == BOSS) && aiState == ATTACKING && collidedBottom == true && jump == false) {
        jump = true;
    }
    
    if (entityType == PLAYER) {
        CheckCollisionsY(underlings, underlingsCount);

        CheckCollisionsY(midboss, midbossCount);
        CheckCollisionsY(boss, 1);
    }

    position.x += velocity.x * deltaTime; // Move on X
    CheckCollisionsX(floors, floorCount);// Fix if needed
    if (entityType == PLAYER) {
        CheckCollisionsX(underlings, underlingsCount);
        CheckCollisionsX(midboss, midbossCount);
        CheckCollisionsX(boss, 1);
    }

    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
}

void Entity::DrawSpriteFromTextureAtlas(ShaderProgram *program, GLuint textureID, int index)
{
    float u = (float)(index % animCols) / (float)animCols;
    float v = (float)(index / animCols) / (float)animRows;
    
    float width = 1.0f / (float)animCols;
    float height = 1.0f / (float)animRows;
    
    float texCoords[] = { u, v + height, u + width, v + height, u + width, v,
        u, v + height, u + width, v, u, v};
    
    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::Render(ShaderProgram *program) {

    if (isActive == false) return;

    program->SetModelMatrix(modelMatrix);
    
    if (animIndices != NULL) {
        DrawSpriteFromTextureAtlas(program, textureID, animIndices[animIndex]);
        return;
    }
    
    float vertices[]  = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}


