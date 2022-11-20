# Contributing Guidelines

# Basic guidelines

All changes you commit or submit by merge request should follow these simple guidelines:
* Use merge requests.
* Should build without new warnings or errors. Please use project's solution file to drive build and test process.
* Please do not squash your Merge Request's commits into one commit. Split PR into many meaningful commit, we can review separately.
* Code should be free from profanities in any language.

# Commits in your merge requests should

* One commit should represent one meaningful change. E.g. please do not add new header file and in the same commit update project solution.
* Have short (72 chars or less) meaningful subject.
* Have a useful subject prefixed (E.g.: `"wperf: Refactor header files"`). See next chapter for details.
* Separate subject from body with a blank line.
* Use the imperative mood in the subject line.
* Wrap lines at 72 characters if possible (E.g.: URLs are very hard to wrap).
* Use the body to explain what and why you have done something. In most cases, you can leave out details about how a change has been made.

Good commit message examples can be found [here](https://wiki.openstack.org/wiki/GitCommitMessages#Information_in_commit_messages).

## Description prefixed

| Prefix | Code change |
| -------------- | ----------- |
| `wperf` 			| Changes to files in `WindowsPerf\wperf` directory. |
| `wperf-driver` 	| Changes to files in `WindowsPerf\wperf-driver` directory. |
| `wperf-test` 	    | Changes to files in `WindowsPerf\wperf-test` directory. |
| `common` 			| Changes to files in `WindowsPerf\wperf-common` directory. |
| `docs`  			| Changes to documentation (E.g.: `README.md`). |
| `sln` 			| Changes to solution files `windowsperf.sln`. |
| `other` 			| Changes to other files (E.g.: `.gitignore`). |

You can group prefixes in your commit message subject (E.g.: `"wperf-common,docs: Add new header"`).

# Advice on merge requests

* Applying the single responsibility principle to merge requests is always a good idea. Try not to include some additional stuff into the merge request. For example, do not fix any typos other than your current context or do not add a tiny bug fix to a feature.
* Title and description is the first place where you can inform other developers about the changes
* Description of a merge request should always be prepared with the same attention, whether the merge request has a small or huge change.
* Always think that anybody could read your merge request anytime.
* You should build your code and test (if possible) before creating the merge request.
* Both reviewers and the author should be polite in the comments.

# If you have commit access

* Do NOT use `git push --force`.
* Use Merge Requests to suggest changes to other maintainers.

# Creating Reliable Kernel-Mode Drivers

To learn more please see article [Creating Reliable Kernel-Mode Drivers](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/creating-reliable-kernel-mode-drivers).

# Code style preferences

Most of code style preferences are defined in project solution (`.sln`) and/or project files attached to this project (`.vcxproj`).

To learn more about how to define code style settings per-project see article [Code style preferences](https://learn.microsoft.com/en-us/visualstudio/ide/code-styles-and-code-cleanup?view=vs-2022).

# Reporting Bugs

A good bug report, which is complete and self-contained, enables us to fix the bug. Before  you report a bug, please check the list of [issues](https://gitlab.com/groups/Linaro/WindowsPerf/-/issues) and, if possible, try a bleeding edge code (latest source tree commit).
 
## What we want

Please include (if possible) all of the following items (if applicable):
* Your operating system name and version. On Windows click `Start` –> `RUN` , type `winver` and press enter. You will see popup window with your OS version.
* The complete command line that triggers the bug.
* The Kernel driver debug logs. You can grab them with [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview) .
* The MSVC toolchain warnings and errors present.
* Current `WindowsPerf` source code version. You can obtain it with `git log -1` command executed in directory with project solution file.
* Describe any limitations of the current code.

## What we do not want

* Screenshots, especially from text editors, command line tools, terminals. These can be copy/pasted as text.
* All sorts of attachments (binary file, source code).

# Common LICENSE tags

(Complete list can be found at: https://spdx.org/licenses)

| Identifier   | Full name |
| ------------ | --------------------------------------- |
| BSD-3-Clause | BSD 3-Clause "New" or "Revised" License |
