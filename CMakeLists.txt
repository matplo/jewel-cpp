cmake_minimum_required(VERSION 3.18)
project(jewel_cpp VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(jewel_core src/core.cpp src/medium.cpp src/radiation.cpp src/final_splitting.cpp src/partonic_pdf.cpp src/scattering.cpp src/branching.cpp src/kinematics.cpp src/shower_state.cpp src/transport.cpp src/shower.cpp src/transfer.cpp src/evolution.cpp src/recoil.cpp src/collision.cpp src/collision_sequence.cpp src/initial_splitting.cpp)
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
 add_executable(evolution-test tests/evolution.cpp)
 target_link_libraries(evolution-test PRIVATE jewel_core)
 add_test(NAME evolution-contracts COMMAND evolution-test)
 add_executable(transfer-test tests/transfer.cpp)
 target_link_libraries(transfer-test PRIVATE jewel_core)
 add_test(NAME transfer-contracts COMMAND transfer-test)
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
 add_executable(jewel-medium-reference ${CMAKE_BINARY_DIR}/medium-reference.f reference/medium_probe.f90 reference/guarded_cascade.f90 reference/generated_random.cpp)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-medium-reference PRIVATE "$<$<COMPILE_LANGUAGE:Fortran>:-std=legacy;-ffixed-line-length-none>")
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

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME transport-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/transport_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/transport-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 target_sources(jewel-medium-reference PRIVATE ${CMAKE_BINARY_DIR}/kinematics-reference.f reference/pythia_probe_stubs.f90)
 add_test(NAME rejection-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/rejection_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/rejection-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME splitting-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/splitting_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/splitting-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME virtuality-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/virtuality_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/virtuality-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME transfer-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/transfer_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/transfer-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME final-scattering-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/final_scattering_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/final-scattering-validation.json)
endif()

add_executable(jewel-recoil-probe app/recoil_probe.cpp)
target_link_libraries(jewel-recoil-probe PRIVATE jewel_core)
if(BUILD_TESTING)
 add_executable(recoil-test tests/recoil.cpp)
 target_link_libraries(recoil-test PRIVATE jewel_core)
 add_test(NAME recoil-contracts COMMAND recoil-test)
endif()
if(JEWEL_REFERENCE_SOURCE)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_recoil.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/recoil-reference.f RESULT_VARIABLE recoil_extracted)
 if(NOT recoil_extracted EQUAL 0)
  message(FATAL_ERROR "Recoil reference extraction failed")
 endif()
 add_executable(jewel-recoil-reference ${CMAKE_BINARY_DIR}/recoil-reference.f reference/recoil_probe.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-recoil-reference PRIVATE -std=legacy -ffixed-line-length-none)
 endif()
 add_test(NAME recoil-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/recoil_differential.py $<TARGET_FILE:jewel-recoil-probe>
  $<TARGET_FILE:jewel-recoil-reference> ${CMAKE_BINARY_DIR}/recoil-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_collision.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/collision-reference.f RESULT_VARIABLE collision_extracted)
 if(NOT collision_extracted EQUAL 0)
  message(FATAL_ERROR "Collision reference extraction failed")
 endif()
 target_sources(jewel-medium-reference PRIVATE ${CMAKE_BINARY_DIR}/collision-reference.f)
 add_test(NAME collision-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/collision_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/collision-validation.json)
endif()

if(BUILD_TESTING)
 add_executable(collision-test tests/collision.cpp)
 target_link_libraries(collision-test PRIVATE jewel_core)
 add_test(NAME collision-contracts COMMAND collision-test)
endif()
if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME collision-step-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/collision_step_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/collision-step-validation.json)
endif()
if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME collision-sequence-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/collision_sequence_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/collision-sequence-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(collision-sequence-test tests/collision_sequence.cpp)
 target_link_libraries(collision-sequence-test PRIVATE jewel_core)
 add_test(NAME collision-sequence-contracts COMMAND collision-sequence-test)
endif()
if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME initial-splitting-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/initial_splitting_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/initial-splitting-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(initial-splitting-test tests/initial_splitting.cpp)
 target_link_libraries(initial-splitting-test PRIVATE jewel_core)
 add_test(NAME initial-splitting-contracts COMMAND initial-splitting-test)
endif()

# Coherent-scattering arithmetic dependencies, independently replayable.
target_sources(jewel_core PRIVATE src/coherence.cpp)
add_executable(jewel-coherence-probe app/coherence_probe.cpp)
target_link_libraries(jewel-coherence-probe PRIVATE jewel_core)
if(JEWEL_REFERENCE_SOURCE)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_coherence.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/coherence-reference.f RESULT_VARIABLE coherence_extracted)
 if(NOT coherence_extracted EQUAL 0)
  message(FATAL_ERROR "Coherence reference extraction failed")
 endif()
 add_executable(jewel-coherence-reference ${CMAKE_BINARY_DIR}/coherence-reference.f
  ${CMAKE_BINARY_DIR}/kinematics-reference.f reference/coherence_probe.f90 reference/pythia_probe_stubs.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-coherence-reference PRIVATE -std=legacy -ffixed-line-length-none)
 endif()
 add_test(NAME coherence-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/coherence_differential.py $<TARGET_FILE:jewel-coherence-probe>
  $<TARGET_FILE:jewel-coherence-reference> ${CMAKE_BINARY_DIR}/coherence-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(coherence-test tests/coherence.cpp)
 target_link_libraries(coherence-test PRIVATE jewel_core)
 add_test(NAME coherence-contracts COMMAND coherence-test)
endif()
add_executable(jewel-coherent-start-probe app/coherent_start_probe.cpp)
target_link_libraries(jewel-coherent-start-probe PRIVATE jewel_core)
if(JEWEL_REFERENCE_SOURCE)
 add_executable(jewel-coherent-start-reference ${CMAKE_BINARY_DIR}/coherence-reference.f
  ${CMAKE_BINARY_DIR}/kinematics-reference.f reference/coherent_start_probe.f90 reference/pythia_probe_stubs.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-coherent-start-reference PRIVATE -std=legacy -ffixed-line-length-none)
 endif()
 add_test(NAME coherent-start-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/coherent_start_differential.py $<TARGET_FILE:jewel-coherent-start-probe>
  $<TARGET_FILE:jewel-coherent-start-reference> ${CMAKE_BINARY_DIR}/coherent-start-validation.json)
endif()

target_sources(jewel_core PRIVATE src/coherent_evolution.cpp)
if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME coherent-evolution-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/coherent_evolution_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/coherent-evolution-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(coherent-evolution-test tests/coherent_evolution.cpp)
 target_link_libraries(coherent-evolution-test PRIVATE jewel_core)
 add_test(NAME coherent-evolution-contracts COMMAND coherent-evolution-test)
endif()

target_sources(jewel_core PRIVATE src/initial_evolution.cpp)
if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME initial-evolution-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/initial_evolution_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/initial-evolution-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(initial-evolution-test tests/initial_evolution.cpp)
 target_link_libraries(initial-evolution-test PRIVATE jewel_core)
 add_test(NAME initial-evolution-contracts COMMAND initial-evolution-test)
endif()

target_sources(jewel_core PRIVATE src/cascade.cpp)

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME cascade-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/cascade_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/cascade-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(cascade-test tests/cascade.cpp)
 target_link_libraries(cascade-test PRIVATE jewel_core)
 add_test(NAME cascade-contracts COMMAND cascade-test)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME cascade-generated-baseline COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/cascade_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/cascade-generated-baseline.json
  --baseline --guarded --generated)
endif()

if(TARGET jewel-medium-reference)
 add_test(NAME cascade-collision-retry COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/cascade_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/cascade-collision-retry.json
  --generated --guarded --case 55 --seed-offset 1000500 --coherence off)
endif()

# Source-compatible compression at the generator-record boundary.
target_sources(jewel_core PRIVATE src/compression.cpp)
add_executable(jewel-compression-probe app/compression_probe.cpp)
target_link_libraries(jewel-compression-probe PRIVATE jewel_core)
if(JEWEL_REFERENCE_SOURCE)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_compression.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/compression-reference.f RESULT_VARIABLE extracted_compression)
 if(NOT extracted_compression EQUAL 0)
  message(FATAL_ERROR "Compression reference verification failed")
 endif()
 add_executable(jewel-compression-reference ${CMAKE_BINARY_DIR}/compression-reference.f reference/compression_probe.f90 reference/record_listing_stub.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-compression-reference PRIVATE -std=legacy -ffixed-line-length-none)
 endif()
 add_test(NAME compression-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/compression_differential.py $<TARGET_FILE:jewel-compression-probe>
  $<TARGET_FILE:jewel-compression-reference> ${CMAKE_BINARY_DIR}/compression-validation.json)
endif()

if(BUILD_TESTING)
 add_executable(compression-test tests/compression.cpp)
 target_link_libraries(compression-test PRIVATE jewel_core)
 add_test(NAME compression-contracts COMMAND compression-test)
endif()

if(JEWEL_REFERENCE_SOURCE)
 foreach(target jewel-medium-reference jewel-coherence-reference jewel-coherent-start-reference)
  target_sources(${target} PRIVATE ${CMAKE_BINARY_DIR}/compression-reference.f reference/record_listing_stub.f90)
 endforeach()
 add_test(NAME cascade-compression-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/cascade_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/cascade-compression.json
  --baseline --generated --compression-padding 20000)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME cascade-compression-boundary COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/cascade_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/cascade-compression-boundary.json
  --case 38 --generated --compression-padding 19995)
 add_test(NAME cascade-compression-medium COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/cascade_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/cascade-compression-coherent.json
  --case 44 --generated --compression-padding 20000)
endif()

option(JEWEL_WITH_PYTHIA8 "Build optional Pythia8 input adapter" OFF)
set(JEWEL_PYTHIA8_ROOT "" CACHE PATH "Pinned Pythia8 installation prefix")
if(JEWEL_WITH_PYTHIA8)
 if(NOT JEWEL_PYTHIA8_ROOT)
  message(FATAL_ERROR "JEWEL_WITH_PYTHIA8 requires an explicit JEWEL_PYTHIA8_ROOT")
 endif()
 foreach(variable PYTHIA8_INCLUDE_DIR PYTHIA8_LIBRARY PYTHIA8_XML_DIR)
  unset(${variable} CACHE)
 endforeach()
 find_path(PYTHIA8_INCLUDE_DIR Pythia8/Pythia.h HINTS ${JEWEL_PYTHIA8_ROOT}/include NO_DEFAULT_PATH REQUIRED)
 find_library(PYTHIA8_LIBRARY NAMES pythia8 HINTS ${JEWEL_PYTHIA8_ROOT}/lib NO_DEFAULT_PATH REQUIRED)
 find_path(PYTHIA8_XML_DIR Index.xml HINTS ${JEWEL_PYTHIA8_ROOT}/share/Pythia8/xmldoc NO_DEFAULT_PATH REQUIRED)
 add_library(jewel_pythia8 adapters/pythia8_input.cpp)
 target_link_libraries(jewel_pythia8 PUBLIC jewel_core PRIVATE ${PYTHIA8_LIBRARY})
 target_include_directories(jewel_pythia8 PRIVATE ${PYTHIA8_INCLUDE_DIR})
 target_compile_definitions(jewel_pythia8 PRIVATE JEWEL_PYTHIA8_XML="${PYTHIA8_XML_DIR}")
 target_compile_options(jewel_pythia8 PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(JEWEL_WITH_PYTHIA8)
 add_executable(jewel-pythia8-input app/pythia8_input.cpp)
 target_link_libraries(jewel-pythia8-input PRIVATE jewel_pythia8)
 if(BUILD_TESTING)
  add_executable(pythia8-input-test tests/pythia8_input.cpp)
  target_link_libraries(pythia8-input-test PRIVATE jewel_pythia8)
  add_test(NAME pythia8-input-contracts COMMAND pythia8-input-test)
 endif()
endif()
if(JEWEL_WITH_PYTHIA8)
 target_sources(jewel_pythia8 PRIVATE adapters/pythia8_hadronizer.cpp)
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_executable(pythia8-boundary-test tests/pythia8_boundary.cpp)
 target_link_libraries(pythia8-boundary-test PRIVATE jewel_pythia8)
 add_test(NAME pythia8-boundary-contracts COMMAND pythia8-boundary-test)
endif()

target_sources(jewel_core PRIVATE src/hard_initialization.cpp)
if(BUILD_TESTING)
 add_executable(hard-initialization-test tests/hard_initialization.cpp)
 target_link_libraries(hard-initialization-test PRIVATE jewel_core)
 add_test(NAME hard-initialization-contracts COMMAND hard-initialization-test)
endif()
if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME hard-initialization-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/hard_initialization_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/hard-initialization.json)
endif()
if(JEWEL_WITH_PYTHIA8)
 target_sources(jewel_pythia8 PRIVATE adapters/pythia8_shower_input.cpp)
endif()
if(JEWEL_WITH_PYTHIA8)
 add_executable(jewel-run-partons app/run_partons.cpp)
 target_link_libraries(jewel-run-partons PRIVATE jewel_pythia8)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME cascade-native-input-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/cascade_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/cascade-native-input.json
  --baseline --generated --native-input)
endif()

target_sources(jewel_core PRIVATE src/strings.cpp)
add_executable(jewel-strings-probe app/strings_probe.cpp)
target_link_libraries(jewel-strings-probe PRIVATE jewel_core)
if(BUILD_TESTING)
 add_executable(strings-test tests/strings.cpp)
 target_link_libraries(strings-test PRIVATE jewel_core)
 add_test(NAME strings-contracts COMMAND strings-test)
endif()
if(JEWEL_REFERENCE_SOURCE)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_strings.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/strings-reference.f RESULT_VARIABLE strings_extracted)
 if(NOT strings_extracted EQUAL 0)
  message(FATAL_ERROR "String reference extraction failed")
 endif()
 add_executable(jewel-strings-reference ${CMAKE_BINARY_DIR}/strings-reference.f
  ${CMAKE_BINARY_DIR}/compression-reference.f reference/strings_probe.f90 reference/record_listing_stub.f90)
 target_compile_options(jewel-strings-reference PRIVATE -std=legacy -ffixed-line-length-none)
endif()
if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME strings-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/strings_differential.py $<TARGET_FILE:jewel-strings-probe>
  $<TARGET_FILE:jewel-strings-reference> ${CMAKE_BINARY_DIR}/strings-validation.json)
endif()
if(JEWEL_WITH_PYTHIA8)
 target_sources(jewel_pythia8 PRIVATE adapters/pythia8_shower_output.cpp)
 add_executable(jewel-run-events app/run_partons.cpp)
 target_compile_definitions(jewel-run-events PRIVATE JEWEL_HADRONIZE_RUN=1)
 target_link_libraries(jewel-run-events PRIVATE jewel_pythia8)
 if(BUILD_TESTING)
  add_executable(pythia8-shower-output-test tests/pythia8_shower_output.cpp)
  target_link_libraries(pythia8-shower-output-test PRIVATE jewel_pythia8)
  add_test(NAME pythia8-shower-output-contracts COMMAND pythia8-shower-output-test)
 endif()
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_executable(pythia8-fragmentation-test tests/pythia8_fragmentation.cpp)
 target_link_libraries(pythia8-fragmentation-test PRIVATE jewel_pythia8)
 add_test(NAME pythia8-fragmentation-contracts COMMAND pythia8-fragmentation-test)
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_executable(pythia8-external-pdf-test tests/pythia8_external_pdf.cpp)
 target_link_libraries(pythia8-external-pdf-test PRIVATE jewel_pythia8)
 set(JEWEL_TEST_EXTERNAL_PDF "" CACHE STRING "Optional installed LHAPDF SET/MEMBER for integration testing")
 if(JEWEL_TEST_EXTERNAL_PDF)
  add_test(NAME pythia8-external-pdf-contracts COMMAND pythia8-external-pdf-test ${JEWEL_TEST_EXTERNAL_PDF})
 endif()
endif()
if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME event-geometry-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/event_geometry_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/event-geometry-validation.json)
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 find_package(Python3 REQUIRED COMPONENTS Interpreter)
 add_test(NAME runner-flow-contracts COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/runner_flow.py $<TARGET_FILE:jewel-run-events>
  $<TARGET_FILE:jewel-medium-probe>)
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_test(NAME runner-recoils-contracts COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/runner_recoils.py $<TARGET_FILE:jewel-run-events>
  $<TARGET_FILE:jewel-medium-probe>)
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_test(NAME runner-recoil-controls COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/runner_recoil_controls.py $<TARGET_FILE:jewel-run-events>
  $<TARGET_FILE:jewel-medium-probe>)
endif()
