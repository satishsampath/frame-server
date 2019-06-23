# Frameserver #

Frameserver is a plugin for video editing apps to do FrameServing, Image Sequence export and AudioServing.

# Features #

Frameserving and Audioserving are the techniques used to transfer audio/video data from one application to another without doing a full fledged render and temporary files. Frameserver is a plugin for NLEs enabling them to export their timeline audio/video data outside so that other applications can use the timeline directly as input.

Another use of the Frameserver is to serve audio/video data to an application that does not understand the source format. If you have a video editor that can open .MOV files and your MPEG encoder can read only .AVI input files, you can open the .MOV file in your NLE and use Frameserver to serve the a/v data to your MPEG encoder. This increases compatibility between applications.

Frameserver can also export the video from your NLE as an Image sequence. Frames can be saved in lossless BMP, TIFF, PNG and high quality JPEG formats.

# Build #

Frameserver's build uses the SCons build system (you can find the build configs in the files SConstruct and SConscript throughout the codebase). Most of the executables get built using MS Visual Studio 6.0 and some of the newer binaries require MS Visual Studio 2008.

## Requirements to build ##

  1. MS Visual Studio (tested with VS 2017 Community Edition)
  1. Windows SDK (tested with Windows 10 SDK - 10.0.17763.0)
  1. NSIS - Nullsoft Scriptable Install system, for the installer (NSIS, http://nsis.sourceforge.net/)
  1. Python, for build scripts (tested with version 2.7, http://www.python.org). Make sure you also have PIP (a tool for installing python packages) installed by following instructions at https://packaging.python.org/tutorials/installing-packages/.
  1. SCons (tested with version 1.2.0, http://www.scons.org). Please use "pip install scons" to get this.
  1. You will need the Plug-in SDKs for all the supported host applications. More info about this in the **Host SDKs** section below.

## Host SDKs ##

Frameserver works as a plug-in for various video editing 'host' applications and allows exporting the rendered video from them as .avi files. To build the plug-in for each host application, you need to have the host's plug-in SDK. You need to download each plug-in SDK yourself from the host application's website, because their Terms & Conditions do not allow redistributing them with the Frameserver codebase.

  * Vegas Pro Plug-in SDK: http://www.sonycreativesoftware.com/download/devkits - look for 'Video Plug-In Development Kit'
  * Premiere Pro Plug-in SDK: http://www.adobe.com/devnet/premiere/sdk/cs5/

Once downloaded, place the host SDK files in the following hierarchy:
```
src/SDKs
  +-- Premiere
  |     +-- compiler
  |     +-- cs4
  |     +-- cs5
  +-- VegasV2
  |     +-- libFileIO
  +-- Wax
```
## Steps to build ##

  1. Open "x64 Native Tools Command Prompt for VS 2017" or the equivalent x64 build terminal in your Visual Studio setup.
  1. Set the following environment variables in your command line (you can also do this permanently in your "System properties" dialog under the "Advanced > Environment Variables" section):
    * NSIS: set it to the full path where you have the "makensis.exe" file of your NSIS installation.
    * Add your python install directory to the path. (this is where SCons gets installed by default as well). For e.g.<br>  set PATH=%PATH%;C:\Python26
  1. From the root directory of the codebase (where you'll find the file "SConstruct"), run "scons installer". This should build the installer and place it as "fssetup.exe"
