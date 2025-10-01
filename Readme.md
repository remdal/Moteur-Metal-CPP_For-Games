/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                    +       +                      */
/*      RMDLGameApplication.h                    +++     +++                        **/
/*      Yuly_Ook_engine                              +       +                       */
/*                                       **           +       +                     **/
/*                                                  +           +                    */
/*      Created by Rémy Gralinger on 01/10/2025.     + + + + + +                     */
/*      Copyright © 2025 Laboitederemdal. All rights reserved.                       */
/*                                                                              * ****/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

Simple Engine pour compiler des shaders sur un cube en MSL (Metal) && C++ et pouvoir se déplacer dans la vue

Il contient des wrappers Objective-C/Objective-C++ obligatoire pour gérer les inputs clavier, fenêtre, ...
un joli Makefile ;)

#Lancement
git clone ..
make
open engine_metal-cpp.app
  or lldb ./engine_metal-cpp.app/Contents/MacOS/engine_metal-cpp


Erreurs actuelles :
2025-10-01 23:56:18.577395+0200 engine_metal-cpp[40863:86856037] SecTaskLoadEntitlements failed error=22 cs_flags=20, pid=40863
2025-10-01 23:56:18.577957+0200 engine_metal-cpp[40863:86856037] SecTaskCopyDebugDescription: engine_metal-cpp[40863]/0#-1 LF=22
2025-10-01 23:56:20.370969+0200 engine_metal-cpp[40863:86856037] [] warning: failed to mark memory(GRAPHICS) error 0x4 ((os/kern) invalid argument)

