/*
Copyright (C) 2017-2018 ezQuake team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "quakedef.h"
#include "gl_model.h"
#include "gl_local.h"
#include "common_draw.h"
#include "glm_draw.h"
#include "tr_types.h"
#include "glsl/constants.glsl"
#include "r_matrix.h"
#include "glm_vao.h"
#include "r_state.h"
#include "glc_vao.h"
#include "r_buffers.h"

extern cvar_t r_smoothtext, r_smoothcrosshair, r_smoothimages;
float cachedMatrix[16];

void GLM_AdjustImages(int first, int last, float x_offset);
void GLC_AdjustImages(int first, int last, float x_offset);
void VK_AdjustImages(int first, int last, float x_offset);

void GLM_DrawImage(float x, float y, float width, float height, float tex_s, float tex_t, float tex_width, float tex_height, byte* color, int flags);
void GLC_DrawImage(float x, float y, float width, float height, float tex_s, float tex_t, float tex_width, float tex_height, byte* color, int flags);
void VK_DrawImage(float x, float y, float width, float height, float tex_s, float tex_t, float tex_width, float tex_height, byte* color, int flags);
void GLM_DrawRectangle(float x, float y, float width, float height, byte* color);
void GLC_DrawRectangle(float x, float y, float width, float height, byte* color);
void VK_DrawRectangle(float x, float y, float width, float height, byte* color);

glm_image_framedata_t imageData;

int Draw_ImagePosition(void)
{
	return imageData.imageCount;
}

void Draw_AdjustImages(int first, int last, float x_offset)
{
	float v1[4] = { x_offset, 0, 0, 1 };
	float v2[4] = { 0, 0, 0, 1 };

	GLM_MultiplyVector(cachedMatrix, v1, v1);
	GLM_MultiplyVector(cachedMatrix, v2, v2);

	x_offset = v1[0] - v2[0];

	if (R_UseModernOpenGL()) {
		GLM_AdjustImages(first, last, x_offset);
	}
	else if (R_UseImmediateOpenGL()) {
		GLC_AdjustImages(first, last, x_offset);
	}
	else if (R_UseVulkan()) {
		VK_AdjustImages(first, last, x_offset);
	}
}

void R_DrawImage(float x, float y, float width, float height, float tex_s, float tex_t, float tex_width, float tex_height, byte* color, qbool alpha_test, texture_ref texnum, qbool isText, qbool isCrosshair)
{
	int flags = IMAGEPROG_FLAGS_TEXTURE;

	if (imageData.imageCount >= MAX_MULTI_IMAGE_BATCH) {
		return;
	}
	if (!GLM_LogCustomImageTypeWithTexture(imagetype_image, imageData.imageCount, texnum)) {
		return;
	}

	flags |= (alpha_test ? IMAGEPROG_FLAGS_ALPHATEST : 0);
	if (isCrosshair) {
		if (!r_smoothcrosshair.integer) {
			flags |= IMAGEPROG_FLAGS_NEAREST;
		}
	}
	else if (isText) {
		flags |= IMAGEPROG_FLAGS_TEXT;
		if (!r_smoothtext.integer) {
			flags |= IMAGEPROG_FLAGS_NEAREST;
		}
	}
	else {
		if (!r_smoothimages.integer) {
			flags |= IMAGEPROG_FLAGS_NEAREST;
		}
	}

	if (R_UseModernOpenGL()) {
		GLM_DrawImage(x, y, width, height, tex_s, tex_t, tex_width, tex_height, color, flags);
	}
	else if (R_UseImmediateOpenGL()) {
		GLC_DrawImage(x, y, width, height, tex_s, tex_t, tex_width, tex_height, color, flags);
	}
	else if (R_UseVulkan()) {
		VK_DrawImage(x, y, width, height, tex_s, tex_t, tex_width, tex_height, color, flags);
	}

	++imageData.imageCount;
}

void R_DrawRectangle(float x, float y, float width, float height, byte* color)
{
	if (R_UseImmediateOpenGL()) {
		GLC_DrawRectangle(x, y, width, height, color);
	}
	else if (R_UseModernOpenGL()) {
		GLM_DrawRectangle(x, y, width, height, color);
	}
	else if (R_UseVulkan()) {
		VK_DrawRectangle(x, y, width, height, color);
	}
}

void R_Cache2DMatrix(void)
{
	GLM_MultiplyMatrix(GLM_ProjectionMatrix(), GLM_ModelviewMatrix(), cachedMatrix);
}

void R_UndoLastCharacter(void)
{
	if (imageData.imageCount) {
		--imageData.imageCount;
	}
}
