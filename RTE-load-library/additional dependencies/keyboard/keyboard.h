#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>

// INPUT_ONLY_APP - определяет регистрацию клавиш:
//	1 - только в приложении
//	0 - в любом состоянии
#define INPUT_ONLY_APP 0


class CKeyboard
{
protected:
	static inline bool waitForIdleInput = true;
	static void IdleInput();
	
	static inline bool m_bPressedKeys[256]{ 0 };
	static inline bool m_bPressedKeysPrev[256]{ 0 };

	static inline std::vector<unsigned int> m_pressedShortCut;
	static inline std::vector<unsigned int> m_pressedShortCutPrev;;

	static inline bool m_bKeyLock[256]{ 0 };
	static inline float m_KeyDelayTimer{ 0.25f };
	static inline float m_KeyRepeatTimer{ 0.01f };
public:

	static void UpdateInput();

	static bool KeyUp(const unsigned int& key);
	static bool KeyStillUp(const unsigned int& key);
	static bool KeyDown(const unsigned int& key);
	static bool KeyStillDown(const unsigned int& key);

	/// <summary>
	/// Возвращает true в момент когда клавиши из массива keys будут в нажатом состоянии одновременно
	/// </summary>
	/// <param name="keys"></param>
	/// <returns></returns>
	static bool ShortCutPressed(std::vector<unsigned int> keys);

	static void LockKey(const unsigned int& key);
	static void UnlockKey(const unsigned int& key);
};