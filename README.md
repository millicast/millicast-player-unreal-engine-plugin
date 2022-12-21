# MillicastPlayer plugin for Unreal Engine 5.1

* Supported UE version 5.1
* Supported on Windows and Linux

This plugin enable to play real time stream from Millicast in your Unreal Engine game.
You can configure your credentials and configure your game logic using unreal object and then render the video in a texture2D.

## Supported Unreal Engine

The Unreal Player supports Unreal engine 4.27, 5.0.3 and 5.1.
In order to get the plugin corresponding to your unreal version,
you must change use the right github branch.
The naming pattern follows : UEX.Y where X is the major unreal version and Y the minor.

* [Unreal 4.27 ](https://github.com/millicast/millicast-player-unreal-engine-plugin/tree/UE4.27)
* [Unreal 5.1](https://github.com/millicast/millicast-player-unreal-engine-plugin/tree/UE5.0.3)

The main is currently for Unreal UE5.1.

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

## Documentation

You can find the documentation for the plugin here: [https://docs.millicast.com/docs/millicast-player-plugin](https://docs.millicast.com/docs/millicast-player-plugin)
