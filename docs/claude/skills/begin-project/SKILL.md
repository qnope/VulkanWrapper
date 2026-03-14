# Skill: Create Feature Specification

## Overview

This skill guides the user through the process of creating a clear, actionable specification for a new feature "$ARGUMENTS". The name of this project is deduced from what the user said. You can ask the user if the name is correct, and if not, user can provide the correct name.

Specifications to be added are in `specs/projects/<project_name>/SPEC.md` and should follow one of the template below depending on which kind of specification it is.

To ensure good result, the specification should be as detailed as possible. Then you can ask a lot of questions to the user to get more details about the feature, and update the specification accordingly. Questions are always asked in interactive mode using the AskUserQuestion tool - never as plain text. Questions are always asked in interractive mode. You can ask several questions and propose for each of them a set of answer or a custom one.

It's very rare if no question is asked during the specification phase. 
Before writing final spec, always ask the user if there is any detail they want to add.

Don't bother with implementation details, the specification should be focused on what the feature is and how to use it, not how it is implemented. The implementation details will be added later.

## Spefication Template

```markdown
# <Project Name> API Specification

## 1. Feature Overview
- Brief description of the feature and its purpose.
- High-level summary of how it will be implemented.

## 2. API Design
- If it is a public API:
  - Detailed description of the API, including:
    - Function signatures
    - Class definitions
    - Data structures
  - Explanation of how the API will be used by clients.

- If it is a feature for the real user:
  - List of user stories that describe the feature from the perspective of the end-user.
  - Each user story should include acceptance criteria.

## 3. Testing and Validation
- Description of how the feature will be tested.
- Types of tests (unit, integration, etc.) and their scope.
- Any specific tools or frameworks that will be used for testing.
- Criteria for successful implementation and testing.
```
