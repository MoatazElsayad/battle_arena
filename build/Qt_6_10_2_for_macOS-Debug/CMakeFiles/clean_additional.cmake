# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/Gladiators_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/Gladiators_autogen.dir/ParseCache.txt"
  "Gladiators_autogen"
  )
endif()
