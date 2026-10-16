# Multiplayer Hangman Engine (SOLID C++ Architecture)

## 1. Problem Description
Traditional command-line games often tightly couple game rules, word loading, scoring arithmetic, and console I/O (`std::cin`/`std::cout`) inside a single monolithic loop. This makes it impossible to change scoring rules, switch word sources (e.g., from a file to an in-memory array or database), or run automated unit tests without human console interaction.

This project solves that problem by designing an extensible, multiplayer **Hangman Game Engine** in C++ strictly adhering to **SOLID design principles** and the **Strategy / Dependency Injection patterns**. The core orchestrator (`HangmanGame`) depends strictly on abstract interfaces (`IWordProvider`, `IScoringPolicy`, and `IUserInterface`), allowing components to be swapped or extended at runtime without modifying existing engine logic.

---

## 2. Implemented Features & SOLID Architecture

### Core Gameplay Features
* **Multiplayer Turn-Based Loop:** Supports $N$ players taking turns guessing letters on a shared secret word, tracking individual lives (default: 6) and persistent scores across multiple rounds.
* **Input Sanitization & Validation:** Automatically cleans word bank entries (filtering non-alphabetic characters, uppercasing, and requiring a minimum length of 3) and validates player character inputs.
* **Duplicate Guess Protection:** Tracks all 26 letters of the alphabet per round; duplicate guesses notify the player and skip the turn without deducting lives or points.
* **Dynamic Elimination & Leaderboard:** Eliminates players whose lives reach zero while allowing remaining players to continue; displays a sorted leaderboard at the end of every round.

### Architectural Abstractions
| Interface | Responsibility | Default Implementation | SOLID Principle Applied |
| :--- | :--- | :--- | :--- |
| `IWordProvider` | Supplies random secret words to the engine. | `FileWordProvider` (loads & sanitizes words from a text file using `std::mt19937`). | **SRP, OCP, DIP:** Decouples file I/O from game state. |
| `IScoringPolicy` | Defines point rewards, penalties, and completion bonuses. | `StandardScoringPolicy` (+10/match, -2/miss, +50 completion bonus). | **SRP, OCP, LSP:** Allows new scoring rules without altering `HangmanGame`. |
| `IUserInterface` | Handles all player prompts and game state notifications. | `ConsoleUI` (interactive CLI via `std::cin` / `std::cout`). | **SRP, ISP, DIP:** Enables headless automated testing and alternative UIs. |
| `HangmanGame` | Orchestrates round setup, turn rotation, and win/loss evaluation. | Concrete Engine (injected with `std::shared_ptr` abstractions). | **DIP, OCP:** Closed for modification, open for extension. |

---

## 3. Build and Run Instructions

### Prerequisites
* A C++17 compatible compiler (`g++`, `clang++`, or MSVC).
* A text file named `words.txt` in the working directory containing at least one word (3+ letters) per line.

### Creating a Sample `words.txt`
```bash
cat << 'EOF' > words.txt
POLYMORPHISM
ENCAPSULATION
ABSTRACTION
INHERITANCE
INTERFACE
EOF


## Output :
!(./output.png)
