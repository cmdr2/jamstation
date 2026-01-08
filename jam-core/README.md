# jam-core

# Building

1. Open the developer console for Visual Studio.
2. Set the `JUCE_HOME` environment variable to the path where JUCE is installed, e.g. `C:\path\to\JUCE`.
2. Run `cmake -B build` (you can also add `-G Ninja` for faster builds, if Ninja is installed)
3. Run `cmake --build build --config Release`
