@rem ###########################################################################
@rem Default Configuration file, DO NOT edit, if you want to provide your own
@rem paths, please copy this file into "build_config.bat", which will be ignored
@rem by git
@rem ###########################################################################

@rem ###########################################################################
@rem # CLANG
@rem Default clang installation dir
@set CONFIG_CLANG_PATH=C:\Program Files\LLVM\bin

@rem ###########################################################################
@rem # MinGW
@rem Default MinGW installation dir
@set CONFIG_MINGW_PATH=C:\MinGW\bin
@rem MinGW 64 installation dir
@set CONFIG_MINGW_64_PATH=C:\MinGW64\bin

@rem ###########################################################################
@rem # Visual Studio
@rem Available platform tools
@set CONFIG_VS_PLATFORM_TOOLS=v100 v110 v120 v140 v141 v140_clang_c2 LLVM-vs2014

@rem ###########################################################################
@rem # PVS Studio
@rem Default PVS-Studio installation dir
@set CONFIG_PVSSTUDIO_PATH=C:\Program Files (x86)\PVS-Studio

