#define _USE_MATH_DEFINES
#include <cmath>


#include <string>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// program include
#include "Headers/TimeManager.h"
#include "Headers/Shader.h"
#include "Headers/Sphere.h"
#include "Headers/Cylinder.h"
#include "Headers/Box.h"
#include "Headers/FirstPersonCamera.h"
#include "Headers/Texture.h"
#include "Headers/AnimationUtils.h"
#include "Headers/Terrain.h"
#include "Headers/Model.h"


//GLM include
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

int screenWidth;
int screenHeight;

GLFWwindow *window;

Shader shader;
//Shader con skybox
Shader shaderSkybox;
//Shader con multiples luces
Shader shaderMulLighting;

Shader shaderTerrain;

std::shared_ptr<FirstPersonCamera> camera(new FirstPersonCamera());

Sphere skyboxSphere(20, 20);

// Model declarations
Terrain terrain(-1, -1, 1000, 8, "../Textures/p4/echoesHeightMap.png");

Model pepsiman;

GLuint textureCaminoID, textureAdoquinID, texturePiedraID, textureBlendMapID, texturePastoID;
GLuint skyboxTextureID;

GLenum types[6] = {
GL_TEXTURE_CUBE_MAP_POSITIVE_X,
GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

//skybox filepaths

std::string fileNames[6] = { "../Textures/mp_bloodvalley/blood-valley_ft.tga",
		"../Textures/mp_bloodvalley/blood-valley_bk.tga",
		"../Textures/mp_bloodvalley/blood-valley_up.tga",
		"../Textures/mp_bloodvalley/blood-valley_dn.tga",
		"../Textures/mp_bloodvalley/blood-valley_rt.tga",
		"../Textures/mp_bloodvalley/blood-valley_lf.tga" };

bool exitApp = false;
int lastMousePosX, offsetX = 0;
int lastMousePosY, offsetY = 0;

// Model matrix definitions
glm::mat4 modelMatrixPepsiman = glm::mat4(1.0f);


double deltaTime;
double currTime, lastTime;

// Se definen todos las funciones.
void reshapeCallback(GLFWwindow *Window, int widthRes, int heightRes);
void keyCallback(GLFWwindow *window, int key, int scancode, int action,
		int mode);
void mouseCallback(GLFWwindow *window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow *window, int button, int state, int mod);
void init(int width, int height, std::string strTitle, bool bFullScreen);
void destroy();
bool processInput(bool continueApplication = true);

// Implementacion de todas las funciones.
void init(int width, int height, std::string strTitle, bool bFullScreen) {

	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		exit(-1);
	}

	screenWidth = width;
	screenHeight = height;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (bFullScreen)
		window = glfwCreateWindow(width, height, strTitle.c_str(),
				glfwGetPrimaryMonitor(), nullptr);
	else
		window = glfwCreateWindow(width, height, strTitle.c_str(), nullptr,
				nullptr);

	if (window == nullptr) {
		std::cerr
				<< "Error to create GLFW window, you can try download the last version of your video card that support OpenGL 3.3+"
				<< std::endl;
		destroy();
		exit(-1);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	glfwSetWindowSizeCallback(window, reshapeCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Init glew
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		std::cerr << "Failed to initialize glew" << std::endl;
		exit(-1);
	}

	glViewport(0, 0, screenWidth, screenHeight);
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Inicialización de los shaders
	shader.initialize("../Shaders/colorShader.vs", "../Shaders/colorShader.fs");
	shaderSkybox.initialize("../Shaders/skyBox.vs", "../Shaders/skyBox.fs");
	shaderMulLighting.initialize("../Shaders/iluminacion_texture_res.vs", "../Shaders/multipleLights.fs");
	shaderTerrain.initialize("../Shaders/terrain.vs", "../Shaders/terrain.fs");

	terrain.init();
	terrain.setShader(&shaderTerrain);

	pepsiman.loadModel("../Models/Pepsiman/pepsiman.fbx");
	pepsiman.setShader(&shaderMulLighting);


	camera->setPosition(glm::vec3(0.0, 3.0, 4.0));
	
	// Carga de texturas para el skybox
	Texture skyboxTexture = Texture("");
	glGenTextures(1, &skyboxTextureID);
	// Tipo de textura CUBE MAP
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	for (int i = 0; i < ARRAY_SIZE_IN_ELEMENTS(types); i++) {
		skyboxTexture = Texture(fileNames[i]);
		skyboxTexture.loadImage(true);
		if (skyboxTexture.getData()) {
			glTexImage2D(types[i], 0, skyboxTexture.getChannels() == 3 ? GL_RGB : GL_RGBA, skyboxTexture.getWidth(), skyboxTexture.getHeight(), 0,
			skyboxTexture.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, skyboxTexture.getData());
		} else
			std::cout << "Failed to load texture" << std::endl;
		skyboxTexture.freeImage();
	}

	Texture textureBlendMap("../Textures/echoesBlendMap.png");
	textureBlendMap.loadImage();
	glGenTextures(1, &textureBlendMapID);
	glBindTexture(GL_TEXTURE_2D, textureBlendMapID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (textureBlendMap.getData()) {
		std::cout << "Numero de canales :=> " << textureBlendMap.getChannels() << std::endl;
		glTexImage2D(GL_TEXTURE_2D, 0, textureBlendMap.getChannels() == 3 ? GL_RGB : GL_RGBA, textureBlendMap.getWidth(), textureBlendMap.getHeight(), 0,
		textureBlendMap.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, textureBlendMap.getData());
		// Generan los niveles del mipmap (OpenGL es el ecargado de realizarlos)
		glGenerateMipmap(GL_TEXTURE_2D);
	} else
		std::cout << "Failed to load texture" << std::endl;
	// Libera la memoria de la textura
	textureBlendMap.freeImage();


	Texture textureCamino("../Textures/camino.png");
	textureCamino.loadImage();
	glGenTextures(1, &textureCaminoID);
	glBindTexture(GL_TEXTURE_2D, textureCaminoID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (textureCamino.getData()) {
		std::cout << "Numero de canales :=> " << textureCamino.getChannels() << std::endl;
		glTexImage2D(GL_TEXTURE_2D, 0, textureCamino.getChannels() == 3 ? GL_RGB : GL_RGBA, textureCamino.getWidth(), textureCamino.getHeight(), 0,
		textureCamino.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, textureCamino.getData());
		// Generan los niveles del mipmap (OpenGL es el ecargado de realizarlos)
		glGenerateMipmap(GL_TEXTURE_2D);
	} else
		std::cout << "Failed to load texture" << std::endl;
	// Libera la memoria de la textura
	textureCamino.freeImage();


	Texture textureAdoquin("../Textures/adoquin.png");
	textureAdoquin.loadImage();
	glGenTextures(1, &textureAdoquinID);
	glBindTexture(GL_TEXTURE_2D, textureAdoquinID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (textureAdoquin.getData()) {
		std::cout << "Numero de canales :=> " << textureAdoquin.getChannels() << std::endl;
		glTexImage2D(GL_TEXTURE_2D, 0, textureAdoquin.getChannels() == 3 ? GL_RGB : GL_RGBA, textureAdoquin.getWidth(), textureAdoquin.getHeight(), 0,
		textureAdoquin.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, textureAdoquin.getData());
		// Generan los niveles del mipmap (OpenGL es el ecargado de realizarlos)
		glGenerateMipmap(GL_TEXTURE_2D);
	} else
		std::cout << "Failed to load texture" << std::endl;
	// Libera la memoria de la textura
	textureAdoquin.freeImage();


	Texture texturePiedra("../Textures/piedra.png");
	texturePiedra.loadImage();
	glGenTextures(1, &texturePiedraID);
	glBindTexture(GL_TEXTURE_2D, texturePiedraID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (texturePiedra.getData()) {
		std::cout << "Numero de canales :=> " << texturePiedra.getChannels() << std::endl;
		std::cout << "AAAA" << std::endl;
		glTexImage2D(GL_TEXTURE_2D, 0, texturePiedra.getChannels() == 3 ? GL_RGB : GL_RGBA, texturePiedra.getWidth(), texturePiedra.getHeight(), 0,
		texturePiedra.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, texturePiedra.getData());
		std::cout << "AAAAA2" << std::endl;
		// Generan los niveles del mipmap (OpenGL es el ecargado de realizarlos)
		glGenerateMipmap(GL_TEXTURE_2D);
	} else
		std::cout << "Failed to load texture" << std::endl;
	// Libera la memoria de la textura
	texturePiedra.freeImage();
	// Texture texturePasto("../Textures/terreno2.png");
	// texturePasto.loadImage();
	// glGenTextures(1, &texturePastoID);
	// glBindTexture(GL_TEXTURE_2D, texturePastoID);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// if (texturePasto.getData()) {
	// 	std::cout << "Numero de canales :=> " << texturePasto.getChannels() << std::endl;
	// 	glTexImage2D(GL_TEXTURE_2D, 0, texturePasto.getChannels() == 3 ? GL_RGB : GL_RGBA, texturePasto.getWidth(), texturePasto.getHeight(), 0,
	// 	texturePasto.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, texturePasto.getData());
	// 	// Generan los niveles del mipmap (OpenGL es el ecargado de realizarlos)
	// 	glGenerateMipmap(GL_TEXTURE_2D);
	// } else
	// 	std::cout << "Failed to load texture" << std::endl;
	// // Libera la memoria de la textura
	// texturePasto.freeImage();
}

void destroy() {
	glfwDestroyWindow(window);
	glfwTerminate();
	// --------- IMPORTANTE ----------
	// Eliminar los shader y buffers creados.

	// Shaders Delete
	shader.destroy();
	shaderMulLighting.destroy();
	shaderSkybox.destroy();
	shaderTerrain.destroy();

	// Basic objects Delete
	skyboxSphere.destroy();



	// Custom objects Delete
	pepsiman.destroy();


	// Textures Delete
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &textureCaminoID);
	glDeleteTextures(1, &textureAdoquinID);
	glDeleteTextures(1, &texturePiedraID);
	glDeleteTextures(1, &textureBlendMapID);
	glDeleteTextures(1, &texturePastoID);


	// Cube Maps Delete
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDeleteTextures(1, &skyboxTextureID);

}

void reshapeCallback(GLFWwindow *Window, int widthRes, int heightRes) {
	screenWidth = widthRes;
	screenHeight = heightRes;
	glViewport(0, 0, widthRes, heightRes);
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action,
		int mode) {
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			exitApp = true;
			break;
		}
	}
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos) {
	offsetX = xpos - lastMousePosX;
	offsetY = ypos - lastMousePosY;
	lastMousePosX = xpos;
	lastMousePosY = ypos;
}

void mouseButtonCallback(GLFWwindow *window, int button, int state, int mod) {
	if (state == GLFW_PRESS) {
		switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			std::cout << "lastMousePos.y:" << lastMousePosY << std::endl;
			break;
		case GLFW_MOUSE_BUTTON_LEFT:
			std::cout << "lastMousePos.x:" << lastMousePosX << std::endl;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			std::cout << "lastMousePos.x:" << lastMousePosX << std::endl;
			std::cout << "lastMousePos.y:" << lastMousePosY << std::endl;
			break;
		}
	}
}

bool processInput(bool continueApplication) {
	if (exitApp || glfwWindowShouldClose(window) != 0) {
		return false;
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera->moveFrontCamera(true, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera->moveFrontCamera(false, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera->moveRightCamera(false, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera->moveRightCamera(true, deltaTime);
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		camera->mouseMoveCamera(offsetX, offsetY, deltaTime);
	offsetX = 0;
	offsetY = 0;


	glfwPollEvents();
	return continueApplication;
}

void applicationLoop() {
	bool psi = true;
    // Model matrix transformations
	modelMatrixPepsiman = glm::translate(modelMatrixPepsiman, glm::vec3(5.0f, 0.05, 0.0f));

	lastTime = TimeManager::Instance().GetTime();

	while (psi) {
		currTime = TimeManager::Instance().GetTime();
		if(currTime - lastTime < 0.016666667){
			glfwPollEvents();
			continue;
		}
		lastTime = currTime;
		TimeManager::Instance().CalculateFrameRate(true);
		deltaTime = TimeManager::Instance().DeltaTime;
		psi = processInput(true);
        
		glm::mat4 projection = glm::perspective(glm::radians(45.0f),
				(float) screenWidth / (float) screenHeight, 0.01f, 100.0f);
		glm::mat4 view = camera->getViewMatrix();

		// Settea la matriz de vista y projection al shader con solo color
		shader.setMatrix4("projection", 1, false, glm::value_ptr(projection));
		shader.setMatrix4("view", 1, false, glm::value_ptr(view));

		// Settea la matriz de vista y projection al shader con skybox
		shaderSkybox.setMatrix4("projection", 1, false,
				glm::value_ptr(projection));
		shaderSkybox.setMatrix4("view", 1, false,
				glm::value_ptr(glm::mat4(glm::mat3(view))));
		// Settea la matriz de vista y projection al shader con multiples luces
		shaderMulLighting.setMatrix4("projection", 1, false,
					glm::value_ptr(projection));
		shaderMulLighting.setMatrix4("view", 1, false,
				glm::value_ptr(view));
		shaderTerrain.setMatrix4("projection", 1, false,
				glm::value_ptr(projection));
		shaderTerrain.setMatrix4("view", 1, false,
				glm::value_ptr(view));

		/*******************************************
		 * Propiedades Luz direccional
		 *******************************************/
		shaderMulLighting.setVectorFloat3("viewPos", glm::value_ptr(camera->getPosition()));
		shaderMulLighting.setVectorFloat3("directionalLight.light.ambient", glm::value_ptr(glm::vec3(0.3, 0.3, 0.3)));
		shaderMulLighting.setVectorFloat3("directionalLight.light.diffuse", glm::value_ptr(glm::vec3(0.7, 0.7, 0.7)));
		shaderMulLighting.setVectorFloat3("directionalLight.light.specular", glm::value_ptr(glm::vec3(0.9, 0.9, 0.9)));
		shaderMulLighting.setVectorFloat3("directionalLight.direction", glm::value_ptr(glm::vec3(-1.0, 0.0, 0.0)));

		shaderTerrain.setVectorFloat3("viewPos", glm::value_ptr(camera->getPosition()));
		shaderTerrain.setVectorFloat3("directionalLight.light.ambient", glm::value_ptr(glm::vec3(0.3, 0.3, 0.3)));
		shaderTerrain.setVectorFloat3("directionalLight.light.diffuse", glm::value_ptr(glm::vec3(0.7, 0.7, 0.7)));
		shaderTerrain.setVectorFloat3("directionalLight.light.specular", glm::value_ptr(glm::vec3(0.9, 0.9, 0.9)));
		shaderTerrain.setVectorFloat3("directionalLight.direction", glm::value_ptr(glm::vec3(-1.0, 0.0, 0.0)));

		/*******************************************
		 * Propiedades SpotLights
		 *******************************************/
		shaderMulLighting.setInt("spotLightCount", 0);
		shaderTerrain.setInt("spotLightCount", 0);

		/*******************************************
		 * Propiedades PointLights
		 *******************************************/
		shaderMulLighting.setInt("pointLightCount", 0);
		shaderTerrain.setInt("pointLightCount", 0);

		/********************************************
		 * TERRAIN
		 ********************************************/
		glm::mat4 modelCesped = glm::mat4(1.0);
		modelCesped = glm::translate(modelCesped, glm::vec3(0.0, 0.0, 0.0));
		modelCesped = glm::scale(modelCesped, glm::vec3(200.0, 0.001, 200.0));
	
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureBlendMapID);
		shaderTerrain.setInt("backgroundTexture", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textureAdoquinID);
		shaderTerrain.setInt("rTexture", 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texturePiedraID);
		shaderTerrain.setInt("gTexture", 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, textureCaminoID);
		shaderTerrain.setInt("bTexture", 3);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, textureBlendMapID);
		shaderTerrain.setInt("blendMapTexture", 4);

		shaderTerrain.setVectorFloat2("scaleUV", glm::value_ptr(glm::vec2(80, 80)));
		terrain.setPosition(glm::vec3(100, 0, 100));
		terrain.render();
		shaderTerrain.setVectorFloat2("scaleUV", glm::value_ptr(glm::vec2(0, 0)));
		glBindTexture(GL_TEXTURE_2D, 0);

		/*******************************************
		 * Custom objects obj
		 *******************************************/
		
		modelMatrixPepsiman[3][1] = terrain.getHeightTerrain(modelMatrixPepsiman[3][0], modelMatrixPepsiman[3][2]);
		glm::mat4 modelMatrixPepsimanBody = glm::mat4(modelMatrixPepsiman);
		modelMatrixPepsimanBody = glm::scale(modelMatrixPepsimanBody, glm::vec3(0.009f));
		pepsiman.setAnimationIndex(0);
		//pepsiman.enableWireMode();
		pepsiman.render(modelMatrixPepsimanBody);
		pepsiman.enableFillMode();
	

		/*******************************************
		 * Skybox
		 *******************************************/
		GLint oldCullFaceMode;
		GLint oldDepthFuncMode;
		// deshabilita el modo del recorte de caras ocultas para ver las esfera desde adentro
		glGetIntegerv(GL_CULL_FACE_MODE, &oldCullFaceMode);
		glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFuncMode);
		shaderSkybox.setFloat("skybox", 0);
		glCullFace(GL_FRONT);
		glDepthFunc(GL_LEQUAL);
		glActiveTexture(GL_TEXTURE0);
		skyboxSphere.render();
		glCullFace(oldCullFaceMode);
		glDepthFunc(oldDepthFuncMode);

		glfwSwapBuffers(window);
	}
}

int main(int argc, char **argv) {
	init(800, 700, "Window GLFW", false);
	applicationLoop();
	destroy();
	return 1;
}