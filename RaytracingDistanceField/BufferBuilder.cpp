#include "BufferBuilder.hpp"

GLuint zero[1024 * 1024] = { 0 };
void R32UIntImageBuffer2D::Clear() {
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sx, sy,
        GL_RED_INTEGER, GL_UNSIGNED_INT, zero);
    glBindTexture(GL_TEXTURE_2D, 0);
}