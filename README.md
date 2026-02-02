# CmdBar - Simple Console Progress Bar

CmdBar is a header-only utility that provides a lightweight progress bar for small console tools and utilities. It is designed to be easy to drop into any project where you want a clear visual indication of progress and a simple timing summary when the work is complete.

### Compatibility and Dependencies
- C++17 Standard and above
- Standard Template Library (STL)
- Uses platform console APIs for cursor control and optional coloring

### OS Support
- Windows
- Linux
- macOS

# Usage

Copy `CmdBar.h` to your project and include the file.

The progress bar is controlled by three simple calls:

- `bar::start(text, total)` - starts a new progress bar
- `bar::step()` progress

# Example code - simple

```cpp
#include "CmdBar.h"

int main()
{
    constexpr size_t total = 200;

    bar::start("Processing", total);

    for (size_t i = 0; i < total; ++i)
    {
        Sleep(25);  // milliseconds (Windows demo)
        bar::step();
    }

    return 0;
}
```

Output while running:

```dos
Processing               [=====================>              ] 42%
```

When finished, CmdBar prints 100% and a timing summary:

```dos
Processing               [==================================================] 100%  ->  3 seconds
```

### Tip
CmdBar stores its progress state as a single global (header-only) instance.  
This means `bar::step()` can be called from anywhere in your codebase (even deep inside your project), as long as you have called `bar::start(text, total)` up front and the total number of steps is known.

Typical usage is to calculate or estimate `total` before starting, and then call `bar::step()` whenever one unit of work is completed.

# Example code - multiple processes

```cpp
#include "CmdBar.h"

void process(const std::string& name, size_t total)
{
	bar::start(name, total);
	for (size_t i = 0; i < total; ++i)
	{
		Sleep(10);
		bar::step();
	}
	bar::stop();
}

int main()
{
	process("Process A", 100);
	process("Process B", 200);
	process("Process C", 300);

	return 0;
}
```

### Processing

<img width="999" height="186" alt="image" src="https://github.com/user-attachments/assets/e2136c9f-dbff-465a-8b21-430c5a5b54b6" />

### Done
<img width="993" height="122" alt="image" src="https://github.com/user-attachments/assets/b2d10a33-fa7c-422c-873b-0fe55650745e" />


## License
This software is released under the MIT License terms.







