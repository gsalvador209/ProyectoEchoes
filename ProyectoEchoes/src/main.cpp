#define _USE_MATH_DEFINES
#include <cmath>
//glew include
#include <GL/glew.h>

//std includes
#include <string>
#include <iostream>

//glfw include
#include <GLFW/glfw3.h>

// program include
#include "Headers/TimeManager.h"

// Shader include
#include "Headers/Shader.h"

// Model geometric includes
#include "Headers/Sphere.h"
#include "Headers/Cylinder.h"
#include "Headers/Box.h"
#include "Headers/FirstPersonCamera.h"
#include "Headers/ThirdPersonCamera.h"

//GLM include
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Headers/Texture.h"

// Include loader Model class
#include "Headers/Model.h"

// Include Terrain
#include "Headers/Terrain.h"

#include "Headers/AnimationUtils.h"

#include "Headers/Colisiones.h"

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))


	//TODO: 
	//1. Crear collisiones invisibles para deteccción de objetos cercanos a recoger
	//2. Crear collisiones para bordes de terreno
	//3. Crear collisiones para Goyo
	//4. Crear collisiones para elementos de la escena como arboles
	//5. Reempaquetar las colisiones en una sola función
	//6. Corregir botón WASD para que avance siempre en dirección de la cámara

int screenWidth;
int screenHeight;

GLFWwindow *window;

Shader shader;
//Shader con skybox
Shader shaderSkybox;
//Shader con multiples luces
Shader shaderMulLighting;
Shader shaderTerrain;

// Variables para la camara
std::shared_ptr<Camera> camera(new ThirdPersonCamera());
std::shared_ptr<Camera> fp_camera(new FirstPersonCamera());
bool first_person_camera;

//Variables para coliiones
//tag, (collider, transform t_n, transform t_n-1)
std::map<std::string, std::tuple<AbstractModel::SBB,glm::mat4, glm::mat4>> collidersSBB;
std::map<std::string, std::tuple<AbstractModel::OBB,glm::mat4, glm::mat4>> collidersOBB;	

// Variables para la camara y controles
float distanceFromPlayer = 6.5; //Distancia incial de camara al personaje
float angleTarget = 90; //Angulo inical de la cámara
glm::vec3 positionTarget;
bool toogle_CRTL = true;
bool toogle_K = true;
bool toogle_Save = true;
float velocity = 0.07f;
float runVelocity = 3.0f;
bool isJump = false;
float GRAVITY = 5;
double tmv = 0;
double startTimeJump = 0;

//Modo transformación
bool transformModeEna = false;
bool toogleTransformModeEna = true;
int transformModeType = 0; //0.- Translate, 1.- Rotate
bool toogleDebug = true;
int modelSelected = 0;
bool toogleModelSelected = true;
int modelsCount = 0;
std::map<int, glm::mat4*> modelsMapping;
std::map<int, glm::mat4> fileData;	

//Declaracion de los objetos geometricos
Sphere skyboxSphere(20, 20);
Box boxCollider;
Sphere modelSphereCollider(10, 10);
Model goyo;
Model islas;
Model stage;

// Terrain model instance
Terrain terrain(-0.75, -0.75, 600, 4, "../Textures/echoesHeightMap.png");


GLuint textureCespedID, textureTerrainRID,textureTerrainGID,textureTerrainBID,textureTerrainBlendMapID;
GLuint skyboxTextureID;

GLenum types[6] = {
GL_TEXTURE_CUBE_MAP_POSITIVE_X,
GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

std::string fileNames[6] = { "../Textures/mp_bloodvalley/blood-valley_ft.tga",
		"../Textures/mp_bloodvalley/blood-valley_bk.tga",
		"../Textures/mp_bloodvalley/blood-valley_up.tga",
		"../Textures/mp_bloodvalley/blood-valley_dn.tga",
		"../Textures/mp_bloodvalley/blood-valley_rt.tga",
		"../Textures/mp_bloodvalley/blood-valley_lf.tga" };

bool exitApp = false;
int lastMousePosX, offsetX = 0;
int lastMousePosY, offsetY = 0;





// Model matrix definitions DEBEN MAPEARSE EN modelMap
glm::mat4 modelMatrixGoyo = glm::mat4(1.0f);
glm::mat4 modelMatrixIslas = glm::mat4(1.0f);
glm::mat4 modelMatrixStage = glm::mat4(1.0f);

// Animation variables
int animationGoyoIndex = 0;
/*
2 .- Idle
3 .- Walk
4 .- Jump
5 .- Run
*/


double deltaTime;
double currTime, lastTime;

// Se definen todos las funciones.
void reshapeCallback(GLFWwindow *Window, int widthRes, int heightRes);
void keyCallback(GLFWwindow *window, int key, int scancode, int action,
		int mode);
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void mouseCallback(GLFWwindow *window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow *window, int button, int state, int mod);
void init(int width, int height, std::string strTitle, bool bFullScreen);
void destroy();
bool processInput(bool continueApplication = true);
void transformModeFunc(int,glm::mat4*);
void writeModelPositionsToFile(std::map<int, glm::mat4*> modelsMapping);
std::string mat4ToString(const glm::mat4 mat);
std::map<int, glm::mat4> readModelPositionsFromFile();

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
	glfwSetScrollCallback(window,scrollCallback);
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

	fileData = readModelPositionsFromFile();
	modelsCount = fileData.size();
	// auto itModelPositions = fileData.begin();
	// for (; itModelPositions != fileData.end(); itModelPositions++)
	// {
		
	// }
	
	//Recobrar información de los archivos
	
	modelMatrixGoyo = fileData[0];
	modelMatrixIslas = fileData[1];
	modelMatrixStage = fileData[2];

	//Mappeo de matrices
	modelsMapping[0] = &modelMatrixGoyo;
	modelsMapping[1] = &modelMatrixIslas;
	modelsMapping[2] = &modelMatrixStage;


	// Inicialización de los shaders
	shader.initialize("../Shaders/colorShader.vs", "../Shaders/colorShader.fs");
	shaderSkybox.initialize("../Shaders/skyBox.vs", "../Shaders/skyBox.fs");
	shaderMulLighting.initialize("../Shaders/iluminacion_textura_animation.vs", "../Shaders/multipleLights.fs");\
	shaderTerrain.initialize("../Shaders/terrain.vs", "../Shaders/terrain.fs");
	
	
	// Inicializacion de los objetos.
	// Skybox
	skyboxSphere.init();
	skyboxSphere.setShader(&shaderSkybox);
	skyboxSphere.setScale(glm::vec3(20.0f, 20.0f, 20.0f));

	//Box collider
	boxCollider.init();
	boxCollider.setShader(&shader);
	boxCollider.setColor(glm::vec4(0.0f,1.0f, 1.0f, 1.0f));

	modelSphereCollider.init();
	modelSphereCollider.setShader(&shader);
	modelSphereCollider.setColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	//GOYO
	goyo.loadModel("../models/Goyo/Goyo.fbx");
	goyo.setShader(&shaderMulLighting);

	//ISLAS
	islas.loadModel("../models/Islas/Islas.fbx");
	islas.setShader(&shaderMulLighting);

	//STAGE
	stage.loadModel("../models/Stage/Stage.fbx");
	stage.setShader(&shaderMulLighting);
	
	 //modelMatrixIslas = glm::rotate(modelMatrixIslas,glm::radians(-90.0f) , glm::vec3(1.0f, 0.0f, 0.0f));
	// modelMatrixIslas = glm::translate(modelMatrixIslas, glm::vec3(0.0f, 19.0f, 0.0f));
	// modelMatrixIslas = glm::scale(modelMatrixIslas, glm::vec3(0.35f, 0.8f, 0.63f));
	

	// Terreno
	terrain.init();
	terrain.setShader(&shaderTerrain);

	camera->setPosition(glm::vec3(0.0, 3.0, 4.0));
	fp_camera ->setPosition(glm::vec3(modelMatrixGoyo[3])+glm::vec3(0.0f,1.8f,0.0f));



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

	// Definiendo la textura a utilizar
	Texture textureCesped("../Textures/terreno2.png");
	// Carga el mapa de bits (FIBITMAP es el tipo de dato de la libreria)
	textureCesped.loadImage();
	// Creando la textura con id 1
	glGenTextures(1, &textureCespedID);
	// Enlazar esa textura a una tipo de textura de 2D.
	glBindTexture(GL_TEXTURE_2D, textureCespedID);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // set texture wrapping to GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Verifica si se pudo abrir la textura
	if (textureCesped.getData()) {
		// Transferis los datos de la imagen a memoria
		// Tipo de textura, Mipmaps, Formato interno de openGL, ancho, alto, Mipmaps,
		// Formato interno de la libreria de la imagen, el tipo de dato y al apuntador
		// a los datos
		std::cout << "Numero de canales :=> " << textureCesped.getChannels() << std::endl;
		glTexImage2D(GL_TEXTURE_2D, 0, textureCesped.getChannels() == 3 ? GL_RGB : GL_RGBA, textureCesped.getWidth(), textureCesped.getHeight(), 0,
		textureCesped.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, textureCesped.getData());
		// Generan los niveles del mipmap (OpenGL es el ecargado de realizarlos)
		glGenerateMipmap(GL_TEXTURE_2D);
	} else
		std::cout << "Failed to load texture" << std::endl;
	// Libera la memoria de la textura
	textureCesped.freeImage();

	// Definiendo la textura
	Texture textureR("../Textures/adoquin.png");
	textureR.loadImage(); // Cargar la textura
	glGenTextures(1, &textureTerrainRID); // Creando el id de la textura del landingpad
	glBindTexture(GL_TEXTURE_2D, textureTerrainRID); // Se enlaza la textura
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Wrapping en el eje u
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // Wrapping en el eje v
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Filtering de minimización
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtering de maximimizacion
	if(textureR.getData()){
		// Transferir los datos de la imagen a la tarjeta
		glTexImage2D(GL_TEXTURE_2D, 0, textureR.getChannels() == 3 ? GL_RGB : GL_RGBA, textureR.getWidth(), textureR.getHeight(), 0,
		textureR.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, textureR.getData());
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else 
		std::cout << "Fallo la carga de textura" << std::endl;
	textureR.freeImage(); // Liberamos memoria

	Texture textureG("../Textures/piedra2.png");
	textureG.loadImage(); // Cargar la textura
	glGenTextures(1, &textureTerrainGID); // Creando el id de la textura del landingpad
	glBindTexture(GL_TEXTURE_2D, textureTerrainGID); // Se enlaza la textura
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Wrapping en el eje u
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // Wrapping en el eje v
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Filtering de minimización
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtering de maximimizacion
	if(textureG.getData()){
		// Transferir los datos de la imagen a la tarjeta
		glTexImage2D(GL_TEXTURE_2D, 0, textureG.getChannels() == 3 ? GL_RGB : GL_RGBA, textureG.getWidth(), textureG.getHeight(), 0,
		textureG.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, textureG.getData());
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else 
		std::cout << "Fallo la carga de textura" << std::endl;
	textureG.freeImage(); // Liberamos memoria

	Texture textureB("../Textures/camino.png");
	textureB.loadImage(); // Cargar la textura
	glGenTextures(1, &textureTerrainBID); // Creando el id de la textura del landingpad
	glBindTexture(GL_TEXTURE_2D, textureTerrainBID); // Se enlaza la textura
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Wrapping en el eje u
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // Wrapping en el eje v
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Filtering de minimización
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtering de maximimizacion
	if(textureB.getData()){
		// Transferir los datos de la imagen a la tarjeta
		glTexImage2D(GL_TEXTURE_2D, 0, textureB.getChannels() == 3 ? GL_RGB : GL_RGBA, textureB.getWidth(), textureB.getHeight(), 0,
		textureB.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, textureB.getData());
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else 
		std::cout << "Fallo la carga de textura" << std::endl;
	textureB.freeImage(); // Liberamos memoria

	Texture textureBlendMap("../Textures/echoesBlendMap.png");
	textureBlendMap.loadImage(); // Cargar la textura
	glGenTextures(1, &textureTerrainBlendMapID); // Creando el id de la textura del landingpad
	glBindTexture(GL_TEXTURE_2D, textureTerrainBlendMapID); // Se enlaza la textura
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Wrapping en el eje u
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // Wrapping en el eje v
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Filtering de minimización
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtering de maximimizacion
	if(textureBlendMap.getData()){
		// Transferir los datos de la imagen a la tarjeta
		glTexImage2D(GL_TEXTURE_2D, 0, textureBlendMap.getChannels() == 3 ? GL_RGB : GL_RGBA, textureBlendMap.getWidth(), textureBlendMap.getHeight(), 0,
		textureBlendMap.getChannels() == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, textureBlendMap.getData());
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else 
		std::cout << "Fallo la carga de textura" << std::endl;
	textureBlendMap.freeImage(); // Liberamos memoria

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
	goyo.destroy();
	islas.destroy();
	stage.destroy();

	// Terrains objects Delete
	terrain.destroy();

	// Textures Delete
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &textureCespedID);
	glDeleteTextures(1, &textureTerrainRID);
	glDeleteTextures(1, &textureTerrainGID);
	glDeleteTextures(1, &textureTerrainBID);
	glDeleteTextures(1, &textureTerrainBlendMapID);

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

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset){
	distanceFromPlayer -= yoffset;
	camera->setDistanceFromTarget(distanceFromPlayer);
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

	if(GLFW_KEY_T==GLFW_PRESS && toogleDebug){
		transformModeEna = !transformModeEna;
		toogleDebug = false;	
	}else if(GLFW_KEY_T==GLFW_RELEASE){
		toogleDebug = true;
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		camera->mouseMoveCamera(offsetX, offsetY, deltaTime);
	offsetX = 0;
	offsetY = 0;

	if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS){
		camera->mouseMoveCamera(offsetX,0.0,deltaTime);
		//fp_cam->mouseMoveCamera(offsetX,offsetY,deltaTime);	
	}
	if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS)
		camera->mouseMoveCamera(0.0,offsetY,deltaTime);
	offsetX = 0;
	offsetY = 0;

	if(glfwGetKey(window,GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && toogle_CRTL){
		//std::cout << "CTRL PRESSED " << std::endl;
		toogle_CRTL = false;
		if(glfwGetKey(window,GLFW_KEY_K)==GLFW_PRESS && toogle_K){
			std::cout << "First person camera: " << first_person_camera << std::endl; 
			first_person_camera = !first_person_camera;
			toogle_K = false;
		}else if(glfwGetKey(window,GLFW_KEY_K)==GLFW_RELEASE){
			toogle_K = true;
		}
		if(glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS && toogle_Save){
			writeModelPositionsToFile(modelsMapping);
			toogle_Save = false;
			//std::cout<<"Model positions written to file"<<std::endl;
			//return continueApplication;
		}else if(glfwGetKey(window,GLFW_KEY_S)==GLFW_RELEASE){
			toogle_Save = true;
			std::cout << "Model positions written to file" << std::endl;
		}
	}
	if(glfwGetKey(window,GLFW_KEY_LEFT_CONTROL)==GLFW_RELEASE)
		toogle_CRTL = true;

	//************************* */
	//Controles Goyo
	//************************* */
	if(!transformModeEna){
		if(!isJump && (window,GLFW_KEY_SPACE)== GLFW_PRESS && animationGoyoIndex != 9){
			isJump = true;
			tmv = 0;
			startTimeJump = currTime;
			if ((glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS)&&(glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS)) {
				animationGoyoIndex = 11;
			}else{
				animationGoyoIndex = 9;
			}//return continueApplication;
		}
		if(glfwGetKey(window,GLFW_KEY_A)== GLFW_PRESS){
			modelMatrixGoyo = glm::rotate(modelMatrixGoyo,0.04f,glm::vec3(0,1,0));
			angleTarget += 0.04f;
			fp_camera->mouseMoveCamera(-3,0,deltaTime);
		}else if(glfwGetKey(window,GLFW_KEY_D)== GLFW_PRESS){
			modelMatrixGoyo = glm::rotate(modelMatrixGoyo,-0.04f,glm::vec3(0,1,0));
			angleTarget -= 0.04f;
			fp_camera->mouseMoveCamera(3,0,deltaTime);
		}
		if(glfwGetKey(window,GLFW_KEY_W)== GLFW_PRESS){
			if(glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS){
				modelMatrixGoyo = glm::translate(modelMatrixGoyo,glm::vec3(runVelocity,0,0));
				if (glfwGetKey(window,GLFW_KEY_SPACE)== GLFW_PRESS){
					animationGoyoIndex = 11;
				}else if(animationGoyoIndex != 11){
					animationGoyoIndex = 5;
				}
			}else{
				modelMatrixGoyo = glm::translate(modelMatrixGoyo,glm::vec3(velocity,0,0));
				animationGoyoIndex = 3;
			}
		}else if(glfwGetKey(window,GLFW_KEY_S)== GLFW_PRESS){
			modelMatrixGoyo = glm::translate(modelMatrixGoyo,glm::vec3(-velocity,0,0));
			animationGoyoIndex = 3;
		}

		bool keySpaceStatus = glfwGetKey(window,GLFW_KEY_SPACE) == GLFW_PRESS;
		if(keySpaceStatus && !isJump){
			isJump = true;
			startTimeJump = currTime;
			tmv = 0; 
		}
	
	}

	if(glfwGetKey(window,GLFW_KEY_TAB)==GLFW_PRESS && toogleModelSelected){
		modelSelected= (modelSelected+1)%modelsCount;
		std::cout << "Model selected: " << modelSelected << std::endl;
		toogleModelSelected = false;
	}else if(glfwGetKey(window,GLFW_KEY_TAB)==GLFW_RELEASE){
		toogleModelSelected = true;
	}

	if(glfwGetKey(window,GLFW_KEY_J)==GLFW_PRESS && toogleTransformModeEna){
		transformModeEna = !transformModeEna;
		toogleTransformModeEna = false;
		std::cout << "Transform mode: " << transformModeEna << std::endl;
	}else if (glfwGetKey(window,GLFW_KEY_J)==GLFW_RELEASE){
		toogleTransformModeEna = true;
	}

	if(glfwGetKey(window,GLFW_KEY_G)==GLFW_PRESS){
		transformModeType = 0;
		std::cout << "Translate mode" << std::endl;
	}
	if(glfwGetKey(window,GLFW_KEY_R)==GLFW_PRESS){
		transformModeType = 1;
		std::cout << "Rotate mode" << std::endl;
	}
	if(glfwGetKey(window,GLFW_KEY_Y)==GLFW_PRESS){
		transformModeType = 2;
		std::cout << "Scale mode" << std::endl;
	}

	glfwPollEvents();
	return continueApplication;
}

void applicationLoop() {
	bool psi = true;


	// Variables to interpolation key frames
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
		std::map<std::string,bool> collisionDetection;

		if(transformModeEna){
			//std::cout << "Transform mode" << std::endl;
			//std::cout << "Model selected: " << modelSelected << std::endl;
			glm::mat4* model = modelsMapping[modelSelected];
			transformModeFunc(transformModeType,model);
		}



		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Configuración de perspectiva y camara 
		glm::mat4 projection = glm::perspective(glm::radians(45.0f),
				(float) screenWidth / (float) screenHeight, 0.01f, 1000.0f);
		camera->setSensitivity(1.2);
		camera->setDistanceFromTarget(distanceFromPlayer);
		positionTarget = modelMatrixGoyo[3]+glm::vec4(0.0f,2.0f,0.0f,0.0f);
		camera->setCameraTarget(positionTarget);
		camera->setAngleTarget(angleTarget);
		camera->updateCamera();
		fp_camera->setPosition(glm::vec3(modelMatrixGoyo[3])+glm::vec3(0.0f,3.5f,0.0f));

		glm::mat4 view = glm::mat4(1.0f);
		if(first_person_camera)
			view = fp_camera->getViewMatrix();
		else
			view = camera->getViewMatrix();

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
		shaderTerrain.setMatrix4("view", 1, false,glm::value_ptr(view));


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

		/*******************************************
		 * Terrain Cesped
		 *******************************************/
		glm::mat4 modelCesped = glm::mat4(1.0);
		modelCesped = glm::translate(modelCesped, glm::vec3(0.0, 0.0, 0.0));
		modelCesped = glm::scale(modelCesped, glm::vec3(200.0, 0.001, 200.0));
		// Se activa la textura del agua
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureCespedID);
		shaderTerrain.setInt("backgroundTexture", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textureTerrainRID);
		shaderTerrain.setInt("rTexture", 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, textureTerrainGID);
		shaderTerrain.setInt("gTexture", 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, textureTerrainBID);
		shaderTerrain.setInt("bTexture", 3);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, textureTerrainBlendMapID);
		shaderTerrain.setInt("blendMapTexture", 4);

		shaderTerrain.setVectorFloat2("scaleUV", glm::value_ptr(glm::vec2(80, 80)));
		terrain.setPosition(glm::vec3(100, 0, 100));
		terrain.render();
		shaderTerrain.setVectorFloat2("scaleUV", glm::value_ptr(glm::vec2(0, 0)));
		glBindTexture(GL_TEXTURE_2D, 0);
		
		/*******************************************
		 * Custom objects obj
		 *******************************************/


		//Goyo
		float z_goyo = terrain.getHeightTerrain(modelMatrixGoyo[3][0], modelMatrixGoyo[3][2]);	
		glm::mat4 modelMatrixGoyoBody = glm::mat4(modelMatrixGoyo);
		modelMatrixGoyoBody = glm::scale(modelMatrixGoyoBody, glm::vec3(0.0025f));
		modelMatrixGoyo[3][1] = z_goyo;
		modelMatrixGoyo[3][1] = -GRAVITY*tmv*tmv + 4*tmv + z_goyo;
		tmv = currTime-startTimeJump;
		goyo.setAnimationIndex(animationGoyoIndex);
		
		if(modelMatrixGoyo[3][1] < z_goyo){
			modelMatrixGoyo[3][1] = z_goyo;
			isJump = false;
			animationGoyoIndex = 2;
		}else{
			animationGoyoIndex = 9;
		}
		goyo.render(modelMatrixGoyoBody);
		
		//Islas
		islas.render(modelMatrixIslas);

		//Stage
		stage.render(modelMatrixStage);



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


		//Pruebas de collision
		auto itOBBC = collidersOBB.begin();
		for( ; itOBBC != collidersOBB.end(); itOBBC++){
			bool isCollision = false;
			auto jtOBBC = collidersOBB.begin();
			for(; jtOBBC != collidersOBB.end(); jtOBBC++){
				if(itOBBC != jtOBBC && testOBBOBB(std::get<0>(itOBBC->second),std::get<0>(jtOBBC->second))){
					std::cout << "Colision entre " << itOBBC->first << " y " << jtOBBC->first << std::endl;	
					isCollision = true;
					addOrUpdateCollisionDetection(collisionDetection,itOBBC->first,isCollision);	
				}
			}
			addOrUpdateCollisionDetection(collisionDetection,itOBBC->first,isCollision);
		}

		//Cajas vs esferas		
		//Caja contra caja
		itOBBC = collidersOBB.begin();
		for( ; itOBBC != collidersOBB.end() ; itOBBC++){
			bool isCollision = false;
			auto itSBBC = collidersSBB.begin();
			for(;itSBBC!=collidersSBB.end();itSBBC++){
				if(testSphereOBox(std::get<0>(itSBBC->second),std::get<0>(itOBBC->second))){
					std::cout << "Estan colisionando" << itSBBC->first << "y" << itOBBC->first << std::endl;
					isCollision = true;
					addOrUpdateCollisionDetection(collisionDetection,itSBBC->first,true);
				}
			}
			addOrUpdateCollisionDetection(collisionDetection,itOBBC->first,isCollision);
		}

		// ESFERA CONTRA ESFERA
		auto itSBBC = collidersSBB.begin();
		for(;itSBBC!=collidersSBB.end();itSBBC++){
			auto jtSBBC = collidersSBB.begin();
			bool isCollision = false;
			for(;jtSBBC!=collidersSBB.end();jtSBBC++){
				if(itSBBC != jtSBBC && testSphereSphereIntersection(
					std::get<0>(itSBBC->second),std::get<0>(jtSBBC->second))){
						std::cout << "Estan colisionando" << itSBBC->first << "y" << itOBBC->first << std::endl;
						isCollision = true;
						addOrUpdateCollisionDetection(collisionDetection,jtSBBC->first,true);
				}
				addOrUpdateCollisionDetection(collisionDetection,itSBBC->first,isCollision);
			}
		}
		
		
		glfwSwapBuffers(window);
	}
}

//Modo para mover obejetos
void transformModeFunc(int mode, glm::mat4 * modelMatrix){
	//glm::mat4 modelMatrix = glm::make_mat4(modelMatrixModel);
	if (mode == 0){ //Traslación
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
			*modelMatrix = glm::translate(*modelMatrix, glm::vec3(0.0, 0.0, 0.1));
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
			*modelMatrix = glm::translate(*modelMatrix, glm::vec3(0.0, 0.0, -0.1));
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
			*modelMatrix = glm::translate(*modelMatrix, glm::vec3(-0.1, 0.0, 0.0));
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
			*modelMatrix = glm::translate(*modelMatrix, glm::vec3(0.1, 0.0, 0.0));
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
			*modelMatrix = glm::translate(*modelMatrix, glm::vec3(0.0, 0.1, 0.0));
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
			*modelMatrix = glm::translate(*modelMatrix, glm::vec3(0.0, -0.1, 0.0));
		}
	}else if (mode == 1){ //Rotación
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
			*modelMatrix = glm::rotate(*modelMatrix, glm::radians(0.05f), glm::vec3(1, 0, 0));
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
			*modelMatrix = glm::rotate(*modelMatrix, glm::radians(-0.05f), glm::vec3(1, 0, 0));
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
			*modelMatrix = glm::rotate(*modelMatrix, glm::radians(0.05f), glm::vec3(0, 1, 0));
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
			*modelMatrix = glm::rotate(*modelMatrix, glm::radians(-0.05f), glm::vec3(0, 1, 0));
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
			*modelMatrix = glm::rotate(*modelMatrix, glm::radians(0.05f), glm::vec3(0, 0, 1));
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
			*modelMatrix = glm::rotate(*modelMatrix, glm::radians(-0.05f), glm::vec3(0, 0, 1));
		}
	} else if (mode == 2){
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
			*modelMatrix = glm::scale(*modelMatrix, glm::vec3(1.0, 1.0, 1.01));
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
			*modelMatrix = glm::scale(*modelMatrix, glm::vec3(1.0, 1.0, 0.99));
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
			*modelMatrix = glm::scale(*modelMatrix, glm::vec3(0.99, 1.0, 1.0));
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
			*modelMatrix = glm::scale(*modelMatrix, glm::vec3(1.01, 1.0, 1.0));
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
			*modelMatrix = glm::scale(*modelMatrix, glm::vec3(1.0, 1.01, 1.0));
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
			*modelMatrix = glm::scale(*modelMatrix, glm::vec3(1.0, 0.99, 1.0));
		}
	}
}

void writeModelPositionsToFile(std::map<int, glm::mat4*> modelsMapping) {
	std::ofstream outFile("../models/positions.txt");
	std::stringstream ss;
	if (!outFile) {
		std::cerr << "Failed to open file for writing" << std::endl;
		return;
	}
	//std::cout << "Writing model positions to file" << std::endl;


	for (int i= 0 ; i < modelsCount ; i++){
		ss << i << " " << mat4ToString(*modelsMapping[i]) <<std::endl;
		//std::cout << mat4ToString(*modelsMapping[0]) << std::endl;
	}
	outFile << ss.str();
	outFile.close();
}

std::map<int, glm::mat4> readModelPositionsFromFile() {
	std::map<int, glm::mat4> modelsMapping;
	std::ifstream inFile("../models/positions.txt");
	if (!inFile) {
		std::cerr << "Failed to open file for reading" << std::endl;
		return modelsMapping;
	}

	std::string line;
	while (std::getline(inFile, line)) {
		std::istringstream ss(line);
		int id;
		ss >> id;
		glm::mat4 mat;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				ss >> mat[i][j];
			}
		}
		modelsMapping[id] = mat;
	}

	inFile.close();
	return modelsMapping;
}

std::string mat4ToString(const glm::mat4 mat) {
	std::stringstream ss;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			ss << mat[i][j];
			if (i != 3 || j != 3) {
				ss << " ";
			}
		}
	}
	return ss.str();
}

int main(int argc, char **argv) {
	init(800, 700, "Window GLFW", false);
	applicationLoop();
	destroy();
	return 1;
}
///XDDXD