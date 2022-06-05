#include "Pch.h"
#include "InputHandler.h"

static std::unordered_map<KeyCode, bool> m_KeyStates;

void InputHandler::OnKeyPressed(KeyCode key)
{
	m_KeyStates[key] = true;
}

void InputHandler::OnKeyReleased(KeyCode key)
{
	m_KeyStates[key] = false;
}

bool InputHandler::IsKeyPressed(KeyCode key)
{
	return m_KeyStates[key];
}

KeyCode InputHandler::WParamToKeyCode(WPARAM wParam)
{
	switch (wParam)
	{
	case VK_LBUTTON:
		return KeyCode::LEFT_MOUSE;
	case VK_MBUTTON:
		return KeyCode::MIDDLE_MOUSE;
	case VK_RBUTTON:
		return KeyCode::RIGHT_MOUSE;
	case 0x57:
		return KeyCode::W;
	case 0x41:
		return KeyCode::A;
	case 0x53:
		return KeyCode::S;
	case 0x44:
		return KeyCode::D;
	case 0x51:
		return KeyCode::Q;
	case 0x45:
		return KeyCode::E;
	}
}

float InputHandler::GetInputAxis1D(KeyCode positiveX, KeyCode negativeX)
{
	return static_cast<float>(m_KeyStates[positiveX] - m_KeyStates[negativeX]);
}

glm::vec2 InputHandler::GetInputAxis2D(KeyCode positiveX, KeyCode negativeX, KeyCode positiveY, KeyCode negativeY)
{
	return glm::vec2(GetInputAxis1D(positiveX, negativeX), static_cast<float>(m_KeyStates[positiveY] - m_KeyStates[negativeY]));
}
