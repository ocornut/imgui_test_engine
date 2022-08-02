# Dear ImGui Test Engine + Test Suite

## Contents

- [imgui_test_engine/](https://github.com/ocornut/imgui_test_engine/tree/main/imgui_test_engine): dear imgui test engine / automation system (library)
- [imgui_tests/](https://github.com/ocornut/imgui_test_engine/tree/main/imgui_tests): dear imgui test suite (application)
- [app_minimal/](https://github.com/ocornut/imgui_test_engine/tree/main/app_minimal): minimal demo app showcasing how to integrate the test engine (app)
- [shared/](https://github.com/ocornut/imgui_test_engine/tree/main/shared): shared C++ helpers for apps

## Don't blink

https://user-images.githubusercontent.com/8225057/182409619-cd3bf990-b383-4a6c-a6ba-c5afe7557d6c.mp4

## Overview

- Designed to **automate and test Dear ImGui applications**. 
- We also use it to **self-test Dear ImGui itself**, reduce regression and facilitate contributions.
- **Test Engine interacts mostly from the point of view of an end-user, by injecting mouse/keyboard** inputs into Dear ImGui's IO. It means it tries to "find its way" toward accomplishing an action. Opening an item may mean CTRL+Tabbing into a given widow, moving things out of the way, scrolling to locate the item, querying its open status, etc.
- It can be used for a variety of testing (smoke testing, integration/functional testing) or automation purposes (running tasks, capturing videos, etc.).
- It **can run in your windowed application**. **It can also run headless** (e.g. running GUI tests from a console or on a CI server without rendering).
- It **can run at simulated human speed** (for watching or exporting videos) or **can run in fast mode** (e.g. teleporting mouse).
- It **can export screenshots and videos/gifs**. They can be leveraged for some forms of testing, but also to generate assets for documentation, or notify teams of certain changes. Assets that often need to be updated are best generated from a script inside of manually recreated/cropped/exported.
- You can use it to program high-level commands e.g. `MenuCheck("Edit/Options/Enable Grid")` or run more programmatic queries ("list openable items in that section, then open them all"). So from your POV it could be used for simple smoke testing ("open all our tools") or for more elaborate testing ("interact with xxx and xxx, check result").
- It **can be used as a form of "live tutorial / demo"** where a script can run on an actual user application to showcase features.
- It includes a performance tool and viewer which we used to record/compare performances between builds and branches (optional, requires ImPlot).
- It is currently a C++ API but down the line we can expect that the commands will be better standardized, stored in data files, called from other languages.
- It has been in use and development since 2018. Library is provided as-is, and we'll provide best effort to make it suitable for user needs.

## Docs / Wiki

See [imgui_test_engine Wiki](https://github.com/ocornut/imgui_test_engine/wiki/imgui_test_engine).

## Licenses

- The [imgui_test_engine/](https://github.com/ocornut/imgui_test_engine/tree/main/imgui_test_engine) folder is under the [Dear ImGui Test Engine License](https://github.com/ocornut/imgui_test_engine/blob/main/imgui_test_engine/LICENSE.txt).<BR>TL;DR: free for individuals, educational, open-source and small businesses uses. Paid for larger businesses. Read license for details. License sales to larger businesses are used to fund and sustain the development of Dear ImGui.
- Other folders are all under the [MIT License](https://github.com/ocornut/imgui_test_engine/blob/main/imgui_tests/LICENSE.txt).
