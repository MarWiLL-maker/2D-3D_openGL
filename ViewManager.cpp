///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
// Marrius Williams Student
//
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Declaration of global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// Camera object used for viewing and interacting with the 3D scene
	Camera* g_pCamera = nullptr;

	// Mouse movement position tracking variables
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// Per-frame delta timing variables to ensure smooth, frame-rate-independent navigation
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// Projection toggle flag: false = Perspective (3D), true = Orthographic (2D top-down)
	bool bOrthographicProjection = false;
}

/***********************************************************
 * ViewManager()
 *
 * The constructor initializes pointers and instantiates the
 * 3D Camera with default positional and orientation parameters.
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
	// Initialize member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = nullptr;
	g_pCamera = new Camera();

	// Default camera view parameters situated to view the scene objects
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80.0f;
}

/***********************************************************
 * ~ViewManager()
 *
 * Destructor cleans up dynamically allocated memory for the
 * camera object and clears pointers.
 ***********************************************************/
ViewManager::~ViewManager()
{
	m_pShaderManager = nullptr;
	m_pWindow = nullptr;
	if (g_pCamera != nullptr)
	{
		delete g_pCamera;
		g_pCamera = nullptr;
	}
}

/***********************************************************
 * CreateDisplayWindow()
 *
 * Creates the GLFW display window, captures the mouse cursor,
 * enables alpha blending, and registers GLFW callbacks for
 * mouse movement and scroll wheel input.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// Try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		nullptr, nullptr);

	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	// Capture and hide the mouse cursor inside the application window
	// for continuous 3D mouse look without leaving window bounds
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Register cursor position callback for pitch and yaw orientation
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// Register scroll wheel callback to dynamically adjust camera movement speed
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// Enable blending for supporting transparent texture rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;
	return window;
}

/***********************************************************
 * Mouse_Position_Callback()
 *
 * Automatically triggered by GLFW whenever the mouse is moved.
 * Computes offset deltas between frames to orient the camera's
 * pitch and yaw angles.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// Prevent an initial camera jump when mouse focus is first acquired
	if (gFirstMouse)
	{
		gLastX = static_cast<float>(xMousePos);
		gLastY = static_cast<float>(yMousePos);
		gFirstMouse = false;
	}

	// Calculate mouse coordinate offsets between the previous and current frame
	// yOffset is inverted (lastY - yMousePos) because OpenGL coordinate system
	// places Y = 0 at the bottom, while window screen space places Y = 0 at the top
	float xOffset = static_cast<float>(xMousePos) - gLastX;
	float yOffset = gLastY - static_cast<float>(yMousePos);

	// Cache current mouse positions for the next frame's comparison
	gLastX = static_cast<float>(xMousePos);
	gLastY = static_cast<float>(yMousePos);

	// Send offsets to Camera class to recalculate direction vectors
	if (g_pCamera != nullptr)
	{
		g_pCamera->ProcessMouseMovement(xOffset, yOffset);
	}
}

/***********************************************************
 * Mouse_Scroll_Callback()
 *
 * Automatically triggered by GLFW when the mouse wheel is rotated.
 * Adjusts camera travel/movement speed dynamically.
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	if (g_pCamera == nullptr)
	{
		return;
	}

	// Modify camera travel speed based on vertical wheel scroll delta
	g_pCamera->MovementSpeed += static_cast<float>(yOffset) * 0.5f;

	// Clamp speed boundaries to prevent negative/inverted motion or excessive speed
	if (g_pCamera->MovementSpeed < 0.1f)
	{
		g_pCamera->MovementSpeed = 0.1f;
	}
	if (g_pCamera->MovementSpeed > 20.0f)
	{
		g_pCamera->MovementSpeed = 20.0f;
	}
}

/***********************************************************
 * ProcessKeyboardEvents()
 *
 * Polls active keyboard keys each frame:
 * - ESC: Closes application
 * - W / S: Moves forward and backward along line of sight (zoom in/out)
 * - A / D: Pans / strafes left and right
 * - Q / E: Moves camera downward and upward along the world Y-axis
 * - P / O: Toggles between Perspective (3D) and Orthographic (2D)
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// Close window if escape key is pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	if (g_pCamera == nullptr)
	{
		return;
	}

	// Forward (Zoom in) and Backward (Zoom out) along camera line of sight
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// Pan Left and Pan Right along camera orthogonal right vector
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// Vertical Upward (E) and Downward (Q) navigation along world Y-axis
	float verticalVelocity = g_pCamera->MovementSpeed * gDeltaTime;
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->Position += glm::vec3(0.0f, 1.0f, 0.0f) * verticalVelocity;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->Position -= glm::vec3(0.0f, 1.0f, 0.0f) * verticalVelocity;
	}

	// Perspective projection view with 'P' key
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		bOrthographicProjection = false;
	}

	// Orthographic projection view with 'O' key
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		bOrthographicProjection = true;
	}
}

/***********************************************************
 * PrepareSceneView()
 *
 * Prepares the 3D scene viewing pipeline each frame:
 * 1. Calculates delta time for frame-rate independence.
 * 2. Processes keyboard input.
 * 3. Builds View Matrix and Projection Matrix (Perspective vs Ortho).
 * 4. Passes matrices and camera position to the active shader uniforms.
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// Per-frame delta timing calculation
	float currentFrame = static_cast<float>(glfwGetTime());
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// Process keyboard events (WASD, QE, P/O)
	ProcessKeyboardEvents();

	// Configure View and Projection matrices based on active mode
	if (bOrthographicProjection)
	{
		// Calculate aspect ratio to avoid shape distortion
		float aspectRatio = static_cast<GLfloat>(WINDOW_WIDTH) / static_cast<GLfloat>(WINDOW_HEIGHT);
		float orthoScale = 6.0f; // Scale factor for orthographic viewing volume

		// Construct orthographic projection matrix: left, right, bottom, top, near, far
		projection = glm::ortho(
			-orthoScale * aspectRatio,
			orthoScale * aspectRatio,
			-orthoScale,
			orthoScale,
			0.1f,
			100.0f
		);

		// Per rubric: Orthographic mode looks directly down at the object from above,
		// hiding the bottom base plane for a schematic 2D plan view
		glm::vec3 orthoEye = glm::vec3(0.0f, 10.0f, 0.0f);     // Positioned directly above
		glm::vec3 orthoCenter = glm::vec3(0.0f, 0.0f, 0.0f);  // Centered on the scene object
		glm::vec3 orthoUp = glm::vec3(0.0f, 0.0f, -1.0f);     // Up points toward negative Z

		view = glm::lookAt(orthoEye, orthoCenter, orthoUp);
	}
	else
	{
		// Standard 3D perspective projection matrix: FOV, aspect ratio, near plane, far plane
		projection = glm::perspective(
			glm::radians(g_pCamera->Zoom),
			static_cast<GLfloat>(WINDOW_WIDTH) / static_cast<GLfloat>(WINDOW_HEIGHT),
			0.1f,
			100.0f
		);

		// Retrieve view matrix generated from current camera position and front vector
		view = g_pCamera->GetViewMatrix();
	}

	// Update shader uniforms
	if (m_pShaderManager != nullptr)
	{
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);

		// Pass view position for accurate specular light calculations
		if (bOrthographicProjection)
		{
			m_pShaderManager->setVec3Value("viewPosition", glm::vec3(0.0f, 10.0f, 0.0f));
		}
		else
		{
			m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
		}
	}
}
