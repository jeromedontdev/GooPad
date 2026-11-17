# Pushing this repository to GitHub

The local repository is already initialised and committed on branch `main`.
Nothing else is needed except a remote and credentials.

## Option A — GitHub CLI (easiest)

```powershell
winget install --id GitHub.cli
gh auth login
cd C:\Projects\ESP32-S3-4848S040
gh repo create esp32-s3-4848s040-test-suite --public --source=. --push
```

`gh repo create` creates the remote, sets `origin`, and pushes in one step.

## Option B — create the repo on the web, then push

1. Create an empty repository at <https://github.com/new>.
   **Do not** add a README, .gitignore or licence — the repo already has them.
2. Then:

```powershell
cd C:\Projects\ESP32-S3-4848S040
git remote add origin https://github.com/<your-username>/esp32-s3-4848s040-test-suite.git
git push -u origin main
```

Git will prompt for credentials. Use a **personal access token** as the
password (Settings → Developer settings → Personal access tokens → Fine-grained
tokens, with `Contents: Read and write` on the target repo). Your account
password will not work.

## Option C — SSH

If you have a key registered with GitHub:

```powershell
cd C:\Projects\ESP32-S3-4848S040
git remote add origin git@github.com:<your-username>/esp32-s3-4848s040-test-suite.git
git push -u origin main
```

Test the key first with `ssh -T git@github.com`.

## Verifying the push

```powershell
git remote -v
git log --oneline -1
git status
```

`git status` should report `nothing to commit, working tree clean`.

## What gets pushed

22 files, no build artefacts:

```
.gitignore
CMakeLists.txt
LICENSE
README.md
partitions.csv
sdkconfig.defaults
main/CMakeLists.txt
main/app.h
main/board.c
main/board.h
main/console.c
main/hw_audio.c
main/hw_ble.c
main/hw_display.c
main/hw_gpio.c
main/hw_power.c
main/hw_sdcard.c
main/hw_touch.c
main/hw_wifi.c
main/idf_component.yml
main/main.c
main/settings.c
main/ui.c
```

`build/`, `managed_components/`, `sdkconfig` and `dependencies.lock` are
ignored. `sdkconfig.defaults` contains every non-default setting, so a fresh
clone builds correctly — this was verified by cloning to a clean directory,
building, flashing and running the full self-test.
