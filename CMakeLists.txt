cmake_minimum_required(VERSION 3.18)
project(jewel_cpp VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(jewel_core src/core.cpp src/medium.cpp src/radiation.cpp src/final_splitting.cpp src/partonic_pdf.cpp src/scattering.cpp src/branching.cpp src/kinematics.cpp src/shower_state.cpp)
target_include_directories(jewel_core PUBLIC include)
target_compile_options(jewel_core PRIVATE -Wall -Wextra -Wpedantic)
add_executable(jewel-probe app/probe.cpp)
target_link_libraries(jewel-probe PRIVATE jewel_core)
include(CTest)
if(BUILD_TESTING)
 add_executable(core-test tests/core.cpp)
 target_link_libraries(core-test PRIVATE jewel_core)
 add_test(NAME core-contracts COMMAND core-test)
 add_executable(medium-test tests/medium.cpp)
 target_link_libraries(medium-test PRIVATE jewel_core)
 add_test(NAME medium-contracts COMMAND medium-test)
 add_executable(radiation-test tests/radiation.cpp)
 target_link_libraries(radiation-test PRIVATE jewel_core)
 add_test(NAME radiation-contracts COMMAND radiation-test)
 add_executable(final-splitting-test tests/final_splitting.cpp)
 target_link_libraries(final-splitting-test PRIVATE jewel_core)
 add_test(NAME final-splitting-contracts COMMAND final-splitting-test)
 add_executable(pdf-test tests/partonic_pdf.cpp)
 target_link_libraries(pdf-test PRIVATE jewel_core)
 add_test(NAME pdf-contracts COMMAND pdf-test)
 add_executable(scattering-test tests/scattering.cpp)
 target_link_libraries(scattering-test PRIVATE jewel_core)
 add_test(NAME scattering-contracts COMMAND scattering-test)
 add_executable(branching-test tests/branching.cpp)
 target_link_libraries(branching-test PRIVATE jewel_core)
 add_test(NAME branching-contracts COMMAND branching-test)
 add_executable(shower-state-test tests/shower_state.cpp)
 target_link_libraries(shower-state-test PRIVATE jewel_core)
 add_test(NAME shower-state-contracts COMMAND shower-state-test)
endif()
set(JEWEL_REFERENCE_SOURCE "" CACHE PATH "Frozen JEWEL 2.6.0 source directory")
if(JEWEL_REFERENCE_SOURCE)
 enable_language(Fortran)
 find_package(Python3 REQUIRED COMPONENTS Interpreter)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/reference.f RESULT_VARIABLE extracted)
 if(NOT extracted EQUAL 0)
  message(FATAL_ERROR "Reference source verification/extraction failed")
 endif()
 add_executable(jewel-reference ${CMAKE_BINARY_DIR}/reference.f reference/probe.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-reference PRIVATE -std=legacy -ffixed-line-length-none)
 endif()
 add_test(NAME reference-harness-contracts COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/harness.py)
 add_test(NAME fortran-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/differential.py $<TARGET_FILE:jewel-probe>
  $<TARGET_FILE:jewel-reference> ${CMAKE_BINARY_DIR}/validation.json)
endif()

add_executable(jewel-medium-probe app/medium_probe.cpp)
target_link_libraries(jewel-medium-probe PRIVATE jewel_core)
if(JEWEL_REFERENCE_SOURCE)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_medium.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/medium-reference.f RESULT_VARIABLE extracted_medium)
 if(NOT extracted_medium EQUAL 0)
  message(FATAL_ERROR "Medium reference verification failed")
 endif()
 add_executable(jewel-medium-reference ${CMAKE_BINARY_DIR}/medium-reference.f reference/medium_probe.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-medium-reference PRIVATE -std=legacy -ffixed-line-length-none)
 endif()
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME medium-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/medium_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/medium-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME sampling-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/sampling_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/sampling-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME radiation-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/radiation_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/radiation-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME final-splitting-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/final_splitting_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/final-splitting-validation.json)
 set_tests_properties(final-splitting-differential PROPERTIES LABELS "full-tables")
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME pdf-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/pdf_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/pdf-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME scattering-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/scattering_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/scattering-validation.json)
 set_tests_properties(scattering-differential PROPERTIES LABELS "full-tables")
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME branching-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/branching_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/branching-validation.json)
endif()

add_executable(jewel-kinematics-probe app/kinematics_probe.cpp)
target_link_libraries(jewel-kinematics-probe PRIVATE jewel_core)
if(JEWEL_REFERENCE_SOURCE)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_kinematics.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/kinematics-reference.f RESULT_VARIABLE extracted_kinematics)
 if(NOT extracted_kinematics EQUAL 0)
  message(FATAL_ERROR "Kinematics reference verification failed")
 endif()
 add_executable(jewel-kinematics-reference ${CMAKE_BINARY_DIR}/kinematics-reference.f reference/kinematics_probe.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-kinematics-reference PRIVATE -std=legacy -ffixed-line-length-none)
 endif()
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME kinematics-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/kinematics_differential.py $<TARGET_FILE:jewel-kinematics-probe>
  $<TARGET_FILE:jewel-kinematics-reference> ${CMAKE_BINARY_DIR}/kinematics-validation.json)
endif()
