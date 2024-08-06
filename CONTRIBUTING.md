# Contributing Guidelines

[[_TOC_]]

# Basic guidelines

All changes you commit or submit by merge request should follow these simple guidelines:
* Use merge requests.
* Should build without new warnings or errors. Please use the project's solution file to drive the build and test process.
* Please do not squash your Merge Request commits into one commit. Split PR into many meaningful commits, we can review separately.
* Code should be free from profanities in any language.

# Merge requests

* Your merge request title should contain WindowsPerf [JIRA](https://linaro.atlassian.net/jira/software/c/projects/WPERF/boards/169) ticket, which is prefixed `WPERF-`. Note: our GitLab JIRA integration requires developers to add it.
  * You can post your MR without a JIRA ticket but we will require a ticket number to merge.
  * WindowsPerf JIRA tickets are in format: `WPERF-[0-9]+`.
* Make use of labels, milestones, assignees, and other relevant query parameters to create informative pull requests.

## Advice on merge requests

* Applying the single responsibility principle to merge requests is always a good idea. Try not to include some additional stuff into the merge request. For example, do not fix any typos other than your current context or do not add a tiny bug fix to a feature.
* Title and description is the first place where you can inform other developers about the changes.
* Description of a merge request should always be prepared with the same attention, whether the merge request has a small or huge change.
* Always think that anybody could read your merge request anytime.
* You should build your code and test (if possible) before creating the merge request.
* Both reviewers and the author should be polite in the comments.

Other:
* The source branch must be rebased onto the target branch.
* Members who can merge are allowed to add commits to merge requests.

## Commits in your merge requests

* One commit should represent one meaningful change. E.g. Please do not add a new header file and in the same commit update project solution.
* Have short (72 chars or less) meaningful subjects.
* Have a useful subject prefixed (E.g.: `"wperf: Refactor header files"`). See next chapter for details.
* Separate subject from body with a blank line.
* Use the imperative mood in the subject line.
* Wrap lines at 72 characters if possible (E.g.: URLs are very hard to wrap).
* Use the body to explain what and why you have done something. In most cases, you can leave out details about how a change has been made.

Good commit message examples can be found [here](https://wiki.openstack.org/wiki/GitCommitMessages#Information_in_commit_messages).

### Commit message prefixes

| Prefix | Code change |
| --------------------- | ----------- |
| `wperf`               | Changes to files in `windowsperf\wperf` directory. |
| `wperf-common`        | Changes to files in `windowsperf\wperf-common` directory. |
| `wperf-devgen`        | Changes to files in `windowsperf\wperf-devgen` directory. |
| `wperf-driver`        | Changes to files in `windowsperf\wperf-driver` directory. |
| `wperf-scripts`       | Changes to files in `windowsperf\wperf-scripts` directory. |
| `wperf-lib`           | Changes to files in `windowsperf\wperf-lib` directory. |
| `wperf-lib-app`       | Changes to files in `windowsperf\wperf-lib-app` directory. |
| `wperf-lib-c-compat`  | Changes to files in `windowsperf\wperf-lib-app\wperf-lib-c-compat` directory. |
| `wperf-lib-timeline`  | Changes to files in `windowsperf\wperf-lib-app\wperf-lib-timeline` directory. |
| `wperf-test`          | Changes to files in `windowsperf\wperf-test` directory. |
| `docs`                | Changes to documentation (E.g.: `README.md` or `CONTRIBUTING.md`). |
| `sln`                 | Changes to solution files `windowsperf.sln`. |
| `other`               | Changes to other files (E.g.: `.gitignore`, `Makefile`). |

You can group prefixes in your commit message subject (E.g.: `"wperf-common,docs: Add new header"`).

Example of correctly prefixed commits:
```
* 8857b6c wperf-scripts: fix missing jsonschema from requirements
* 1790de1 wperf: Remove backslashes from help message
* 86443ad wperf-driver: Do not free core_info if it's a NULL pointer
```

# If you have commit access

* Do NOT use `git push --force` on `main` branch.
* Use Merge Requests to suggest changes to other maintainers.

# Creating Reliable Kernel-Mode Drivers

To learn more please see article [Creating Reliable Kernel-Mode Drivers](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/creating-reliable-kernel-mode-drivers).

# Code style preferences

Most of the code style preferences are defined in project solution (`.sln`) and/or project files attached to this project (`.vcxproj`).

To learn more about how to define code style settings per-project see article [Code style preferences](https://learn.microsoft.com/en-us/visualstudio/ide/code-styles-and-code-cleanup?view=vs-2022).

# Creating Reliable Kernel-Mode Drivers

You can read more about how to create reliable Windows Kernel Drivers, just follow these [guidelines](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/creating-reliable-kernel-mode-drivers).

# Testing your changes

You should test your code locally before you submit a patch.

* If you are modifying `wperf` project please (if possible) add new unit tests that will cover new functions you've added. Unit test project for `wperf` is [wperf-test](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-test). See `wperf-test` [README.md](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf-test/README.md) for more details on how to write and run the tests. Unit tests isolate and exercise specific units of your code (functions). You should split your code in such a way that core functionality shouldn't be depending on e.g. user interface. Those functions can be unit tested. See examples of unit tests for [utils](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf-test/wperf-test-utils.cpp) functions for inspiration.

* You should also run regression tests using [PyTest](https://docs.pytest.org/en/) library and corresponding Python test scripts we've provided with [wperf-scripts/tests](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-scripts/tests). See `wperf-scripts/tests` [README.md](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf-scripts/tests/README.md) for more details.

* Make sure to run our stress test for at least 5 interactions. Detailed instructions can be found here [wperf-scripts/tests](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/tree/main/wperf-scripts/tests).

# Reporting Bugs

A good bug report, which is complete and self-contained, enables us to fix the bug. Before  you report a bug, please check the list of [issues](https://gitlab.com/groups/Linaro/WindowsPerf/-/issues) and, if possible, try a bleeding edge code (latest source tree commit).
 
## What we want

Please include (if possible) all of the following items (if applicable):
* Your operating system name and version. On Windows click `Start` –> `RUN` , type `winver` and press enter. You will see a popup window with your OS version.
* The complete command line that triggers the bug.
* The Kernel driver debug logs. You can grab them with [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview) .
* The MSVC toolchain warnings and errors present.
* Current `WindowsPerf` source code version. You can obtain it with `git log -1` command executed in the directory with the project solution file.
* Describe any limitations of the current code.

## What we do not want

* Screenshots, especially from text editors, command line tools, terminals. These can be copy/pasted as text.
* All sorts of attachments (binary file, source code).

# Common LICENSE tags

(Complete list can be found at: https://spdx.org/licenses)

| Identifier   | Full name |
| ------------ | --------------------------------------- |
| BSD-3-Clause | BSD 3-Clause "New" or "Revised" License |
