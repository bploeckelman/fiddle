For web builds, install and setup emscripten
```shell
cd C:/path/to/tools
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
# (optional) Fetch the latest version of the emsdk (not needed the first time you clone)
git pull

# Download and install the latest SDK tools.
.\emsdk.bat install latest
# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
.\emsdk.bat activate latest
# (optional on windows after 'activate') Activate PATH and other environment variables in the current terminal
.\emsdk_env.bat
```
NOTE that this isn't entirely working due to toolchain configurations
- need to make sure emscripten paths are set in env vars (visible to cmake)
- maybe add ninja to PATH so it can be triggered by emcmake
- maybe setup build config in cmakelists so all toolchain bits (like ninja) are available
