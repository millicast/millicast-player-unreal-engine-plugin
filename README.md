# MillicastPlayer plugin for Unreal Engine 5.1

* Supported UE version 5.1, 5.0.3, 4.27
* Supported on Windows and Linux

This plugin enable to play real time stream from Millicast in your Unreal Engine game.
You can configure your credentials and configure your game logic using unreal object and then render the video in a texture2D.

## Supported Codecs

* VP8 (software decoding)
* VP9 (software decoding)
* H264 is **NOT** supported, you will get a black screen if you try viewing a stream encoded with H264.

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

### Installation steps for Mac

For Mac, you need to add the WebRTC module to the plugin.
Download the libWebRTC-97.0-x64-Release.tar.gz file that you can find in the release assets of the latest github pre-release for mac.
Then, extract it and move the ``ThirdParty`` folder into the ``Source`` folder of the plugins.
After that, you should be able to build and run the plugin for Mac. If you already have an xcode project just build through xcode,  otherwise launch your .uproject file to generate all the needed files. 

## Documentation

You can find the documentation for the plugin here: [https://docs.millicast.com/docs/millicast-player-plugin](https://docs.millicast.com/docs/millicast-player-plugin)
