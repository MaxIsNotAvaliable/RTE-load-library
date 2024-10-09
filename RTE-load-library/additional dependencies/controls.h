#pragma once

class Controls
{
private:
	static inline bool isEnabled = true;


public:
	static inline bool IsMenuVisible = true;

	static inline void StopExecution()
	{
		Controls::isEnabled = false;
	}

	static inline bool GetStatus()
	{
		return Controls::isEnabled;
	}
};

