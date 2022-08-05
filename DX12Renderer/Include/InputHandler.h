#pragma once

enum class KeyCode : uint32_t
{
	LEFT_MOUSE, MIDDLE_MOUSE, RIGHT_MOUSE,
	W, A, S, D,
	SHIFT, CTRL,
	Q, E
};

class InputHandler
{
public:
	static void OnKeyPressed(KeyCode key);
	static void OnKeyReleased(KeyCode key);

	static void OnMouseMoved(glm::vec2 newPosition);

	static bool IsKeyPressed(KeyCode key);
	static KeyCode WParamToKeyCode(WPARAM wParam);

	static float GetInputAxis1D(KeyCode up, KeyCode down);
	static glm::vec2 GetInputAxis2D(KeyCode up, KeyCode down, KeyCode left, KeyCode right);

	static glm::vec2 GetMousePositionAbs();
	static glm::vec2 GetMousePositionRel();

};
