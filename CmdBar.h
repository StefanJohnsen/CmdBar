#pragma once
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <cstdio>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#else
#include <unistd.h>
#endif

/*----------------------------------------------------------------
  CmdBar.h

  Header-only progress bar for simple console tools.
  Requirements: C++17
  OS Support: Windows/Linux/MacOS

  License: MIT
  Author: Stefan Falk Johnsen
  Copyright (c) 2025 FalconCoding

  GitHub: https://github.com/StefanJohnsen/CmdBar
----------------------------------------------------------------*/

namespace bar
{
	// Progress bar formatting constants (compile-time).
	//bar_width  : width of the visual bar in characters (e.g. "[====>       ]")
	//text_width : left label field width (std::setw) before the bar is printed

	constexpr size_t bar_width = 50;
	constexpr size_t text_width = 35;

	using namespace std::chrono;

	using Clock = steady_clock;

	inline bool progress_idle = false;
	inline bool progress_stop = false;
	inline std::string progress_text;
	inline size_t progress_total = 0;
	inline size_t progress_step = 0;
	inline size_t progress_current = 0;
	inline time_point<Clock> progress_time;

#ifdef _WIN32

	struct ConsoleAttr
	{
		WORD old_attr = 0;
	};

	inline bool SetConsoleTextAttributeBlue(ConsoleAttr& attr)
	{
		const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		if (console == INVALID_HANDLE_VALUE) return false;

		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (!GetConsoleScreenBufferInfo(console, &csbi)) return false;

		attr.old_attr = csbi.wAttributes;

		constexpr WORD fg_mask = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

		const WORD new_attr = static_cast<WORD>((attr.old_attr & ~fg_mask) | (FOREGROUND_BLUE | FOREGROUND_INTENSITY));

		return SetConsoleTextAttribute(console, new_attr) != 0;
	}

	inline void RestoreConsoleTextAttribute(const bool colored, const ConsoleAttr& attr)
	{
		if (!colored) return;

		const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		if (console == INVALID_HANDLE_VALUE) return;

		SetConsoleTextAttribute(console, attr.old_attr);
	}

	inline void hide_cursor()
	{
		if (progress_idle) return;

		const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		if (console == INVALID_HANDLE_VALUE) return;

		CONSOLE_CURSOR_INFO info;
		if (!GetConsoleCursorInfo(console, &info)) return;

		info.bVisible = false;
		SetConsoleCursorInfo(console, &info);
	}

	inline void show_cursor()
	{
		if (progress_idle) return;

		const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		if (console == INVALID_HANDLE_VALUE) return;

		CONSOLE_CURSOR_INFO info;
		if (!GetConsoleCursorInfo(console, &info)) return;

		info.bVisible = true;
		SetConsoleCursorInfo(console, &info);
	}

#else

	struct ConsoleAttr {};

	inline bool SetConsoleTextAttributeBlue(ConsoleAttr& /*attr*/)
	{
		if (!isatty(fileno(stdout))) return false;
		std::cout << "\x1b[94m";
		return true;
	}

	inline void RestoreConsoleTextAttribute(const bool colored, const ConsoleAttr& /*attr*/)
	{
		if (!colored) return;
		std::cout << "\x1b[0m";
	}

	inline void hide_cursor()
	{
		if (progress_idle) return;
		if (!isatty(fileno(stdout))) return;

		std::cout << "\x1b[?25l";
		std::cout.flush();
	}

	inline void show_cursor()
	{
		if (progress_idle) return;
		if (!isatty(fileno(stdout))) return;

		std::cout << "\x1b[?25h";
		std::cout.flush();
	}

#endif

	inline bool isIdle()
	{
		return progress_idle;
	}

	inline void idle(bool set = true)
	{
		if (set) show_cursor();
		progress_idle = set;
	}

	inline void clear()
	{
		progress_stop = false;
		progress_text.clear();
		progress_total = 0;
		progress_step = 0;
		progress_current = 0;
	}

	inline void set_text(const std::string& text)
	{
		progress_text = text;

		if (progress_text.size() > text_width)
		{
			if (text_width >= 3)
				progress_text = progress_text.substr(0, text_width - 3) + "...";
			else
				progress_text = progress_text.substr(0, text_width);
		}
	}
	inline void print(size_t complete)
	{
		if (progress_idle) return;
		if (progress_stop) return;

		const auto pos = static_cast<size_t>(std::round(static_cast<double>(bar_width) * (static_cast<double>(complete) / 100.0)));

		std::cout << "\r" << std::left << std::setw(text_width) << progress_text << " [";

		ConsoleAttr attr;
		const bool colored = SetConsoleTextAttributeBlue(attr);

		for (size_t i = 0; i < bar_width; ++i)
		{
			if (i < pos)
				std::cout << "=";
			else if (i == pos)
				std::cout << ">";
			else
				std::cout << " ";
		}

		RestoreConsoleTextAttribute(colored, attr);

		std::cout << "] " << complete << "%";
		std::cout.flush();
	}

	inline void stop();

	inline void step(const size_t step)
	{
		if (progress_idle) return;
		if (progress_stop) return;

		if (progress_total == 0)
			throw std::runtime_error("Progress total is zero. Call start() first.");

		if (step > progress_total) return;

		if (step == progress_total)
		{
			stop();
			return;
		}

		const auto temp = (step * 100 + progress_total / 2) / progress_total;

		if (temp == progress_current)
			return;

		progress_current = temp;

		if (progress_current == 100)
			stop();
		else
			print(progress_current);
	}

	inline void step()
	{
		if (progress_idle) return;
		if (progress_stop) return;

		step(++progress_step);
	}

	inline void start(const std::string& text, const size_t total)
	{
		if (total == 0)
			throw std::runtime_error("Progress total is zero.");

		clear();

		hide_cursor();

		set_text(text);
		
		progress_total = total;
		progress_time = Clock::now();

		print(0);
	}

	inline std::string stop(const time_point<Clock>& start)
	{
		const auto duration = duration_cast<std::chrono::microseconds>(Clock::now() - start);
		const auto hours = duration_cast<std::chrono::hours>(duration);
		const auto minutes = duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
		const auto seconds = duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1));
		const auto milliseconds = duration_cast<std::chrono::milliseconds>(duration % std::chrono::seconds(1));
		const auto microseconds = duration_cast<std::chrono::microseconds>(duration % std::chrono::milliseconds(1));

		std::string result;

		if (hours.count() > 0 || minutes.count() > 0)
		{
			result += (hours.count() < 10 ? "0" : "") + std::to_string(hours.count()) + ":";
			result += (minutes.count() < 10 ? "0" : "") + std::to_string(minutes.count()) + ":";
			result += (seconds.count() < 10 ? "0" : "") + std::to_string(seconds.count());
		}
		else if (seconds.count() > 0)
			result += std::to_string(seconds.count()) + " seconds";
		else if (milliseconds.count() > 0)
			result += std::to_string(milliseconds.count()) + " milliseconds";
		else
			result += std::to_string(microseconds.count()) + " microseconds";

		return result;
	}

	inline void stop()
	{
		if (progress_idle) return;
		if (progress_stop) return;

		print(100);

		std::cout << "  ->  " << stop(progress_time) << std::endl;

		clear();

		show_cursor();

		progress_stop = true;
	}
}
