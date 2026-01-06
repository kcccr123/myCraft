#include<iostream>
// were using Emscripten for web compile
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#ifndef __EMSCRIPTEN__
#include<glad/glad.h>
#endif
#include<GLFW/glfw3.h>
#ifdef __EMSCRIPTEN__
#include<GLES3/gl3.h>
#endif
#include<stb/stb_image.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include"Texture.h"
#include"shaders/shaderClass/shaderClass.h"
#include"shaders/VAO.h"
#include"shaders/VBO.h"
#include"shaders/EBO.h"
#include"player/Camera.h"
#include"player/Player.h"

#include"entities/header/Block.h"
#include "entities/header/Map.h"

#include "gameLogic/UpdatePacket.h"
#include "gameLogic/DestroyPacket.h"

#include "constants.h"

#include"entities/header/Model/Mesh.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


Vertex lightVertices[] =
{
	// Updated coordinates to make the light block 0.5 times in size
	Vertex{glm::vec3(-0.5f, -0.5f,  0.5f)},
	Vertex{glm::vec3(-0.5f, -0.50, -0.5f)},
	Vertex{glm::vec3(0.5f, -0.5f, -0.5f)},
	Vertex{glm::vec3(0.5f, -0.5f,  0.5f)},
	Vertex{glm::vec3(-0.5f,  0.5f,  0.5f)},
	Vertex{glm::vec3(-0.5f,  0.5f, -0.5f)},
	Vertex{glm::vec3(0.5f,  0.5f, -0.5f)},
	Vertex{glm::vec3(0.5f,  0.5f,  0.5f)}
};

GLuint lightIndices[] =
{
	0, 1, 2,
	0, 2, 3,
	0, 4, 7,
	0, 7, 3,
	3, 7, 6,
	3, 6, 2,
	2, 6, 5,
	2, 5, 1,
	1, 5, 4,
	1, 4, 0,
	4, 5, 6,
	4, 6, 7
};

const unsigned int width = 1200;
const unsigned int height = 800;

// Global game state structure for Emscripten main loop
// Game context is now stored on heap instead of stack since emscripten kills stack every frame
struct GameContext {
	GLFWwindow* window;
	Shader* shaderProgram;
	Shader* lightShader;
	Mesh* light;
	Map* map;
	Player* camera;

	// Frame timing
	double prevTime;
	unsigned int counter;

	// World state
	int posX;
	int posY;
	std::deque<std::unique_ptr<UpdatePacket>> updateQue;
	std::vector<Triangle> playerVerts;
	std::vector<glm::vec3> blockCords;

	// ImGui
	ImFontAtlas* fontAtlas;

	GameContext() : window(nullptr), shaderProgram(nullptr), lightShader(nullptr), 
		light(nullptr), map(nullptr), camera(nullptr), 
		prevTime(0.0), counter(0), posX(0), posY(0), fontAtlas(nullptr) {}
};

// Global context pointer
GameContext* g_ctx = nullptr;

// Main game loop iteration - runs once per frame
void main_loop_iteration() {
	if (!g_ctx) return;

	double crntTime = glfwGetTime();
	double timeDiff = crntTime - g_ctx->prevTime;
	g_ctx->counter++;

	if (timeDiff >= 1.0 / 30.0)
	{
		// Creates new title
		std::string FPS = std::to_string((1.0 / timeDiff) * g_ctx->counter);
		std::string ms = std::to_string((timeDiff / g_ctx->counter) * 1000);
		std::string newTitle = "MyCraft - " + FPS + "FPS / " + ms + "ms";
		glfwSetWindowTitle(g_ctx->window, newTitle.c_str());

		// Resets times and counter
		g_ctx->prevTime = crntTime;
		g_ctx->counter = 0;
	}

	bool updateFlag = false;
	if (g_ctx->updateQue.size() > 0) {
		//printf("%d \n", updateQue[0]->getActionID());
		switch (g_ctx->updateQue[0]->getActionID()) {
		case 1:
		{
			// Player Delete Block
			glm::vec3 deleteBlock = g_ctx->updateQue[0]->conductAction();

			//add block to inventory
			// 1. get destryoed blockid, (might need to get a return value from removeBlockFromChunk)
			// 2. call addBlockToInventory from player class
			g_ctx->camera->addItemToInventory(g_ctx->map->removeBlockFromChunk(0, 0, deleteBlock.x, deleteBlock.y, deleteBlock.z), 1);
			break;
		}
		case 2:
		{
			// Player add block from inventory
			glm::vec3 addBlock = g_ctx->updateQue[0]->conductAction();
			if (g_ctx->camera->getSelectedSlot() != -1) {
				g_ctx->map->addBlockToChunk(0, 0, addBlock.x, addBlock.y, addBlock.z, g_ctx->camera->getInventory()[g_ctx->camera->getSelectedSlot()][1]);
				g_ctx->camera->removeItemFromInventory(1, g_ctx->camera->getSelectedSlot());
				if (g_ctx->camera->getInventory()[g_ctx->camera->getSelectedSlot()][1] == -1) {
					g_ctx->camera->setSelectedSlot(-1);
				}
			}
			break;
		}
		case 3:

			break;
		default:
			printf(" no update \n");
		}

		g_ctx->updateQue.pop_front();
		updateFlag = true;
	}

	// World Generation Logic
	int newX = static_cast<int>(round((g_ctx->camera->Position.x - 16) / Constants::CHUNK_SIZE));
	int newY = static_cast<int>(round((g_ctx->camera->Position.z - 16) / Constants::CHUNK_SIZE));
	if (newX != g_ctx->posX || newY != g_ctx->posY) {

		//fprintf(stdout, "%d %d \n", newX, newY);
		g_ctx->map->playerPositionCord(newX, newY);
		g_ctx->map->addChunk(-1);
		g_ctx->map->printChunks();
		g_ctx->posX = newX;
		g_ctx->posY = newY;
		updateFlag = true;
	}
	if (updateFlag == true) {
		g_ctx->map->updateMap(0, 0);
		g_ctx->blockCords = g_ctx->map->getBlockCordinates();
	}


	// update current position of player hitbox
	// Handles camera inputs
	// Collision self contained with player class.
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);
	view = g_ctx->camera->getView();
	projection = g_ctx->camera->getProjection(90.0f, 0.1f, 10.0f);
	// verticies of visable blocks
	g_ctx->playerVerts = g_ctx->map->getPlayerChunk(g_ctx->camera->Position, g_ctx->camera->extractFrustumPlanes(projection * view));

	// controls
	g_ctx->camera->Inputs(g_ctx->window, timeDiff, g_ctx->blockCords, g_ctx->playerVerts, &g_ctx->updateQue, g_ctx->posX, g_ctx->posY);
	
	g_ctx->camera->updateBoundingBox();
	
	// Updates and exports the camera matrix to the Vertex Shader
	g_ctx->camera->updateMatrix(90.0f, 0.1f, 100.0f);

	// Specify the color of the background
	glClearColor(0.7f, 0.7f, 1.0f, 1.0f);
	// Clean the back buffer and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draws different meshes
	// ok not drawing the block apparently jsut ruins the shader, no block no shader
	g_ctx->light->Draw(*g_ctx->lightShader, *g_ctx->camera);
	g_ctx->map->drawMap(*g_ctx->shaderProgram, *g_ctx->camera);

	// Menu elements
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

#ifdef __EMSCRIPTEN__
	// Virtual cursor handling for UI mode on web
	// Override ImGui mouse position with virtual cursor when in inventory/menu mode
	if (g_ctx->camera->isInUIMode()) {
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2(g_ctx->camera->getVirtualCursorX(), g_ctx->camera->getVirtualCursorY());
		
		// Forward mouse button state to ImGui
		io.MouseDown[0] = glfwGetMouseButton(g_ctx->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		io.MouseDown[1] = glfwGetMouseButton(g_ctx->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
	}
#endif

	// hit esc
	ImGui::SetNextWindowSize(ImVec2(250, 25));
	ImGui::SetNextWindowPos(ImVec2(15, 15));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::Begin("HitEsc", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration);

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
		ImGui::Text("Press ESC for options");
		ImGui::PopStyleColor();

	ImGui::End();
	ImGui::PopStyleColor();

	// show selected item
	ImGui::SetNextWindowSize(ImVec2(200, 55));
	ImGui::SetNextWindowPos(ImVec2(950, 695));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
	ImGui::Begin("SelectedItem", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
		ImGui::Text("Selected Item:");

		if (g_ctx->camera->getSelectedSlot() == -1) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			ImGui::Text("None");
			ImGui::PopStyleColor();
		}
		else {
			ImGui::Text((
				"Block: " + std::to_string(g_ctx->camera->getInventory()[g_ctx->camera->getSelectedSlot()][1]) + " "
				+ "Amount: " + std::to_string(g_ctx->camera->getInventory()[g_ctx->camera->getSelectedSlot()][0]))
				.c_str());
		}

	ImGui::End();
	ImGui::PopStyleColor();

	//inventory
	if (g_ctx->camera->isInventoryOpen()) {
		ImGui::SetNextWindowSize(ImVec2(600, 270));
		ImGui::SetNextWindowPos(ImVec2(300, 330));

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::Begin("Inventory", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

			ImGui::PushFont(g_ctx->fontAtlas->Fonts[1]);
			ImGui::Text("Inventory");
			ImGui::PopFont();

			ImGui::Columns(2, nullptr, false);

			std::array <std::array<int, 2>, 20> inventoryArray = g_ctx->camera->getInventory(); 

			for (int i = 0; i < inventoryArray.size(); i++) {


				// Render selectable or empty text
				if (inventoryArray[i][1] == -1) {
					ImGui::Text("Empty Slot");
				}
				else {
					// %TODO Selectable not reacting to clicks sometimes.
					if (ImGui::Selectable((
						std::to_string(i + 1) + " "
						"Block: " + std::to_string(inventoryArray[i][1]) + " "
						+ "Amount: " + std::to_string(inventoryArray[i][0]))
						.c_str(), (g_ctx->camera->getSelectedSlot() == i))) 
						{

							g_ctx->camera->setSelectedSlot(i); // set selected state if clicked
							//printf("Item %d clicked!\n", camera.getSelectedSlot());
						}
				}
				ImGui::NextColumn();
			}

		ImGui::End();
		ImGui::PopStyleColor();
	}

	//show menu
	if (g_ctx->camera->isMenuOpen()) {
		ImGuiIO& io = ImGui::GetIO();
		
		// Draw a black translucent overlay
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGui::SetNextWindowBgAlpha(0.7f);
		ImGui::Begin("Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
		ImGui::End();

		ImVec2 windowSize = ImVec2(200, 100);
		ImGui::SetNextWindowSize(windowSize);
		ImGui::SetNextWindowPos(ImVec2(500, 250));
		ImGui::Begin("Menu", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

			// Calculate position for centering button
			ImVec2 buttonSize(150, 50); // button size
			ImVec2 buttonPosition = ImVec2((windowSize.x - buttonSize.x) * 0.5f, (windowSize.y - buttonSize.y) * 0.5f); // Find center
			// Set cursor position to center the button
			ImGui::SetCursorPos(buttonPosition);

			if (ImGui::Button("Exit Game", buttonSize)) {
				// Cancel main loop and exit
				#ifdef __EMSCRIPTEN__
				emscripten_cancel_main_loop();
				#else
				glfwSetWindowShouldClose(g_ctx->window, true);
				#endif
			}
	
		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(325, 375));
		ImGui::SetNextWindowSize(ImVec2(550, 290));
		ImGui::Begin("Instructions", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
			
			ImGui::PushFont(g_ctx->fontAtlas->Fonts[2]);
			ImVec2 windowSize2 = ImVec2(550, 290);
			ImVec2 textSize = ImGui::CalcTextSize("Controls");
			float textWidth = textSize.x;
			float windowWidth = windowSize2.x;
			float paddingX = (windowWidth - textWidth) * 0.5f;
			ImGui::SetCursorPosX(paddingX);
			ImGui::Text("Controls");
			ImGui::PopFont();
			ImGui::Text("\nESC to open menu");
			ImGui::Text("\nW, A, S, D to move forwards, left, backwards, right");
			ImGui::Text("\nSpace to jump");
			ImGui::Text("\nTAB to open inventory");
			ImGui::Text("\nLeft click to break blocks and collect blocks \ninto your inventory");
			ImGui::Text("\nIf a block in your inventory is selected, \nright click to place selected block");
			
		ImGui::End();

	}

	// player cursor
	if (!g_ctx->camera->isMenuOpen() && !g_ctx->camera->isInventoryOpen()) {
		ImGui::SetNextWindowPos(ImVec2(597, 397));
		ImGui::SetNextWindowSize(ImVec2(6, 6));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		ImGui::Begin("crosshair", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
		ImGui::End();
		ImGui::PopStyleColor();
	}	

#ifdef __EMSCRIPTEN__
	// Draw virtual cursor when in UI mode (inventory/menu) on web
	if (g_ctx->camera->isInUIMode()) {
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		float cx = g_ctx->camera->getVirtualCursorX();
		float cy = g_ctx->camera->getVirtualCursorY();
		
		// Draw a simple arrow cursor
		// Main triangle (arrow shape)
		ImVec2 p1(cx, cy);                      // Tip
		ImVec2 p2(cx, cy + 20);                 // Bottom left
		ImVec2 p3(cx + 14, cy + 14);            // Bottom right
		
		// Draw filled triangle with outline
		drawList->AddTriangleFilled(p1, p2, p3, IM_COL32(255, 255, 255, 255));
		drawList->AddTriangle(p1, p2, p3, IM_COL32(0, 0, 0, 255), 2.0f);
	}
#endif

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Swap the back buffer with the front buffer
	glfwSwapBuffers(g_ctx->window);
	// Take care of all GLFW events
	glfwPollEvents();

	// Check if window should close (for desktop builds)
	#ifndef __EMSCRIPTEN__
	if (glfwWindowShouldClose(g_ctx->window)) {
		// Desktop: Just return, the while loop will exit
		return;
	}
	#else
	// Web: Check if window should close and cancel main loop
	if (glfwWindowShouldClose(g_ctx->window)) {
		emscripten_cancel_main_loop();
	}
	#endif
}

int main()
{
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.3 for desktop, WebGL 2.0 for web
	#ifdef __EMSCRIPTEN__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	#endif
	
	//make window unresizable
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// Create a GLFWwindow object of 800 by 800 pixels, naming it "MyCraft"
	GLFWwindow* window = glfwCreateWindow(width, height, "MyCraft", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		glfwTerminate();
		return -1;
	}
	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	// Load GLAD so it configures OpenGL 
	// WE ARE CONVERTING MyCraft to a BROWSER game with Emscripten.
	// Emscripten automatically initalizes WebGL so, uncomment gladLoadGL if compiling otherwise
	#ifndef __EMSCRIPTEN__
		gladLoadGL();
	#endif

	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, width, height);



	// Temporary Texture data
	// Want to move this into map in future.
	/**/
	Texture textures[]
	{
		Texture(("textures/grass.png"), "diffuse", 0, GL_RGBA, GL_UNSIGNED_BYTE),
		Texture(("textures/grass.png"), "specular", 1, GL_RED, GL_UNSIGNED_BYTE)
	};

	Texture texturesBrick[]
	{
		Texture(("textures/brick.png"), "diffuse", 0, GL_RGBA, GL_UNSIGNED_BYTE),
		Texture(("textures/brick.png"), "specular", 1, GL_RED, GL_UNSIGNED_BYTE)
	};


	// Generates Shader object using shaders default.vert and default.frag
	// Load platform-specific shaders: WebGL 2.0 (ES 3.0) for web, OpenGL 3.3 Core for desktop
	#ifdef __EMSCRIPTEN__
	Shader shaderProgram("shaders/web/default.vert", "shaders/web/default.frag");
	#else
	Shader shaderProgram("shaders/desktop/default.vert", "shaders/desktop/default.frag");
	#endif

	// Store mesh data in vectors for the mesh
	std::vector <Texture> tex(textures, textures + sizeof(textures) / sizeof(Texture));

	// Shader for light cube
	#ifdef __EMSCRIPTEN__
	Shader lightShader("shaders/web/light.vert", "shaders/web/light.frag");
	#else
	Shader lightShader("shaders/desktop/light.vert", "shaders/desktop/light.frag");
	#endif

	// Store mesh data in vectors for the mesh
	std::vector <Vertex> lightVerts(lightVertices, lightVertices + sizeof(lightVertices) / sizeof(Vertex));
	std::vector <GLuint> lightInd(lightIndices, lightIndices + sizeof(lightIndices) / sizeof(GLuint));
	// Create light mesh
	Mesh light(lightVerts, lightInd, tex);

	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec3 lightPos = glm::vec3(2.0f, 15.0f, 2.0f);
	glm::mat4 lightModel = glm::mat4(1.0f);
	lightModel = glm::translate(lightModel, lightPos);

	glm::vec3 objectPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 objectModel = glm::mat4(1.0f);
	objectModel = glm::translate(objectModel, objectPos);
	

	
	lightShader.Activate();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lightModel));
	glUniform4f(glGetUniformLocation(lightShader.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	shaderProgram.Activate();
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(objectModel));
	glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	

	// ----------------------------- Menu (ImGui) -----------------------------------------------------------------------------------------------------
	#ifdef __EMSCRIPTEN__
	const char* glsl_version = "#version 300 es";
	#else
	const char* glsl_version = "#version 130";
	#endif
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Button] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Set button color to grey
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Set hover color to light grey

	// add scaled fonts
	ImFontAtlas* fontAtlas = io.Fonts;
	fontAtlas->AddFontFromFileTTF("libraries/include/mygui/misc/fonts/ProggyTiny.ttf", 16.0f);
	fontAtlas->AddFontFromFileTTF("libraries/include/mygui/misc/fonts/ProggyTiny.ttf", 40.0f);
	fontAtlas->AddFontFromFileTTF("libraries/include/mygui/misc/fonts/ProggyTiny.ttf", 24.0f);

	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(1, 1);


	// ----------------------------- Game Logic -----------------------------------------------------------------------------------------------

	// Enables the Depth Buffer
	glEnable(GL_DEPTH_TEST);

	// Initalize map
	Map* map = new Map(1);
	map->addChunk(0);
	map->loadMap();

	// Creates camera object
	// x z y
	Player camera(width, height, glm::vec3(10.0f, 50.0f, 10.0f));

	// Gets block coordinates for detection logic
	std::vector<glm::vec3> blockCords = map->getBlockCordinates();

	// Initialize global context for main loop
	g_ctx = new GameContext();
	g_ctx->window = window;
	g_ctx->shaderProgram = &shaderProgram;
	g_ctx->lightShader = &lightShader;
	g_ctx->light = &light;
	g_ctx->map = map;
	g_ctx->camera = &camera;
	g_ctx->prevTime = 0.0;
	g_ctx->counter = 0;
	g_ctx->posX = 0;
	g_ctx->posY = 0;
	g_ctx->blockCords = blockCords;
	g_ctx->fontAtlas = fontAtlas;

	// Game loop
	#ifdef __EMSCRIPTEN__
	// Web: Use Emscripten main loop (non-blocking, returns control to browser)
	// 0 = use requestAnimationFrame for timing (recommended)
	// 1 = simulate infinite loop (throws exception to prevent code after from running)
		emscripten_set_main_loop(main_loop_iteration, 0, 1);
	#else
	// Desktop: Traditional blocking while loop
	while (!glfwWindowShouldClose(window))
	{
		main_loop_iteration();
	}
	#endif

	// Cleanup (only reached on desktop, web cleanup happens when page closes)
	// Delete all the objects we've created
	shaderProgram.Delete();
	lightShader.Delete();

	//cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	
	delete map;
	delete g_ctx;
	
	return 0;
}