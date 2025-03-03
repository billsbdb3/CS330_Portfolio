///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////
#include <tuple>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include "SceneManager.h"

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag, GLint wrapS = GL_REPEAT, GLint wrapT = GL_REPEAT)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		GLenum format;
		if (colorChannels == 1)
			format = GL_RED;
		else if (colorChannels == 3)
			format = GL_RGB;
		else if (colorChannels == 4)
			format = GL_RGBA;
		else
		{
			std::cout << "Error: Unsupported number of channels: " << colorChannels << std::endl;
			stbi_image_free(image);
			glBindTexture(GL_TEXTURE_2D, 0);
			return false;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);

		// generate the texture mipmaps for mapping textures to lower resolutions
		// **IMPORTANT: Generate mipmaps BEFORE setting wrap parameters.**
		glGenerateMipmap(GL_TEXTURE_2D);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Use mipmapping for minification
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
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
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		// Find the texture slot (0-15)
		int textureSlot = FindTextureSlot(textureTag);
		if (textureSlot != -1)
		{
			// Activate the correct texture unit BEFORE binding.
			glActiveTexture(GL_TEXTURE0 + textureSlot);

			// Bind the texture to the active texture unit.
			glBindTexture(GL_TEXTURE_2D, m_textureIDs[textureSlot].ID);

			// Tell the shader which texture unit to use.  This is CRUCIAL.
			m_pShaderManager->setIntValue(g_TextureValueName, textureSlot); // Pass the SLOT, not the ID
		}
		else
		{
			std::cerr << "Texture slot not found for tag: " << textureTag << std::endl;
		}
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
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
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures() {
	bool bReturn = false;

	// Load glass texture (repeating)
	bReturn = CreateGLTexture("textures/glass.jpg", "glass"); // Uses default GL_REPEAT
	if (!bReturn) std::cerr << "Failed to load glass texture." << std::endl;

	// Load green_stem texture (repeating)
	bReturn = CreateGLTexture("textures/green_stem.jpg", "green_stem"); // Uses default GL_REPEAT
	if (!bReturn) std::cerr << "Failed to load green_stem texture." << std::endl;

	// Load white_flower texture (repeating)
	bReturn = CreateGLTexture("textures/white_flower.png", "white_flower"); // Uses default GL_REPEAT
	if (!bReturn) std::cerr << "Failed to load white_flower texture." << std::endl;

	// Load beige_puff texture (repeating)
	bReturn = CreateGLTexture("textures/beige_puff.jpg", "beige_puff"); // Uses default GL_REPEAT
	if (!bReturn) std::cerr << "Failed to load beige_puff texture." << std::endl;

	// Load wood texture (repeating)
	bReturn = CreateGLTexture("textures/wood.jpg", "wood"); // Uses default GL_REPEAT
	if (!bReturn) std::cerr << "Failed to load wood texture." << std::endl;

	// Load desk texture (repeating) - STILL NEEDS 3 or 4 channels
	bReturn = CreateGLTexture("textures/desk.jpg", "desk");  //Uses default GL_REPEAT
	if (!bReturn) std::cerr << "Failed to load desk texture." << std::endl;

	// Load wet_glass texture (repeating)
	bReturn = CreateGLTexture("textures/wet_glass.jpg", "vase_opening_side"); // Uses default GL_REPEAT
	if (!bReturn) std::cerr << "Failed to load vase_opening_side texture." << std::endl;



	// Load keyboard texture (CLAMP_TO_EDGE)
	bReturn = CreateGLTexture("textures/keyboard_texture.jpg", "keyboard_texture", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE); // Specify clamping
	if (!bReturn) std::cerr << "Failed to load keyboard texture." << std::endl;

	// Load mouse texture (CLAMP_TO_EDGE)
	bReturn = CreateGLTexture("textures/mouse_texture.jpg", "mouse_texture", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);   // Specify clamping
	if (!bReturn) std::cerr << "Failed to load mouse texture." << std::endl;

	// Bind textures (this part remains unchanged)
	BindGLTextures();
}
/***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// Plastic material (Monitor Bezel, etc.)
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f); // Dark gray
	plasticMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Low specular
	plasticMaterial.shininess = 32.0f;  // Slightly shiny
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	// Silver Material (Monitor Stand)
	OBJECT_MATERIAL silverMaterial;
	silverMaterial.diffuseColor = glm::vec3(0.75f, 0.75f, 0.75f); // Light gray
	silverMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // Strong specular
	silverMaterial.shininess = 128.0f; // Very shiny
	silverMaterial.tag = "silver";
	m_objectMaterials.push_back(silverMaterial);

	// Glass material (Vase)
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.2f); // Slightly bluish tint
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // Very strong specular
	glassMaterial.shininess = 256.0f; // Extremely shiny
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	// Brown stem material
	OBJECT_MATERIAL brownStemMaterial;
	brownStemMaterial.diffuseColor = glm::vec3(0.45f, 0.35f, 0.25f);
	brownStemMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	brownStemMaterial.shininess = 16.0f; // a bit shiny
	brownStemMaterial.tag = "brown_stem";
	m_objectMaterials.push_back(brownStemMaterial);

	// Green stem material
	OBJECT_MATERIAL greenStemMaterial;
	greenStemMaterial.diffuseColor = glm::vec3(0.15f, 0.4f, 0.2f);
	greenStemMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	greenStemMaterial.shininess = 16.0f;
	greenStemMaterial.tag = "green_stem";
	m_objectMaterials.push_back(greenStemMaterial);

	// Beige puff material
	OBJECT_MATERIAL beigePuffMaterial;
	beigePuffMaterial.diffuseColor = glm::vec3(0.93f, 0.86f, 0.76f);
	beigePuffMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	beigePuffMaterial.shininess = 4.0f; // not very shiny
	beigePuffMaterial.tag = "beige_puff";
	m_objectMaterials.push_back(beigePuffMaterial);

	// White flower material
	OBJECT_MATERIAL whiteFlowerMaterial;
	whiteFlowerMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	whiteFlowerMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	whiteFlowerMaterial.shininess = 8.0f;
	whiteFlowerMaterial.tag = "white_flower";
	m_objectMaterials.push_back(whiteFlowerMaterial);

	// Wood material (Desk)
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.2f);    // Brownish
	woodMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);   // Moderate specular
	woodMaterial.shininess = 32.0f;  // Somewhat shiny
	woodMaterial.tag = "desk";
	m_objectMaterials.push_back(woodMaterial);

	// Organizer Material (Example: Light Gray Plastic)
	OBJECT_MATERIAL organizerMaterial;
	organizerMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);  // Light gray
	organizerMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Moderate specular
	organizerMaterial.shininess = 32.0f;
	organizerMaterial.tag = "organizer";
	m_objectMaterials.push_back(organizerMaterial);

	// Teacup Material (Example: Ceramic)
	OBJECT_MATERIAL teacupMaterial;
	teacupMaterial.diffuseColor = glm::vec3(0.95f, 0.9f, 0.85f); // Off-white
	teacupMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f); // Moderate specular
	teacupMaterial.shininess = 64.0f; // More shiny than plastic
	teacupMaterial.tag = "teacup";
	m_objectMaterials.push_back(teacupMaterial);

	// Saucer Material (Example: Ceramic - same as teacup)
	OBJECT_MATERIAL saucerMaterial;
	saucerMaterial.diffuseColor = glm::vec3(0.95f, 0.9f, 0.85f); // Off-white
	saucerMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f); // Moderate specular
	saucerMaterial.shininess = 64.0f; // More shiny than plastic
	saucerMaterial.tag = "saucer"; // Use the same material as the teacup
	m_objectMaterials.push_back(saucerMaterial);

	// Gray Book Material
	OBJECT_MATERIAL grayBookMaterial;
	grayBookMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Gray
	grayBookMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); // Low specular
	grayBookMaterial.shininess = 8.0f;   // Not very shiny
	grayBookMaterial.tag = "gray_book";
	m_objectMaterials.push_back(grayBookMaterial);

	// Black Book Material
	OBJECT_MATERIAL blackBookMaterial;
	blackBookMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);  // Black
	blackBookMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); // Low specular
	blackBookMaterial.shininess = 8.0f;   // Not very shiny
	blackBookMaterial.tag = "black_book";
	m_objectMaterials.push_back(blackBookMaterial);

	// Light Blue Book Material
	OBJECT_MATERIAL lightBlueBookMaterial;
	lightBlueBookMaterial.diffuseColor = glm::vec3(0.4f, 0.6f, 0.8f); // Light Blue
	lightBlueBookMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Low specular
	lightBlueBookMaterial.shininess = 8.0f;  // Not very shiny
	lightBlueBookMaterial.tag = "light_blue_book";
	m_objectMaterials.push_back(lightBlueBookMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// --- Directional Light (Main Light Source) ---
	// * Softer, coming from the front-left, slightly above.
	// * Notice the shadows in the reference image.
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.5f, -0.6f, 0.7f); // Front-left, down
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.4f));   // Reduced ambient
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.7f));   // Moderate diffuse
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(0.6f));  // Moderate specular
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// --- Point Light 1 (Overhead, Slightly Behind) ---
	// * Acts as a general fill light, softening shadows.
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(0.0f, 12.0f, 5.0f)); // Higher, slightly behind
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.2f));    // Low ambient
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.5f));    // Moderate diffuse
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.3f));   // Low specular
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.045f);     // Slightly increased
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.0075f);  // Slightly increased
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// --- Point Light 2 (Front-Right, Close to Objects) ---
	// * Adds a highlight to the right side of objects, creating more contrast.
	m_pShaderManager->setVec3Value("pointLights[1].position", glm::vec3(10.0f, 6.0f, -3.0f)); // Front-right, closer
	m_pShaderManager->setVec3Value("pointLights[1].ambient", glm::vec3(0.1f));     // Very low ambient
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", glm::vec3(0.6f));     // Moderate diffuse
	m_pShaderManager->setVec3Value("pointLights[1].specular", glm::vec3(0.8f));    // Stronger specular
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// --- Point Light 3 ---
	m_pShaderManager->setVec3Value("pointLights[2].position", glm::vec3(-7.0f, 8.0f, 10.0f));
	m_pShaderManager->setVec3Value("pointLights[2].ambient", glm::vec3(0.1f));
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", glm::vec3(0.3f));
	m_pShaderManager->setVec3Value("pointLights[2].specular", glm::vec3(0.2f));
	m_pShaderManager->setFloatValue("pointLights[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", false);

	// --- Point Light 4 ---
	m_pShaderManager->setVec3Value("pointLights[3].position", glm::vec3(2.0f, 4.0f, -5.0f));
	m_pShaderManager->setVec3Value("pointLights[3].ambient", glm::vec3(0.05f));
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", glm::vec3(0.2f));
	m_pShaderManager->setVec3Value("pointLights[3].specular", glm::vec3(0.1f));
	m_pShaderManager->setFloatValue("pointLights[3].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[3].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[3].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", false);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// define the materials for objects in the scene
	DefineObjectMaterials();

	// add and define the light sources for the scene
	SetupSceneLights();

	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();

	// Load additional meshes for the scene
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();
	
	
}

// --- Helper Functions---

// --- Book Helper Functions ---
void SceneManager::DrawBook(const glm::vec3& position, const glm::vec3& scale, const glm::vec4& color) {
	SetShaderColor(color.r, color.g, color.b, color.a);
	SetTransformations(scale, 0.0f, 0.0f, 0.0f, position);
	m_basicMeshes->DrawBoxMesh();
}

// Gray book (bottom)
void SceneManager::DrawGrayBook(const glm::vec3& basePosition, float deskHeight) {
	SetShaderMaterial("gray_book");
	glm::vec3 bookScale = glm::vec3(10.5f, 2.5f, 4.5f); // Width, height, depth
	glm::vec3 bookPosition = glm::vec3(basePosition.x, deskHeight + bookScale.y / 2.0f, basePosition.z + 0.2f);
	glm::vec4 bookColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
	DrawBook(bookPosition, bookScale, bookColor);
}

// Black book (middle)
void SceneManager::DrawBlackBook(const glm::vec3& basePosition, float deskHeight) {
	SetShaderMaterial("black_book");
	glm::vec3 grayBookScale = glm::vec3(10.5f, 2.5f, 4.5f); // Gray book scale for positioning
	glm::vec3 blackBookScale = glm::vec3(9.5f, 1.25f, 4.0f); // Slightly smaller
	glm::vec3 grayBookPosition = glm::vec3(basePosition.x, deskHeight + grayBookScale.y / 2.0f, basePosition.z + 0.2f);
	glm::vec3 blackBookPosition = grayBookPosition + glm::vec3(0.2f, grayBookScale.y / 2.0f + blackBookScale.y / 2.0f, -0.2f); // On top of gray book
	glm::vec4 bookColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f); // Black (or very dark gray)
	DrawBook(blackBookPosition, blackBookScale, bookColor);
}

// Light blue book (top)
void SceneManager::DrawLightBlueBook(const glm::vec3& basePosition, float deskHeight) {
	SetShaderMaterial("light_blue_book");
	glm::vec3 grayBookScale = glm::vec3(10.5f, 2.5f, 4.5f); // Gray book scale
	glm::vec3 blackBookScale = glm::vec3(9.5f, 1.25f, 4.0f); // Black book scale
	glm::vec3 lightBlueBookScale = glm::vec3(9.0f, 1.25f, 3.5f); // Slightly smaller again
	glm::vec3 grayBookPosition = glm::vec3(basePosition.x, deskHeight + grayBookScale.y / 2.0f, basePosition.z + 0.2f);
	glm::vec3 blackBookPosition = grayBookPosition + glm::vec3(0.2f, grayBookScale.y / 2.0f + blackBookScale.y / 2.0f, -0.2f); // On top of gray book
	glm::vec3 lightBlueBookPosition = blackBookPosition + glm::vec3(-0.2f, blackBookScale.y / 2.0f + lightBlueBookScale.y / 2.0f, 0.2f); // On top of black, offset
	glm::vec4 bookColor = glm::vec4(0.4f, 0.6f, 0.8f, 1.0f); // Light blue
	DrawBook(lightBlueBookPosition, lightBlueBookScale, bookColor);
}



// --- Monitor Helper Functions ---

void SceneManager::DrawMonitorBezel(const glm::vec3& position, const glm::vec3& scale, const glm::vec4& color)
{
	SetShaderColor(color.r, color.g, color.b, color.a);
	SetTransformations(scale, 0.0f, 0.0f, 0.0f, position);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawMonitorScreen(const glm::vec3& position, const glm::vec3& scale)
{
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White screen
	SetTransformations(scale, 0.0f, 0.0f, 0.0f, position);
	m_basicMeshes->DrawBoxMesh();
}


void SceneManager::DrawMonitorStand(const glm::vec3& basePosition)
{
	// Stand Base
	glm::vec3 scaleXYZ = glm::vec3(7.0f, 0.3f, 5.0f);
	glm::vec3 positionXYZ = glm::vec3(basePosition.x, 0.15f, basePosition.z - 2.0f); //Relative to the base position.
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.82f, 0.82f, 0.82f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Stand Arm (22.5° forward tilt)
	const float armLength = 8.2f;
	scaleXYZ = glm::vec3(0.8f, armLength, 0.8f);
	positionXYZ = glm::vec3(basePosition.x, 0.15f, basePosition.z - 3.6f); //Relative to the base position
	SetTransformations(scaleXYZ, 22.5f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Connection Point (Hidden)
	scaleXYZ = glm::vec3(1.8f, 0.5f, 0.8f);
	positionXYZ = glm::vec3(basePosition.x, basePosition.y + 3.45f, basePosition.z - 0.5f);  //Relative to the base position
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
}


void SceneManager::DrawMonitor(float deskHeight)
{
	const float monitorWidth = 18.0f;
	const float totalHeight = 12.0f;
	const float silverBezelHeight = 2.5f;
	const float blackBezelThickness = 0.6f;
	const float screenDepth = 0.05f;
	const float monitorBaseY = deskHeight + 4.0f; // Base Y-position (raised above desk)
	const float blackBezelHeight = 0.5f; // Black bezel between silver and screen

	glm::vec3 basePosition(0.0f, monitorBaseY, -1.0f); // added a base position for the whole monitor

	// --- Draw the monitor components ---

	// Black Bezel - Top
	DrawMonitorBezel(
		glm::vec3(basePosition.x, basePosition.y + totalHeight - blackBezelThickness / 2, basePosition.z),
		glm::vec3(monitorWidth, blackBezelThickness, 0.2f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	// Black Bezel - Left
	DrawMonitorBezel(
		glm::vec3(basePosition.x - monitorWidth / 2 + blackBezelThickness / 2, basePosition.y + totalHeight / 2, basePosition.z),
		glm::vec3(blackBezelThickness, totalHeight, 0.2f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	// Black Bezel - Right
	DrawMonitorBezel(
		glm::vec3(basePosition.x + monitorWidth / 2 - blackBezelThickness / 2, basePosition.y + totalHeight / 2, basePosition.z),
		glm::vec3(blackBezelThickness, totalHeight, 0.2f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	// Silver Bezel (Chin)
	DrawMonitorBezel(
		glm::vec3(basePosition.x, basePosition.y + silverBezelHeight / 2, basePosition.z + 0.05f),
		glm::vec3(monitorWidth, silverBezelHeight, 0.3f),
		glm::vec4(0.09f, 0.09f, 0.09f, 1.0f) 
	);

	// Black Bezel (Between silver and screen)
	DrawMonitorBezel(
		glm::vec3(basePosition.x, basePosition.y + silverBezelHeight + blackBezelHeight / 2, basePosition.z),
		glm::vec3(monitorWidth, blackBezelHeight, 0.2f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	// White Screen
	float screenHeight = totalHeight - silverBezelHeight - blackBezelHeight - 0.6f;
	DrawMonitorScreen(
		glm::vec3(basePosition.x, basePosition.y + silverBezelHeight + blackBezelHeight + screenHeight / 2, basePosition.z + 0.05f),
		glm::vec3(monitorWidth - 1.0f, screenHeight, screenDepth)
	);

	// Draw Stand Components
	DrawMonitorStand(basePosition);
}


// --- Vase Helper Functions ---
void SceneManager::DrawVaseBase(const glm::vec3& basePosition) {
	SetShaderTexture("glass");
	SetShaderMaterial("glass");
	SetTransformations(glm::vec3(2.0f, 1.2f, 2.0f), 0.0f, 0.0f, 0.0f, basePosition);
	m_basicMeshes->DrawSphereMesh();
}

void SceneManager::DrawVaseNeck(const glm::vec3& basePosition) {
	SetShaderTexture("glass");
	SetShaderMaterial("glass");
	// Corrected: Base + base half-height + neck half-height.
	glm::vec3 neckPosition = basePosition + glm::vec3(0.0f, 0.8f, 0.0f);
	SetTransformations(glm::vec3(1.5f, 2.2f, 1.5f), 0.0f, 0.0f, 0.0f, neckPosition);
	m_basicMeshes->DrawTaperedCylinderMesh();
}

void SceneManager::DrawVaseOpening(const glm::vec3& basePosition) {
	SetShaderTexture("vase_opening_side"); // Sides texture
	glm::vec3 neckPosition = basePosition + glm::vec3(0.0f, 0.8f , 0.0f); // Use calculated neck position
	glm::vec3 openingPosition = neckPosition + glm::vec3(0.0f, 1.1f + 1.0f, 0.0f); 
	SetTransformations(glm::vec3(0.75f, 2.0f, 0.75f), 0.0f, 0.0f, 0.0f, openingPosition);
	m_basicMeshes->DrawCylinderMesh(false, false); // Sides
	SetShaderTexture("glass"); // Texture for top and bottom
	m_basicMeshes->DrawCylinderMesh(true, false, false); // Top
	m_basicMeshes->DrawCylinderMesh(false, true, false); // Bottom
}

void SceneManager::DrawVaseRim(const glm::vec3& basePosition) {
	SetShaderTexture("glass");
	SetShaderMaterial("glass");
	glm::vec3 neckPosition = basePosition + glm::vec3(0.0f, 1.8f, 0.0f); // Use calculated neck position
	glm::vec3 rimPosition = neckPosition + glm::vec3(0.0f, 1.1f + 2.0f, 0.0f); 
	SetTransformations(glm::vec3(0.9f, 0.9f, 0.5f), 90.0f, 0.0f, 0.0f, rimPosition);
	m_basicMeshes->DrawTorusMesh();
}

void SceneManager::DrawBrownStems(const glm::vec3& basePosition) {
	SetShaderTexture("wood");
	SetShaderMaterial("brown_stem");

	const std::vector<glm::vec3> stemOffsets = {
		glm::vec3(-0.6f, 0.1f, 0.2f),   // Left
		glm::vec3(-0.5f, 0.2f, -0.25f),  // Right
		glm::vec3(-0.1f, 0.0f, 0.6f),   // Front
		glm::vec3(-0.2f, 0.1f, -0.55f)  // Back
	};

	glm::vec3 neckPosition = basePosition + glm::vec3(0.0f, 1.2f + 1.1f, 0.0f); // Use calculated neck position
	glm::vec3 rimPosition = neckPosition + glm::vec3(0.0f, 1.1f + 2.0f, 0.0f);

	for (const auto& offset : stemOffsets) {
		SetTransformations(glm::vec3(0.1f, 0.1f, 1.5f), -90.0f, 25.0f, 10.0f, rimPosition + offset);
		m_basicMeshes->DrawTaperedCylinderMesh();
	}
}

void SceneManager::DrawBeigePuffs(const glm::vec3& basePosition) {
	SetShaderTexture("beige_puff");
	SetShaderMaterial("beige_puff");

	const float rimRadius = 1.0f;
	const int puffCount = 24;
	glm::vec3 neckPosition = basePosition + glm::vec3(0.0f, 1.2f + 1.1f, 0.0f); // Use calculated neck position
	glm::vec3 rimPosition = neckPosition + glm::vec3(0.0f, 1.1f + 2.0f, 0.0f);

	for (int i = 0; i < puffCount; ++i) {
		float angle = glm::radians(360.0f * i / puffCount);
		float spread = 0.8f + 0.4f * (i % 3);

		glm::vec3 puffPosition = glm::vec3(
			rimPosition.x + rimRadius * spread * cos(angle),
			rimPosition.y + 0.3f + 0.5f * (i % 4),
			rimPosition.z + rimRadius * spread * sin(angle)
		);

		SetTransformations(glm::vec3(0.25f, 0.25f, 0.8f), 0.0f, glm::degrees(angle) + 90.0f, 0.0f, puffPosition);
		m_basicMeshes->DrawSphereMesh();
	}
}

void SceneManager::DrawGreenBranches(const glm::vec3& basePosition) {
	SetShaderTexture("green_stem");
	SetShaderMaterial("green_stem");

	const std::vector<std::tuple<glm::vec3, float, float>> branches = {
	  std::make_tuple(glm::vec3(0.4f, 0.6f,  0.3f),  30.0f, -15.0f),
	  std::make_tuple(glm::vec3(-0.5f, 0.8f, -0.2f), -25.0f,  20.0f),
	  std::make_tuple(glm::vec3(0.2f, 1.0f, -0.4f),  10.0f,   5.0f)
	};
	glm::vec3 neckPosition = basePosition + glm::vec3(0.0f, 1.2f + 1.1f, 0.0f); // Use calculated neck position
	glm::vec3 rimPosition = neckPosition + glm::vec3(0.0f, 1.1f + 2.0f, 0.0f);

	// Use constexpr indices
	constexpr int POS_INDEX = 0;
	constexpr int Y_ROT_INDEX = 1;
	constexpr int Z_ROT_INDEX = 2;

	for (const auto& branch : branches) {
		// Access tuple elements using std::get<INDEX>(tuple)
		glm::vec3 pos = std::get<POS_INDEX>(branch);
		float yRot = std::get<Y_ROT_INDEX>(branch);
		float zRot = std::get<Z_ROT_INDEX>(branch);

		// Main branch
		SetTransformations(glm::vec3(0.06f, 0.06f, 2.5f), -90.0f, yRot, zRot, rimPosition + pos);
		m_basicMeshes->DrawTaperedCylinderMesh();

		// Sub-branches
		for (int i = 0; i < 3; ++i) {
			glm::vec3 subPos = rimPosition + pos + glm::vec3((i + 1) * 0.2f, 1.0f + (i * 0.8f), (i + 1) * 0.2f);
			SetTransformations(glm::vec3(0.04f, 0.04f, 1.5f), -90.0f, yRot + 25.0f, zRot + 20.0f, subPos);
			m_basicMeshes->DrawCylinderMesh();

			// Flowers on sub-branches
			SetShaderTexture("white_flower");
			SetShaderMaterial("white_flower");
			for (int j = 0; j < 2; ++j) {
				glm::vec3 flowerPos = subPos + glm::vec3(0.1f * j, 0.5f + 0.4f * j, 0.1f * j);
				SetTransformations(glm::vec3(0.1f), 0.0f, 0.0f, 0.0f, flowerPos);
				m_basicMeshes->DrawSphereMesh();
			}
			SetShaderTexture("green_stem"); //reset texture
			SetShaderMaterial("green_stem"); //reset material
		}
	}
}

void SceneManager::DrawWhiteFlowers(const glm::vec3& basePosition) {
	SetShaderTexture("white_flower");
	SetShaderMaterial("white_flower");

	const std::vector<glm::vec3> flowerOffsets = {
		glm::vec3(0.1f, 0.2f, 0.1f),  glm::vec3(-0.15f, 0.3f, -0.1f),
		glm::vec3(0.0f, 0.4f, 0.2f),  glm::vec3(-0.2f, 0.25f, 0.15f),
		glm::vec3(0.15f, 0.35f, -0.2f), glm::vec3(-0.1f, 0.4f, -0.15f)
	};
	// Clusters
	const std::vector<glm::vec3> flowerClusters = {
	  glm::vec3(0.5f, 1.3f,  0.4f),
	  glm::vec3(-0.6f, 1.6f, -0.3f),
	  glm::vec3(0.3f, 2.2f, -0.5f),
	  glm::vec3(-0.4f, 1.9f,  0.2f),
	  glm::vec3(0.2f, 2.1f,  0.3f)
	};

	glm::vec3 neckPosition = basePosition + glm::vec3(0.0f, 1.2f + 1.1f, 0.0f); // Use calculated neck position
	glm::vec3 rimPosition = neckPosition + glm::vec3(0.0f, 1.1f + 2.0f, 0.0f);

	for (const auto& clusterBase : flowerClusters) {
		for (int i = 0; i < 3; ++i) {
			const auto& offset = flowerOffsets[i % flowerOffsets.size()];
			glm::vec3 flowerPos = rimPosition + clusterBase + offset * glm::vec3(i + 1, 0.8f, (i % 2) ? -1.0f : 1.0f);
			glm::vec3 flowerScale = glm::vec3(0.1f) * (0.9f + 0.2f * (i % 3));
			SetTransformations(flowerScale, 0.0f, 30.0f * (i % 4), 15.0f * (i % 2), flowerPos);
			m_basicMeshes->DrawSphereMesh();
		}
	}

	// Scattered flowers
	const std::vector<glm::vec3> scatteredFlowers = {
	  glm::vec3(0.4f, 1.7f,  0.5f), glm::vec3(-0.3f, 2.0f, -0.4f),
	  glm::vec3(0.15f, 1.8f,  0.6f), glm::vec3(-0.5f, 1.9f,  0.3f),
	  glm::vec3(0.25f, 2.3f, -0.2f),glm::vec3(-0.2f, 2.1f,  0.4f)
	};

	for (size_t i = 0; i < scatteredFlowers.size(); ++i) {
		glm::vec3 scale = glm::vec3(0.1f) * (0.85f + 0.1f * i);
		SetTransformations(scale, 10.0f * (i % 3), 45.0f * (i % 4), 5.0f * (i % 2), rimPosition + scatteredFlowers[i]);
		m_basicMeshes->DrawSphereMesh();
	}
}

void SceneManager::DrawKeyboard(float deskHeight) {
	SetShaderTexture("keyboard_texture");

	// 1. Dimensions of a full-size Magic Keyboard
	
	float keyboardWidth = 11.0f;
	float keyboardDepth = 4.5f;
	float keyboardHeight = 0.4f;
	glm::vec3 scale = glm::vec3(keyboardWidth, keyboardHeight, keyboardDepth);


	// 2.Centered, on top of desk, slightly in front
	glm::vec3 position = glm::vec3(0.0f, deskHeight + keyboardHeight / 2.0f, -0.5f);


	// 3. Calculate UV scale based on *actual* texture aspect ratio and model aspect ratio.
	float textureAspectRatio = 3.63415f; 
	float modelAspectRatio = keyboardWidth / keyboardDepth;

	float uvScaleU = 1.0f;
	float uvScaleV = 1.0f;

	
	// GL_CLAMP_TO_EDGE).
	if (modelAspectRatio > textureAspectRatio) {
		// Model is wider than texture: scale U
		uvScaleU = modelAspectRatio / textureAspectRatio;
		uvScaleV = 1.0f;
	}
	else {
		// Model is "taller" (deeper) than texture: scale V
		uvScaleU = 1.0f;
		uvScaleV = textureAspectRatio / modelAspectRatio;
	}

	// 4. Set UV Scale and Transformations
	SetTextureUVScale(uvScaleU, uvScaleV);
	SetTransformations(scale, 0.0f, 0.0f, 0.0f, position);

	// 5. Draw ONLY the top face.
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::box_top);

	// --- Draw the other faces ---
	
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::box_back);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::box_bottom);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::box_left);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::box_right);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::box_front);
}

void SceneManager::DrawMouse(float deskHeight) {
	SetShaderTexture("mouse_texture");  

	// Elongated sphere for the mouse.
	glm::vec3 scale = glm::vec3(1.25f, 0.2f, 2.0f); // x: width, y: height (very thin), z: length

	// Position: To the right of the keyboard, just above the desk.
	glm::vec3 position = glm::vec3(8.0f, deskHeight + scale.y / 2.0f, -1.0f); // Adjust X and Z as needed.

	SetTransformations(scale, 0.0f, 0.0f, 0.0f, position); // Apply scale and position
	m_basicMeshes->DrawSphereMesh(); // Draw the elongated sphere.
}

void SceneManager::DrawTeacup(float deskHeight) {
	SetShaderMaterial("teacup");

	// --- 1. Bottom (Flattened Half-Sphere) ---
	glm::vec3 bottomScale = glm::vec3(1.5f, 0.5f, 1.5f); // Flatten the sphere on the Y-axis
	glm::vec3 bottomPosition = glm::vec3(12.0f, deskHeight + 0.15f + bottomScale.y, 1.0f); //Position above desk
	SetTransformations(bottomScale, 0.0f, 0.0f, 0.0f, bottomPosition);
	m_basicMeshes->DrawSphereMesh();  

	// --- 2. Body (Cylinder) ---
	glm::vec3 bodyScale = glm::vec3(1.5f, 1.0f, 1.5f); // Diameter and height of the cylinder.
	// Position the cylinder *on top* of the half-sphere:
	glm::vec3 bodyPosition = glm::vec3(12.0f, deskHeight + 0.15f + bottomScale.y + (bodyScale.y / 2.0), 1.0f);
	SetTransformations(bodyScale, 0.0f, 0.0f, 0.0f, bodyPosition);
	m_basicMeshes->DrawCylinderMesh(true, false); // Draw only sides
}
void SceneManager::DrawSaucer(float deskHeight) {
	SetShaderMaterial("saucer");

	// --- 1. Top (Flattened Half-Sphere) ---
	glm::vec3 topScale = glm::vec3(3.0f, 0.4f, 3.0f); // Wider and flatter than teacup bottom
	glm::vec3 topPosition = glm::vec3(12.0f, deskHeight + 0.15f, 1.0f); // Adjust position
	SetTransformations(topScale, 0.0f, 0.0f, 0.0f, topPosition);
	m_basicMeshes->DrawSphereMesh();

	// --- 2. Base (Flattened Cylinder) ---
	glm::vec3 baseScale = glm::vec3(1.5f, 0.2f, 1.5f);  // Smaller diameter, very thin
	// Position *under* the half-sphere:
	glm::vec3 basePosition = glm::vec3(12.0f, deskHeight + 0.15f, 1.0f);
	SetTransformations(baseScale, 0.0f, 0.0f, 0.0f, basePosition);
	m_basicMeshes->DrawCylinderMesh(); //draw all parts of the cylinder
}

void SceneManager::DrawOrganizer(float deskHeight) {
	SetShaderMaterial("organizer");

	
	float baseWidth = 6.0f;
	float baseHeight = 0.5f; // Thickness of the base
	float baseDepth = 8.0f;
	glm::vec3 baseScale = glm::vec3(baseWidth, baseHeight, baseDepth);

	
	glm::vec3 basePosition = glm::vec3(18.0f, deskHeight + baseHeight / 2.0f, 2.0f); 

	SetTransformations(baseScale, 0.0f, 0.0f, 0.0f, basePosition);
	m_basicMeshes->DrawBoxMesh();  // Draw the base

	// --- Back Panel ---
	float backHeight = 10.0f; // Height of the back panel
	glm::vec3 backScale = glm::vec3(baseWidth, backHeight, 0.2f); // Thin back panel
	glm::vec3 backPosition = glm::vec3(basePosition.x, deskHeight + baseHeight + backHeight / 2.0f, basePosition.z - baseDepth / 2.0f + 0.1f); //Behind the base
	SetTransformations(backScale, 0.0f, 0.0f, 0.0f, backPosition);
	m_basicMeshes->DrawBoxMesh();

	// --- Side Panels (Left and Right) ---
	float sideHeight = 10.0f;
	glm::vec3 sideScale = glm::vec3(0.2f, sideHeight, baseDepth);
	// Left
	glm::vec3 leftSidePosition = glm::vec3(basePosition.x - baseWidth / 2.0f + 0.1f, deskHeight + baseHeight + sideHeight / 2.0f, basePosition.z);
	SetTransformations(sideScale, 0.0f, 0.0f, 0.0f, leftSidePosition);
	m_basicMeshes->DrawBoxMesh();
	//Right
	glm::vec3 rightSidePosition = glm::vec3(basePosition.x + baseWidth / 2.0f - 0.1f, deskHeight + baseHeight + sideHeight / 2.0f, basePosition.z);
	SetTransformations(sideScale, 0.0f, 0.0f, 0.0f, rightSidePosition);
	m_basicMeshes->DrawBoxMesh();

	// --- Shelves and Dividers ---
	float shelfThickness = 0.2f;
	float lipHeight = 1.0f;
	float dividerDepth = baseDepth - 0.5f; //  Slightly less deep than the base
	float shelfSpacing = (backHeight - baseHeight) / 5.0f; //  Five shelves

	for (int i = 0; i < 5; ++i) { // Five shelves
		// Shelf
		glm::vec3 shelfScale = glm::vec3(baseWidth - 0.4f, shelfThickness, dividerDepth); // Slightly smaller than base
		glm::vec3 shelfPosition = glm::vec3(basePosition.x, deskHeight + baseHeight + (i + 1) * shelfSpacing, basePosition.z - 0.25f);
		SetTransformations(shelfScale, 0.0f, 0.0f, 0.0f, shelfPosition);
		m_basicMeshes->DrawBoxMesh();

		// Lip (Front edge of the shelf)
		glm::vec3 lipScale = glm::vec3(baseWidth - 0.4f, lipHeight, 0.2f);
		glm::vec3 lipPosition = glm::vec3(basePosition.x, deskHeight + baseHeight + (i + 1) * shelfSpacing + lipHeight / 2.0f - shelfThickness / 2.0f, basePosition.z + dividerDepth / 2.0f - 0.1f); // Front edge
		SetTransformations(lipScale, 0.0f, 0.0f, 0.0f, lipPosition);
		m_basicMeshes->DrawBoxMesh();
	}
}

// --- RenderScene Function ---

void SceneManager::RenderScene() {
		// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/


	//---------------------------------------------------------
	// DARKER RECTANGULAR DESK PLANE
	//---------------------------------------------------------
	// Set rectangular dimensions (X-axis longer than Z-axis)
	scaleXYZ = glm::vec3(25.0f, 1.0f, 12.0f);  // X:25, Z:12 for rectangular shape

	// Position at ground level (Y=0)
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Darkened off-white color (RGB: 232,232,227 in 0-1 range)
	//SetShaderColor(0.91f, 0.91f, 0.89f, 1.0f);

	// Set plane material
	SetShaderMaterial("desk");

	// Draw the desk surface
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/


	


	// 3. DRAW THE SCENE (using helper functions)

	// Desk height.  Adjust if your desk is at a different Y.
	float deskHeight = 0.0f;

	// Draw Keyboard and Mouse.  Place *before* the vase, so the vase is in front.
	DrawKeyboard(deskHeight);
	DrawMouse(deskHeight);

	DrawTeacup(deskHeight);
	DrawSaucer(deskHeight);

	// Draw the monitor
	DrawMonitor(deskHeight);

	// VASE AND PLANT
	const glm::vec3 basePosition(-17.0f, 6.0f, -5.0f); // Define vase base position

	DrawVaseBase(basePosition);
	DrawVaseNeck(basePosition);
	DrawVaseOpening(basePosition);
	DrawVaseRim(basePosition);
	DrawBrownStems(basePosition);
	DrawBeigePuffs(basePosition);
	DrawGreenBranches(basePosition);
	DrawWhiteFlowers(basePosition);

	// Books under vase
	DrawGrayBook(basePosition, deskHeight);      // Bottom, gray
	DrawBlackBook(basePosition, deskHeight);     // Middle, black
	DrawLightBlueBook(basePosition, deskHeight);  // Top, light blue

	DrawOrganizer(deskHeight);
}

