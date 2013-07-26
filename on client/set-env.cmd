@echo off

SET OV_DEP_ITPP=C:\Program Files\openvibe\dependencies\itpp
SET OV_DEP_CMAKE=C:\Program Files\openvibe\dependencies\cmake\cmake-2.6.2-win32-x86
SET OV_DEP_EXPAT=C:\Program Files\openvibe\dependencies\expat
SET OV_DEP_BOOST=C:\Program Files\openvibe\dependencies\boost
SET OV_DEP_GLADE=C:\Program Files\openvibe\dependencies\gtk
SET OV_DEP_ITPP=C:\Program Files\openvibe\dependencies\itpp
SET OV_DEP_OGRE=C:\Program Files\openvibe\dependencies\ogre-vc2008
SET OV_DEP_CEGUI=C:\Program Files\openvibe\dependencies\cegui-vc2008
SET OV_DEP_VRPN=C:\Program Files\openvibe\dependencies\vrpn
SET OV_DEP_LUA=C:\Program Files\openvibe\dependencies\lua

SET OGRE_HOME=C:\Program Files\openvibe\dependencies\ogre-vc2008
SET VRPNROOT=C:\Program Files\openvibe\dependencies\vrpn

SET PATH=%OV_DEP_LUA%\lib;%PATH%
SET PATH=%OV_DEP_ITPP%\bin;%PATH%
SET PATH=%OV_DEP_CMAKE%\bin;%PATH%
SET PATH=%OV_DEP_EXPAT%\bin;%PATH%
SET PATH=%OV_DEP_BOOST%\bin;%PATH%
SET PATH=%OV_DEP_GLADE%\bin;%PATH%
SET PATH=%OV_DEP_ITPP%\bin;%PATH%
SET PATH=%OV_DEP_CEGUI%\bin;%PATH%
SET PATH=%OV_DEP_OGRE%\bin\release;%OV_DEP_OGRE%\bin\debug;%PATH%
SET PATH=%OV_DEP_VRPN%\bin;%PATH%
