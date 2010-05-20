# Microsoft Developer Studio Project File - Name="tool" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=tool - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tool.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tool.mak" CFG="tool - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tool - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "tool - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tool - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GX /Ox /Oa /Og /Oi /Os /Op /Ob2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# SUBTRACT CPP /Ot
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x1009 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "tool - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "debug/"
# PROP Intermediate_Dir "debug/"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
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

# Name "tool - Win32 Release"
# Name "tool - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "ucdata"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\tool\ucdata\ucdata_lt.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\ucdata\ucdata_lt.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\tool\snprintf.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\snscanf.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\threadalloc.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_base64.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_basic.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_crc32.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_datetime.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_mm_file.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_string.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_url.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_ustring.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_util.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\rp_regexp.cpp
# End Source File
# Begin Source File

SOURCE=..\tool\tl_wregexp.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\tool\html_entities_ph.h
# End Source File
# Begin Source File

SOURCE=..\tool\snprintf.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_array.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_base64.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_basic.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_config.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_datetime.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_dictionary.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_filesystem.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_hash.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_hash_table.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_markup.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_mm_file.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_pool.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_slice.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_string.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_sync.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_ternary_tree.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_type_traits.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_url.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_ustring.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_value.h
# End Source File
# Begin Source File

SOURCE=..\tool\tl_wregexp.h
# End Source File
# Begin Source File

SOURCE=..\tool\tool.h
# End Source File
# End Group
# End Target
# End Project
