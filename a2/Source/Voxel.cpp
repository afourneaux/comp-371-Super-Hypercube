#include "Voxel.h"

Voxel::Voxel(vec3 position, int vao, int shaderProgram, vec3 localScale) : mPosition(position), mvao(vao), mScaleVector(localScale)
{
	mWorldMatrixLocation = glGetUniformLocation(shaderProgram, "worldMatrix");
}

void Voxel::Draw(GLenum renderingMode) {
		glBindVertexArray(mvao);
		mat4 worldMatrix = mAnchor * translate(mat4(1.0f), mPosition) * rotate(mat4(1.0f), radians(mOrientation.x), vec3(1.0f, 0.0f, 0.0f)) * rotate(mat4(1.0f), radians(mOrientation.y), vec3(0.0f, 1.0f, 0.0f)) * rotate(mat4(1.0f), radians(mOrientation.z), vec3(0.0f, 0.0f, 1.0f)) * scale(mat4(1.0f), mScale * mScaleVector);
		glUniformMatrix4fv(mWorldMatrixLocation, 1, GL_FALSE, &worldMatrix[0][0]);
		glDrawArrays(renderingMode, 0, 36);
}