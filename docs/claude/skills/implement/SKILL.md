# Implement

Implement $ARGUMENTS.

## Workflow

1. Create a plan, interracts with the user to define it. To do so you can check the /dev skill.
2. When user accepts the plan, launch an agent team with 4 agents.
  1. One tester that will create the tests and implement them. It will use /dev and /test skills. It can compile the tests if needed. He will communicate with developper to say if tests builds, or not and pass or not pass.
  2. One developper using /dev skill. It will the compile the project and examples and run them.
  3. One reviewer using /review skill. He will review the code.
  4. One QA, the QA will launch the example Advanced/Main to be sure that the rendered scene in screenshot.png corresponds the demand from the user. This one can wait that the other agents (mainly the reviewer) are done to begin to work.
3. When QA says that the work is finished, it means the feature has been implemented.
4. You can commit using /commit skill.

I expect developper and tester to run in parralel. Reviewer can also be run in parralel to catch mistake early.

