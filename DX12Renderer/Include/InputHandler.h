#pragma once

class InputHandler
{
public:
	enum class KeyCode : uint32_t
	{
		KC_LEFT_MOUSE,
		KC_RIGHT_MOUSE
	};

public:
	InputHandler();
	~InputHandler();

	void OnKeyPressed(KeyCode key);
	void OnKeyReleased(KeyCode key);

	bool IsKeyPressed(KeyCode key);

private:
	std::unordered_map<KeyCode, bool> m_KeyStates;

};
