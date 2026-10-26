#include <cstddef>
///////////////////////////////////////////////////////////////////////////////
// SceneManager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Marrius Williams - SNHU Student / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, October 16th, 2026
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <vector>
#include <string>
#include <iostream>

// Structure to bundle Phong light source attributes
struct LightSourceData {
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float focalStrength;
	float specularIntensity;
};

// Global shader uniform variable names
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

// Externalized configuration constants for scene lighting
namespace LightConfig
{
	// Key Light (Primary neutral overhead illumination)
	const glm::vec3 KEY_POS(0.0f, 12.0f, 8.0f);
	const glm::vec3 KEY_AMBIENT(0.15f, 0.15f, 0.15f);
	const glm::vec3 KEY_DIFFUSE(0.65f, 0.65f, 0.65f);
	const glm::vec3 KEY_SPECULAR(0.3f, 0.3f, 0.3f);
	constexpr float KEY_FOCAL = 32.0f;
	constexpr float KEY_INTENSITY = 0.5f;

	// Accent Light (Secondary warm amber point light)
	const glm::vec3 ACCENT_POS(4.0f, 3.5f, 2.0f);
	const glm::vec3 ACCENT_AMBIENT(0.08f, 0.04f, 0.01f);
	const glm::vec3 ACCENT_DIFFUSE(0.85f, 0.55f, 0.2f);
	const glm::vec3 ACCENT_SPECULAR(0.4f, 0.25f, 0.1f);
	constexpr float ACCENT_FOCAL = 16.0f;
	constexpr float ACCENT_INTENSITY = 0.4f;
}

// Externalized geometric transformation constants
namespace SceneConfig
{
	// Desk Surface
	const glm::vec3 DESK_SCALE(20.0f, 1.0f, 10.0f);
	const glm::vec3 DESK_POS(0.0f, 0.0f, 0.0f);

	// Stand Foot
	const glm::vec3 STAND_FOOT_SCALE(2.0f, 0.08f, 1.6f);
	const glm::vec3 STAND_FOOT_POS(0.0f, 0.04f, 0.0f);

	// Stand Neck
	const glm::vec3 STAND_NECK_SCALE(1.2f, 2.2f, 0.1f);
	const glm::vec3 STAND_NECK_POS(0.0f, 1.0f, -0.3f);
	constexpr float STAND_NECK_TILT = -10.0f;

	// Monitor Body
	const glm::vec3 MONITOR_BODY_SCALE(7.0f, 4.2f, 0.25f);
	const glm::vec3 MONITOR_BODY_POS(0.0f, 3.6f, -0.4f);
	constexpr float MONITOR_TILT = -5.0f;

	// Display Screen
	const glm::vec3 SCREEN_SCALE(6.6f, 3.2f, 0.02f);
	const glm::vec3 SCREEN_POS(0.0f, 3.9f, -0.27f);

	// Cup
	const glm::vec3 CUP_SCALE(0.85f, 2.2f, 0.85f);
	const glm::vec3 CUP_POS(3.6f, 0.0f, 0.2f);

	// Pencil Shaft
	const glm::vec3 PENCIL_BODY_SCALE(0.12f, 2.2f, 0.12f);
	const glm::vec3 PENCIL_BODY_POS(-3.2f, 0.06f, 2.2f);
	constexpr float PENCIL_ROTATION_Z = -90.0f;

	// Pencil Tip
	const glm::vec3 PENCIL_TIP_SCALE(0.12f, 0.45f, 0.12f);
	const glm::vec3 PENCIL_TIP_POS(-2.1f, 0.06f, 2.2f);

	// Colors
	const glm::vec4 PENCIL_BODY_COLOR(0.95f, 0.78f, 0.15f, 1.0f);
	const glm::vec4 PENCIL_TIP_COLOR(0.15f, 0.15f, 0.15f, 1.0f);
}

/***********************************************************
 *  SceneManager()
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);

	if (image)
	{
		std::cout << "Successfully loaded image: " << filename << ", width: " << width << ", height: " << height << ", channels: " << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			stbi_image_free(image);
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image: " << filename << std::endl;
	return false;
}

/***********************************************************
 *  BindGLTextures()
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (!bFound))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return textureID;
}

/***********************************************************
 *  FindTextureSlot()
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (!bFound))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return textureSlot;
}

/***********************************************************
 *  FindMaterial()
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.empty())
	{
		return false;
	}

	for (size_t i = 0; i < m_objectMaterials.size(); ++i)
	{
		if (m_objectMaterials[i].tag.compare(tag) == 0)
		{
			material = m_objectMaterials[i];
			return true;
		}
	}

	return false;
}

/***********************************************************
 *  SetTransformations()
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 scale = glm::scale(scaleXYZ);
	glm::mat4 rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 translation = glm::translate(positionXYZ);

	glm::mat4 modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor(redColorValue, greenColorValue, blueColorValue, alphaValue);

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);
		int textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 ***********************************************************/
void SceneManager::SetShaderMaterial(std::string materialTag)
{
	if (!m_objectMaterials.empty())
	{
		OBJECT_MATERIAL material;
		if (FindMaterial(materialTag, material))
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
 *  LoadSceneTextures()
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bSuccess = false;

	bSuccess = CreateGLTexture("Textures/rusticwood.jpg", "rusticwood");
	if (!bSuccess)
	{
		std::cout << "Failed to load texture: Textures/rusticwood.jpg" << std::endl;
	}

	bSuccess = CreateGLTexture("Textures/stainless.jpg", "stainless");
	if (!bSuccess)
	{
		std::cout << "Failed to load texture: Textures/stainless.jpg" << std::endl;
	}

	bSuccess = CreateGLTexture("Textures/cheddar.jpg", "cheddar");
	if (!bSuccess)
	{
		std::cout << "Failed to load texture: Textures/cheddar.jpg" << std::endl;
	}

	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// Load all 4 required primitive meshes into GPU memory
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  DefineObjectMaterials()
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	m_objectMaterials.clear();

	// Helper lambda to construct and register materials without modifying the header struct
	auto AddMaterial = [this](std::string tag, glm::vec3 ambientColor, float ambientStrength,
		glm::vec3 diffuseColor, glm::vec3 specularColor, float shininess)
		{
			OBJECT_MATERIAL mat;
			mat.tag = tag;
			mat.ambientColor = ambientColor;
			mat.ambientStrength = ambientStrength;
			mat.diffuseColor = diffuseColor;
			mat.specularColor = specularColor;
			mat.shininess = shininess;
			m_objectMaterials.push_back(mat);
		};

	// 1. Desk Surface (Rustic Wood)
	AddMaterial("desk_wood", glm::vec3(0.5f, 0.5f, 0.5f), 0.3f, glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(0.5f, 0.5f, 0.5f), 32.0f);

	// 2. Stand & Monitor Body (Stainless Steel)
	AddMaterial("stain", glm::vec3(0.5f, 0.5f, 0.5f), 0.35f, glm::vec3(0.75f, 0.75f, 0.75f), glm::vec3(0.9f, 0.9f, 0.9f), 64.0f);

	// 3. Display Screen (Cheddar Graphic)
	AddMaterial("chedd", glm::vec3(0.5f, 0.5f, 0.5f), 0.5f, glm::vec3(0.85f, 0.85f, 0.85f), glm::vec3(0.2f, 0.2f, 0.2f), 16.0f);

	// 4. Cup (Ceramic Finish)
	AddMaterial("cup", glm::vec3(0.4f, 0.4f, 0.4f), 0.3f, glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(0.6f, 0.6f, 0.6f), 32.0f);

	// 5. Pencil Body (Yellow Lacquered Wood)
	AddMaterial("pencil_body", glm::vec3(0.5f, 0.4f, 0.5f), 0.35f, glm::vec3(0.9f, 0.75f, 0.1f), glm::vec3(0.3f, 0.3f, 0.3f), 16.0f);

	// 6. Pencil Tip (Graphite Core)
	AddMaterial("pencil_tip", glm::vec3(0.2f, 0.2f, 0.2f), 0.2f, glm::vec3(0.3f, 0.3f, 0.3f), glm::vec3(0.5f, 0.5f, 0.5f), 8.0f);
}

/***********************************************************
 *  SetupSceneLights()
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Multi-light definitions initialized with externalized constants
	std::vector<LightSourceData> sceneLights = {
		{
			LightConfig::KEY_POS,
			LightConfig::KEY_AMBIENT,
			LightConfig::KEY_DIFFUSE,
			LightConfig::KEY_SPECULAR,
			LightConfig::KEY_FOCAL,
			LightConfig::KEY_INTENSITY
		},
		{
			LightConfig::ACCENT_POS,
			LightConfig::ACCENT_AMBIENT,
			LightConfig::ACCENT_DIFFUSE,
			LightConfig::ACCENT_SPECULAR,
			LightConfig::ACCENT_FOCAL,
			LightConfig::ACCENT_INTENSITY
		}
	};

	// Procedural loop to bind uniforms dynamically without duplicated code
	for (size_t i = 0; i < sceneLights.size(); ++i)
	{
		std::string prefix = "lightSources[" + std::to_string(i) + "].";

		m_pShaderManager->setVec3Value(prefix + "position", sceneLights[i].position);
		m_pShaderManager->setVec3Value(prefix + "ambient", sceneLights[i].ambient);
		m_pShaderManager->setVec3Value(prefix + "diffuse", sceneLights[i].diffuse);
		m_pShaderManager->setVec3Value(prefix + "specular", sceneLights[i].specular);
		m_pShaderManager->setFloatValue(prefix + "focalStrength", sceneLights[i].focalStrength);
		m_pShaderManager->setFloatValue(prefix + "specularIntensity", sceneLights[i].specularIntensity);
	}
}

/***********************************************************
 *  RenderScene()
 ***********************************************************/
void SceneManager::RenderScene()
{
	// 1. Desk Surface (Plane)
	SetTransformations(SceneConfig::DESK_SCALE, 0.0f, 0.0f, 0.0f, SceneConfig::DESK_POS);
	SetShaderTexture("rusticwood");
	SetShaderMaterial("desk_wood");
	m_basicMeshes->DrawPlaneMesh();

	// 2. Stand Foot / Base (Box)
	SetTransformations(SceneConfig::STAND_FOOT_SCALE, 0.0f, 0.0f, 0.0f, SceneConfig::STAND_FOOT_POS);
	SetShaderTexture("stainless");
	SetShaderMaterial("stain");
	m_basicMeshes->DrawBoxMesh();

	// 3. Stand Neck (Box)
	SetTransformations(SceneConfig::STAND_NECK_SCALE, SceneConfig::STAND_NECK_TILT, 0.0f, 0.0f, SceneConfig::STAND_NECK_POS);
	SetShaderTexture("stainless");
	SetShaderMaterial("stain");
	m_basicMeshes->DrawBoxMesh();

	// 4. Monitor Body (Box)
	SetTransformations(SceneConfig::MONITOR_BODY_SCALE, SceneConfig::MONITOR_TILT, 0.0f, 0.0f, SceneConfig::MONITOR_BODY_POS);
	SetShaderTexture("stainless");
	SetShaderMaterial("stain");
	m_basicMeshes->DrawBoxMesh();

	// 5. Display Screen (Box)
	SetTransformations(SceneConfig::SCREEN_SCALE, SceneConfig::MONITOR_TILT, 0.0f, 0.0f, SceneConfig::SCREEN_POS);
	SetShaderTexture("cheddar");
	SetShaderMaterial("chedd");
	m_basicMeshes->DrawBoxMesh();

	// 6. Cup / Pen Holder (Cylinder)
	SetTransformations(SceneConfig::CUP_SCALE, 0.0f, 0.0f, 0.0f, SceneConfig::CUP_POS);
	SetShaderTexture("stainless");
	SetShaderMaterial("cup");
	m_basicMeshes->DrawCylinderMesh();

	// 7. Pencil Shaft (Cylinder - Composite Part 1)
	SetTransformations(SceneConfig::PENCIL_BODY_SCALE, 0.0f, 0.0f, SceneConfig::PENCIL_ROTATION_Z, SceneConfig::PENCIL_BODY_POS);
	SetShaderColor(SceneConfig::PENCIL_BODY_COLOR.r, SceneConfig::PENCIL_BODY_COLOR.g, SceneConfig::PENCIL_BODY_COLOR.b, SceneConfig::PENCIL_BODY_COLOR.a);
	SetShaderMaterial("pencil_body");
	m_basicMeshes->DrawCylinderMesh();

	// 8. Pencil Tip (Cone - Composite Part 2)
	SetTransformations(SceneConfig::PENCIL_TIP_SCALE, 0.0f, 0.0f, SceneConfig::PENCIL_ROTATION_Z, SceneConfig::PENCIL_TIP_POS);
	SetShaderColor(SceneConfig::PENCIL_TIP_COLOR.r, SceneConfig::PENCIL_TIP_COLOR.g, SceneConfig::PENCIL_TIP_COLOR.b, SceneConfig::PENCIL_TIP_COLOR.a);
	SetShaderMaterial("pencil_tip");
	m_basicMeshes->DrawConeMesh();
}