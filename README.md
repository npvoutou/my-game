# ⚔️ UE5 Action RPG Prototype

This is the repository for a little Unreal Engine game I created during my transition into game development, tracking `Source/`, `Config/`, some `Content/` assets (large ones are excluded), and project files.

The full gameplay video is available in [Youtube][https://youtu.be/wgV2nLxDLBU].

![lil-thumbnail-url]

[lil-thumbnail-url]: https://i.ytimg.com/vi_webp/wgV2nLxDLBU/maxresdefault.webp

## Project Description

The project was designed as a complete mini-game experience, combining gameplay systems and programming concepts learned through Unreal Engine courses, personal study, and hands-on experimentation.

The prototype follows a knight character through a complete gameplay progression loop spanning two levels. In the first level, the player must defeat all enemies to unlock a gateway to the next area, where they ultimately face a boss encounter. Defeating the boss rewards the player with a key used to unlock a final treasure chest and complete the game. Throughout the journey, the player can discover and equip weapons with different damage values, collect health potions and coins, and interact with environmental hazards such as explosive objects that can damage both enemies and the player. Enemy characters feature aggro-based AI behavior, becoming hostile and pursuing the player when they enter a detection area, then disengaging once the player leaves their range. Some enemies can also drop rewards such as coins or health potions based on configurable drop probabilities. To increase replayability, many enemies and pickups are spawned through randomized spawning volumes, creating variation between gameplay sessions.

The prototype also includes gameplay state persistence through save and load systems, allowing players to preserve progression across sessions. Additional features include pause functionality, audio settings for music and sound effects, and a gameplay HUD displaying health, stamina, collected coins, and key progression status. The primary goal of the project was to explore gameplay programming, AI systems, state management, level progression, inheritance, reusable systems, and scalable gameplay architecture in Unreal Engine 5, using C++ as the primary implementation language and limiting Blueprint usage primarily to UI and presentation-related functionality.

## Gameplay Features

* Melee combat system with multiple weapon pickups and varying damage values.
* Enemy AI featuring aggro-based detection, pursuit, combat engagement, and disengagement behaviors.
* Multi-level gameplay progression with enemy-clear objectives, portal-based level transitions, boss encounters, and key-based progression.
* Loot and pickup systems including weapons, health potions, coins, and randomized enemy drops.
* Environmental hazards such as explosive objects that can damage both players and enemies.
* Randomized spawning systems using spawning volumes for enemies and pickups, creating gameplay variation between sessions.
* Save and load functionality preserving significant gameplay state across play sessions.
* Pause menu, settings menu, and audio controls.
* HUD systems displaying player health, stamina, collected coins, inventory status, and progression indicators.
