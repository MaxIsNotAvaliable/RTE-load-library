#include "keyboard.h"
#include <algorithm>

void CKeyboard::IdleInput()
{
	if (!waitForIdleInput) return;

	waitForIdleInput = false;
	UpdateInput();
}

void CKeyboard::UpdateInput()
{
	IdleInput();

#if INPUT_ONLY_APP
	UCHAR pKeyBuffer[256];
	ZeroMemory(pKeyBuffer, sizeof(UCHAR) * 0x100);
	if (!GetKeyboardState(pKeyBuffer))
		return;
#endif

	memcpy(&m_bPressedKeysPrev, &m_bPressedKeys, sizeof(bool) * 0x100);

	for (int i = 1; i < 0x100; i++)
	{
#if INPUT_ONLY_APP
		if (!(pKeyBuffer[i] & 0xF0))
#else
		if (GetAsyncKeyState(i))
#endif
		{
			m_bKeyLock[i] = false;
			m_bPressedKeys[i] = false;
		}
		else
		{
			m_bPressedKeys[i] = !(m_bKeyLock[i]);
		}
	}

	std::vector<unsigned int> shortCut;
	std::vector<unsigned int> shortCutPrev;
	for (int i = 1; i < 0x100; i++)
	{
		if (!m_bPressedKeys[i])
			shortCut.push_back(i);
		if (!m_bPressedKeysPrev[i])
			shortCutPrev.push_back(i);
	}
	m_pressedShortCut = shortCut;
	m_pressedShortCutPrev = shortCutPrev;
}

bool CKeyboard::KeyUp(const unsigned int& key)
{
	return (m_bPressedKeys[key]) && (!m_bPressedKeysPrev[key]);
}

bool CKeyboard::KeyStillUp(const unsigned int& key)
{
	return (m_bPressedKeys[key]) && (m_bPressedKeysPrev[key]);
}

bool CKeyboard::KeyDown(const unsigned int& key)
{
	return (!m_bPressedKeys[key]) && (m_bPressedKeysPrev[key]);
}

bool CKeyboard::KeyStillDown(const unsigned int& key)
{
	return (!m_bPressedKeys[key]) && (!m_bPressedKeysPrev[key]);
}

bool CKeyboard::ShortCutPressed(std::vector<unsigned int> keys)
{
	std::sort(keys.begin(), keys.end());
	bool result = std::includes(m_pressedShortCut.begin(), m_pressedShortCut.end(), keys.begin(), keys.end());
	bool resultPrev = std::includes(m_pressedShortCutPrev.begin(), m_pressedShortCutPrev.end(), keys.begin(), keys.end());

	if (result && result != resultPrev)
		return true;
	
	return false;
}

void CKeyboard::LockKey(const unsigned int& key)
{
	m_bKeyLock[key] = true;
}

void CKeyboard::UnlockKey(const unsigned int& key)
{
	m_bKeyLock[key] = false;
}
