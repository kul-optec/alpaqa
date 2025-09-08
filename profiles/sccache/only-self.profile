[conf]
&:tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MSVC_DEBUG_INFORMATION_FORMAT": "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"}
&:tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_C_COMPILER_LAUNCHER": "sccache"}
&:tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_CXX_COMPILER_LAUNCHER": "sccache"}
