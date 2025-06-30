#pragma once

#include <gl/GL.h>
#include <cmath>

#include "EngineAPI.h"
#include "Material.h"

class Mesh {
public:
    Vec3 position;
    Vec3 rotation; // Euler angles in degrees
    Vec3 scale;

    Material material;

    Mesh()
    {
        position = Vec3{ 0, 0, 0 };
        rotation = Vec3{ 0, 0, 0 };
        scale = Vec3{ 1, 1, 1 };
    }

    void Update(float deltaTime)
    {
    }

    void Draw()
    {
        glPushMatrix();

        glTranslatef(position.x, position.y, position.z);
        glRotatef(rotation.x, 1.0f, 0.0f, 0.0f);
        glRotatef(rotation.y, 0.0f, 1.0f, 0.0f);
        glRotatef(rotation.z, 0.0f, 0.0f, 1.0f);
        glScalef(scale.x, scale.y, scale.z);

        material.Apply();

        glBegin(GL_QUADS);

        glVertex3f(-1, -1, 1);
        glVertex3f(1, -1, 1);
        glVertex3f(1, 1, 1);
        glVertex3f(-1, 1, 1);

        glVertex3f(-1, -1, -1);
        glVertex3f(-1, 1, -1);
        glVertex3f(1, 1, -1);
        glVertex3f(1, -1, -1);

        glVertex3f(-1, 1, -1);
        glVertex3f(-1, 1, 1);
        glVertex3f(1, 1, 1);
        glVertex3f(1, 1, -1);

        glVertex3f(-1, -1, -1);
        glVertex3f(1, -1, -1);
        glVertex3f(1, -1, 1);
        glVertex3f(-1, -1, 1);

        glVertex3f(1, -1, -1);
        glVertex3f(1, 1, -1);
        glVertex3f(1, 1, 1);
        glVertex3f(1, -1, 1);

        glVertex3f(-1, -1, -1);
        glVertex3f(-1, -1, 1);
        glVertex3f(-1, 1, 1);
        glVertex3f(-1, 1, -1);

        glEnd();
        glPopMatrix();
    }
};