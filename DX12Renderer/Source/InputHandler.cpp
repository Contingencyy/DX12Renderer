#include "Pch.h"
#include "InputHandler.h"

InputHandler::InputHandler()
{
}

InputHandler::~InputHandler()
{
}

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
