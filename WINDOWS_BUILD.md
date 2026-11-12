# Making a Windows beta (no Windows PC needed)

1. Create a free private repository on github.com and upload this whole folder (the `.github` folder must come with it).
2. Open the repository's **Actions** tab, choose **Windows build**, press **Run workflow**.
3. After about 15 to 25 minutes, open the finished run and download the **StarterAmp-Windows** file at the bottom. Unzip it once: it contains the app, the VST3 and a short README.txt for testers.
4. Zip that folder and send it to your friends.

If the run fails, open it, copy the red error text and send it to me.

Notes
- The beta is not code-signed, so Windows shows a "protected your PC" warning (instructions are in README.txt).
- This builds 64-bit Windows only. AU is Mac only.
- The first Windows build has never been run by me; expect that it may need one or two fixes.


## CLAP and AAX (b94)
- **CLAP** is built by default now, next to the VST3 and the Standalone, using the open clap-juce-extensions project (fetched from GitHub the first time you configure, pinned to one commit). Switch it off with `-DSTARTERAMP_BUILD_CLAP=OFF`. The file is in the `Release/CLAP` folder of the build's artefacts. It has been compiled and checked for its entry point on Linux only; the Windows and Mac builds of it have not been run yet.
- **AAX (Pro Tools)** needs Avid's AAX SDK, which is free but must be downloaded from Avid's developer site after you register and accept their licence (it cannot be included here). Unpack it and configure with `-DSTARTERAMP_AAX_SDK_PATH=<folder>` on Windows or macOS; the AAX format is then added. Pro Tools only loads release AAX plug-ins that are signed through PACE (iLok), which needs an account with Avid's developer programme; a developer build of Pro Tools loads unsigned ones. Nothing about AAX has been compiled or run, because there is no SDK here.
