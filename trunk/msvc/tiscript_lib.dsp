# Microsoft Developer Studio Project File - Name="tiscript_lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=tiscript_lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tiscript_lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tiscript_lib.mak" CFG="tiscript_lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tiscript_lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "tiscript_lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tiscript_lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Ox /Oa /Og /Oi /Os /Op /Ob2 /I "../include" /I "../com" /I "../int" /I "../tool" /I "../dybase/inc" /I "../sdk/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# SUBTRACT CPP /Ot
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x1009 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "tiscript_lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "debug"
# PROP Intermediate_Dir "debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../include" /I "../com" /I "../int" /I "../tool" /I "../dybase/inc" /I "../sdk/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x1009 /d "_DEBUG"
# ADD RSC /l 0x1009 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "tiscript_lib - Win32 Release"
# Name "tiscript_lib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\int\cs_api.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_bytevector.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_cobject.cpp
# End Source File
# Begin Source File

SOURCE=..\com\cs_com.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_datetime.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_debug.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_enter.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_env.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_error.cpp
# End Source File
# Begin Source File

SOURCE=..\com\cs_eval.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_fcn.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_file.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_float.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_hash.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_heap.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_instanceof.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_int.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_integer.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_math.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_method.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_object.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_parse.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_persistent.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_printf.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_rcode.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_regexp.cpp
# End Source File
# Begin Source File

SOURCE=..\com\cs_scn.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_storage.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_stream.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_string.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_symbol.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_system.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_type.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_vector.cpp
# End Source File
# Begin Source File

SOURCE=..\com\cs_wcode.cpp
# End Source File
# Begin Source File

SOURCE=..\int\cs_xml_parser.cpp
# End Source File
# Begin Source File

SOURCE=..\int\sockio\sockio.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\cs.h
# End Source File
# Begin Source File

SOURCE=..\include\cs_com.h
# End Source File
# Begin Source File

SOURCE=..\include\cs_int.h
# End Source File
# End Group
# End Target
# End Project
