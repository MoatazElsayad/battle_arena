<div align="center">

# 🛡️ Battle Arena (Gladiators)

**A fast-paced 2D pixel-art fighting game built with C++ and Qt 6.**

![C++](https://img.shields.io/badge/C++-17%2B-blue.svg?style=flat&logo=c%2B%2B)
![Qt](https://img.shields.io/badge/Qt-6.x-41CD52.svg?style=flat&logo=qt)
![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C.svg?style=flat&logo=cmake)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg?style=flat)

</div>

---

## 📖 Overview

**Battle Arena** is an action-packed 2D fighting game featuring beautiful pixel art, fluid mechanics, and a robust progression system. Built from the ground up using **C++** and the **Qt 6 Framework**, the project utilizes frame-by-frame animation rendering mapped to a custom combat engine. 

Players can log in, select from a diverse roster of gladiator classes, and enter the arena to fight formidable enemies in 1v1 duels.

## ✨ Key Features

- 🏰 **Dynamic Combat Engine:** Precision-based attacks, attack cooldowns, attack range validation, and active healing mechanics.
- 🎨 **Rich 2D Animations:** Perfectly synced sprite sheet animations (Idle, Dash, Jump, Attack, Hurt, and Death). Features automatic flipping, Z-order layering, and pixel-perfect ground alignment.
- 👤 **Player Profiles & Lobbies:** Secure registration and login. A fully animated character selection lobby with live idle previews.
- 🔥 **Cinematic UX/UI:** Smooth interface featuring floating text (damage/heals/misses), dynamic health HUDs, polished menus, and a perfectly centered, responsive Game Over screen.
- 🎵 **Audio Integration:** Dedicated `SoundManager` for immersive combat hit sounds and background music.
- 🏆 **Leaderboards & Scaling:** Progress tracking and persistent databases (`DatabaseManager`) to record damage dealt, taken, and high scores.

## 👥 Character Roster

### **Heroes**
- 🗡️ **Knight** - A heavily armored warrior dealing balanced damage.
- 🪓 **Demon Slayer** - High burst damage and mobility.
- 🏹 **Huntress** - Ranged/Agility combatant.
- 🧙‍♂️ **Wizard** - Magic caster.
- *Also features: Arcen, Fantasy Warrior, Martial Hero, and Medieval Warrior.*

### **Enemies**
- 🦇 **Flying Demon** - Aerial mob with unpredictable attack patterns.
- 🦎 **Kobold** - Fast, aggressive ground enemy.

---

## 🎮 Controls

| Action | Key Binding | Description |
| :--- | :---: | :--- |
| **Move Left** | `A` / `Left Arrow` | Moves the character to the left. |
| **Move Right** | `D` / `Right Arrow` | Moves the character to the right. |
| **Attack** | `SPACE` / `Click` | Performs the standard combo attack sequence. |
| **Heal** | `H` | Restores +25 HP (Has a cooldown timer). |
| **Pause** | `ESC` / `P` | Pauses the battle and opens the pause menu. |

*(Note: Exact bindings can be adjusted in the Settings Page).*

---

## 🛠️ Prerequisites & Installation

To build and run **Battle Arena**, you will need:
- **C++ 17 or 20** Compiler (GCC, Clang, or MSVC)
- **CMake** (v3.16 or higher)
- **Qt 6 Framework** (Widgets, Core, Gui components)

### Build Instructions (Linux / WSL / macOS)

```bash
# 1. Clone the repository
git clone https://github.com/MoatazElsayad/battle_arena.git
cd battle_arena

# 2. Create a build directory
mkdir build && cd build

# 3. Configure the project with CMake
cmake ..

# 4. Build the executable
cmake --build .

# 5. Run the game
./Gladiators
```

### Build Instructions (Windows - MSVC/MinGW)
You can directly open the `CMakeLists.txt` file in **Visual Studio** or **VS Code**, allow the CMake plugin to configure the environment, and hit the **Build/Run** button.

---

## 🏗️ Architecture Overview

The codebase is heavily modularized to separate UI logic from game states:
- `MainWindow & GamePage`: The core application shells and stacked widgets routing user navigation.
- `BattleWidget`: The central render engine for 2D arena combat utilizing `QPainter`.
- `GameManager`: The singleton/handler keeping track of instances, turns, rules, and outcomes.
- `AnimatedCharacter & AnimationManager`: Parses pixel-art sprite sheets dynamically, controls frame rate extraction, and manages state machines (Idle -> Attack -> Hurt -> Return to Idle).

## 📄 License
This project is proprietary and built for educational and portfolio purposes. 

*Characters and UI assets belong to their respective artists and creators.*
