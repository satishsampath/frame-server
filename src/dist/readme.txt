README for DebugMode FrameServer
--------------------------------

This file is a part of the setup distribution. If you are reading this file now, you have already installed DebugMode FrameServer. This has been tested with many third party software, including

Sony Vegas 10, 9, 6.0, 5.0, 4.0/SonicFoundry Vegas Video 3.0
Adobe Premiere CS5, CS4, 6.0/6.5, Premiere Pro 1.0/1.5, Premiere Elements 1.0
Ulead MediaStudio Pro 7.0
Ulead VideoStudio 8.0
Pure Motion EditStudio 4.1.3
Windows media player
VirtualDub
Avisynth
Procoder
Cinema Craft Encoder (CCE) 2.5 to 2.67
TMPGEnc 2.510
and many more...

For support, please visit the UserForum at "http://www.debugmode.com/forums/"

Thanks to DDogg for the immense help in testing out the Frameserver plugins with a myraid of applications which i have never even heard of. ;)

Known issues
-------------

1) If audio is not saved as PCM with AVI file, seek operation will fail if started from the middle of the served video.


Usage
======

For information about NETWORK FRAMESERVING, scroll down one page.


Vegas/VideoFectory users
------------------------

The frameserver plugin will be installed in the "<Vegas-Video-FileIO-Plug-ins-folder>" folder. 

To use the FrameServer, you have to render the timeline to an AVI file using the
"DebugMode FrameServer" file type.

Premiere/PremierePro/PremiereElements users
------------------------------------------

The frameserver plugin will be installed in the "<Premiere-Plug-ins-folder>" folder. 

To use the FrameServer, you have to render the timeline to "DebugMode FrameServer" using
the menu option "File > Export Timeline > Movie" and choosing "DebugMode FrameServer"
(in Premiere 6.0/6.5, other versions may vary). For PremierePro/PremiereElements, the menu option is 
"File > Export > Movie". Also remember to uncheck the "Add to Project when finished" option
to prevent the signpost AVI file being added to the project after frameserving.


Pure Motion EditStudio users
----------------------------

The frameserver plugin will be installed in the "<EditStudio-Plugins-folder>" folder. 

To use the FrameServer, you have to select "File > Build movie" and choose the preset
"Debugmode FrameServer (project settings)".


Ulead MediaStudioPro users
--------------------------

The frameserver plugin will be installed in the "<UleadMSPro-vio-Plug-ins-folder>"
folder. 

To use the FrameServer, you have to render the timeline to "DebugMode FrameServer" using
the menu option "File > Create > Video File" (in MSPro 7.0, other versions may vary). 
Make sure you click the "Options" button and disable the "Play after creating" option 
before rendering.


Ulead VideoStudio users
--------------------------

The frameserver plugin will be installed in the "<UleadVideoStudio-vio-Plug-ins-folder>"
folder. 

To use the FrameServer choose menu "Share > Create Video File" and select the "Custom" template.
That will bring up the "Create video file" dialog. In the "Save as type" combo box choose
"DebugMode FrameServer Files" and give the AVI filename to save. Click "Save" and the Frameserver starts.
(These steps are for VideoStudio 8.0, other versions may vary). 


Network Frameserving
---------------------

Debugmode FrameServer can also serve audio and video across the network (upto 8 clients). This feature is still
in developmental stages, but working successfully. The bandwidth in a common 100Mbps ethernet network will
not be enough for realtime network frameserving, so try this only if you have access to a Gigabit network.
(Of course it will work in a 100Mbps network also but the performance will be well below realtime).


Here are the steps..

1) Make sure all the client machines have the frameserver installed.
2) Open your NLE and render the project using the FrameServer. In the Frameserver options dialog, 
   check the "Enable Network Frameserving" option. You can use the default port value for the network
   connection, or if you have problems change it to something else.
3) Click the next button and the frameserver is ready.
4) On the client machine run the "FrameServer Network Client" application from the FrameServer
   start menu folder. Give the IP address and port of the server machine.
5) Also give the path to which the signpost file must be saved. When doing network frameserving,
   DO NOT COPY THE SIGNPOST AVI file to the client machines. The signpost AVI will be automatically
   copied over the network by the client and saved to the given path.
6) Click the OK button in the client and the signpost AVI file will be created in the client machine.
   Now you can use the signpost AVI in the client. When finished, close both the FrameServer client and 
   server applications.

If you have tried network serving and have any suggestions/comments/issues please drop a mail or visit the
Frameserver User Forums at debugmode.com

Thanks
Satish
