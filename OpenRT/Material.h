#pragma once
#include <gl/GL.h>

class Material {
public:
    float r = 1, g = 1, b = 1; // base color
    bool lit = true;           // enable lighting?
    bool wireframe = false;    // draw mode?

    Material() {}

    void Apply() const {
        if (lit) {
            glEnable(GL_LIGHTING);
            glEnable(GL_COLOR_MATERIAL);
        }
        else {
            glDisable(GL_LIGHTING);
        }

        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glColor3f(r, g, b);
    }
};
