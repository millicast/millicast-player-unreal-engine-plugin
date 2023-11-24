# MillicastPlayer plugin for Unreal Engine

* Supported UE version 5.3, 5.2, 5.1, 5.0.3, 4.27
* Supported on Windows, Linux, Mac and Android

This plugin enable to play real time stream from Millicast in your Unreal Engine game.
You can configure your credentials and configure your game logic using unreal object and then render the video in a texture2D.

## Supported Codecs

* VP8 (software decoding)
* VP9 (software decoding)
* H264 is supported by using the millicast-webrtc branch and downloading the millicast version of libwebrtc. See below for setup instructions. Otherwise, for the main branch H264 decoding will result in a black screen.

## Installation

You can install the plugin from the source code.
Follow these steps :

* create a project with the UE editor.
* Close the editor
* Go at the root of your project folder (C:\Users\User\Unreal Engine\MyProject)
* Create a new directory "Plugins" and move into it
* Clone the MillicastPlayer repository : ``git clone https://github.com/millicast/millicast-player-unreal-engine-plugin.git MillicastPlayer``
* Open your project with UE

It will prompt you, saying if you want to re-build MillicastPlayer plugin, say yes.
You are now in the editor and can build your game using MillicastPlayer.

Note: After you package your game, it is possible that you will get an error when launching the game :  

> "Plugin MillicastPlayer could not be load because module MillicastPlayer has not been found"

And then the game fails to launch.
That is because Unreal has excluded the plugin.
If that is the case, create an empty C++ class in your project. This will force Unreal to include the plugin. Then, re-package the game, launch it, and it should be fixed.

### Installation steps for Mac or H264 playback

If you want to run the plugin on Mac, Android, or have H264 decoding support, you will need to use our webrtc library instead of the Unreal one. You need to add it directly into your project, in the plugin ThirdParty.

Download the ``ThirdParty.zip`` file that you can find in the release assets of the latest github pre-release.

Then, extract it and move the ``ThirdParty`` folder into the ``Source`` folder of the plugins.
After that, you should be able to build and run the plugin using our WebRTC library.

## Documentation

You can find the documentation for the plugin here: [https://docs.millicast.com/docs/millicast-player-plugin](https://docs.millicast.com/docs/millicast-player-plugin)
