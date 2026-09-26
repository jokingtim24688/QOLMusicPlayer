# Progress

## 2026-09-26 — Research & planning (no code yet)
- Repo was empty. Branch: `claude/vibrant-heisenberg-5xg2ip`.
- Researched Neverlose/Orbit/Krypton-style HUDs and how to build one **externally** without anti-cheat risk.
- Full findings and plan: [docs/PLAN.md](docs/PLAN.md).
- Key decisions (proposed, awaiting user confirmation):
  - Separate click-through topmost overlay window (Crosshair X / Discord model), no injection.
  - FPS + 1% low via ETW (PresentMon approach); CPU/GPU via PDH counters. No kernel drivers.
  - Recommended stack: C++20 + D3D11 + DirectComposition + Dear ImGui.
- graphify wiki not built yet: the repo has no code, and indexing the docs needs an LLM API key that this container doesn't have. It will be built once M0 code exists (code indexing needs no key).

## Next
- User answers the open questions in docs/PLAN.md §4.
- Then start M0 (skeleton overlay window).
