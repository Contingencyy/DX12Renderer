#pragma once

enum class KeyCode : uint32_t
{
	LEFT_MOUSE, MIDDLE_MOUSE, RIGHT_MOUSE,
	W, A, S, D,
	Q, E
};

class InputHandler
{
public:
	static void OnKeyPressed(KeyCode key);
	static void OnKeyReleased(KeyCode key);

	static bool IsKeyPressed(KeyCode key);
	static KeyCode WParamToKeyCode(WPARAM wParam);

	static float GetInputAxis1D(KeyCode up, KeyCode down);
	static glm::vec2 GetInputAxis2D(KeyCode up, KeyCode down, KeyCode left, KeyCode right);

};
