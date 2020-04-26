# Installing FMPy on Windows

This installation

- does not require admin privileges
- does not modify registry and PATH
- can easily be removed by running the uninstaller

1. Download and run the [Miniconda Installer](https://repo.anaconda.com/miniconda/Miniconda3-latest-Windows-x86_64.exe)

2. Click `Next`

<img src="Miniconda_Welcome.PNG" width="300">

3. Click `I Agree`

<img src="Miniconda_License_Agreement.PNG" width="300">

4. Select `Just Me (recommended)` and click `Next`

<img src="Miniconda_Install_Type.PNG" width="300">

5. Select a local folder where you have write access (not a network folder) and click `Next`

<img src="Miniconda_Install_Location.PNG" width="300">

6. Uncheck both options and click `Install`

<img src="Miniconda_Advanced_Installation_Options.PNG" width="300">

7. Uncheck both options and click `Finish`

<img src="Miniconda_Completing_Setup.PNG" width="300">

8. Open `Start Menu > Anaconda Prompt (Miniconda3)` and run the command `conda install -y -c conda-forge fmpy`

```
(base) C:\>conda install -y -c conda-forge fmpy
Collecting package metadata (current_repodata.json): done
Solving environment: done

...

Downloading and Extracting Packages

...

Preparing transaction: done
Verifying transaction: done
Executing transaction: done
```

## Run the FMPy GUI

Open `Start Menu > Anaconda Prompt (Miniconda3)` and run the command `python -m fmpy.gui`
