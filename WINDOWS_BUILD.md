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
