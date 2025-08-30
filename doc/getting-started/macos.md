# macOS Setup Guide

Install [Docker Desktop](https://docs.docker.com/desktop/setup/install/mac-install/).
Download it, run the installer and follow any installation prompts.

Install [Visual Studio Code](https://code.visualstudio.com/Download)

Open Visual Studio Code, and navigate to the Extensions menu located at the bottom of the left hand side bar.
Install the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension.

![vscode extension](../img/vscode-extension.png)

Install [git](https://git-scm.com/downloads) if you do not have it already.
You can install it using [Homebrew](https://brew.sh/) (`brew install git`), or downloading from their [website](https://git-scm.com/downloads).

git clone ukoOS (`git clone https://github.com/UMN-Kernel-Object/ukoos`), open the folder in Visual Studio Code (File -> Open Folder).
It should prompt you to `reopen in Dev Container.` If not, press `Cmd` + `Shift` + `P` and type `Reopen in Dev Container`.

You are now in the ukoOS Dev Container.
To verify this, run the below command and verify the line `NAME="Alpine Linux"` is present.
`cat /etc/os-release`

When you have a change ready to be commited, you must sign off your commits. Your commits should look something like this:
`git commit -s -m 'commit message'`
