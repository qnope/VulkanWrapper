# SKILL: Write a Complete Architecture Document with Subdirectories

## Overview

This skill enables you to write a comprehensive architecture document for a software project, organizing content into logical sections and subdirectories for clarity and maintainability.

Always launch several subagents in parallel to write the different sections of the architecture document. Each subagent can focus on a specific module or component, allowing for efficient and detailed documentation.

## Steps

1. **Define the Top-Level Structures**
  - Create a root directory for the architecture documentation (e.g., `specs/architecture/<library_name>`).
  - Inside, create a main `README.md` that provides an overview and links to subtopics.
  - This file will serve as the entry point for readers to navigate the architecture documentation.
  - It will contain an explanation of the different layer and how they interact together.
  - It is expected that tests architecture are put inside another directory, like `specs/architecture/tests/README.md` for general information and `specs/architecture/tests/<module>/README.md` for module-specific tests. The same goes for performance architecture, which should be in `specs/architecture/performance/README.md` and `specs/architecture/performance/<module>/README.md`.
  - It is expected that example architecture are put inside another directory, like `specs/architecture/examples/README.md` for general information and `specs/architecture/examples/<example_name>/README.md` for specific examples.
  - It is expected that if there are several libraries, they should be put in different subdirectories, like `specs/architecture/library1/README.md` and `specs/architecture/library2/README.md`.

2. **Define bottom-level sections**
  - For each subdirectory where there is code, create a 'README.md' and place it in `specs/architecture/<library_name>/<module_name>/`.
  - This README will explain how to use the component, and where do you need to use it.
  - You can expect to have a big depth of subdirectories, for example `specs/architecture/<library_name>/<module_name>/<submodule>/`.

## What the README file contain

1. **Overview**: A brief description of the component or module, its purpose, and its role within the overall architecture.
2. **How to use**: A detailed explanation of how to use the component, including any necessary setup, configuration, or dependencies.
3. **Diagrams**: If applicable, include diagrams to illustrate the architecture, such as component diagrams, sequence diagrams, or flowcharts.
4. **Conciseness**: The README should be concise and focused on the most important information. Avoid unnecessary details or tangential topics that may distract from the main points. 50 lines is a good target for the length of the README file, but it can be longer if necessary to cover all relevant information. Never go above 100 lines. It's better to write:
```markdown
to configure: `python configure.py --option1 value1 --option2 value2`
```
instead of:
```markdown
To configure the project, you can run the following command in your terminal:
```bash
python configure.py --option1 value1 --option2 value2
```
```
