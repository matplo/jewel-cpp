cmake_minimum_required(VERSION 3.18)
project(jewel_cpp VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(jewel_core src/geometry.cpp src/core.cpp src/medium.cpp src/radiation.cpp src/final_splitting.cpp src/partonic_pdf.cpp src/scattering.cpp src/branching.cpp src/kinematics.cpp src/shower_state.cpp src/transport.cpp src/shower.cpp src/transfer.cpp src/evolution.cpp src/recoil.cpp src/collision.cpp src/collision_sequence.cpp src/initial_splitting.cpp)
target_include_directories(jewel_core PUBLIC include)
target_compile_options(jewel_core PRIVATE -Wall -Wextra -Wpedantic)
add_executable(jewel-probe app/probe.cpp)
target_link_libraries(jewel-probe PRIVATE jewel_core)
include(CTest)
if(BUILD_TESTING)
 find_package(Python3 QUIET COMPONENTS Interpreter)
 if(Python3_Interpreter_FOUND)
  add_test(NAME jet-comparison-contracts COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/jet_comparison.py)
 endif()
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
 file(WRITE ${CMAKE_BINARY_DIR}/reference-expanding-backend.f90 "logical function reference_vacuum_enabled()\nreference_vacuum_enabled=.false.\nend function\n")
 file(WRITE ${CMAKE_BINARY_DIR}/reference-vacuum-backend.f90 "logical function reference_vacuum_enabled()\nreference_vacuum_enabled=.true.\nend function\n")
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_medium.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/vacuum-reference.f --vacuum RESULT_VARIABLE extracted_vacuum)
 if(NOT extracted_vacuum EQUAL 0)
  message(FATAL_ERROR "Vacuum reference verification failed")
 endif()
 add_executable(jewel-vacuum-reference ${CMAKE_BINARY_DIR}/vacuum-reference.f reference/medium_probe.f90 reference/geometry_lengths.f reference/guarded_cascade.f90 reference/generated_random.cpp ${CMAKE_BINARY_DIR}/reference-vacuum-backend.f90)
 add_executable(jewel-medium-reference ${CMAKE_BINARY_DIR}/medium-reference.f reference/medium_probe.f90 reference/geometry_lengths.f reference/guarded_cascade.f90 reference/generated_random.cpp ${CMAKE_BINARY_DIR}/reference-expanding-backend.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-vacuum-reference PRIVATE "$<$<COMPILE_LANGUAGE:Fortran>:-std=legacy;-ffixed-line-length-none>")
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
 set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/reference/kinematics-source.json
  ${CMAKE_SOURCE_DIR}/reference/extract_kinematics.py)
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
 target_sources(jewel-vacuum-reference PRIVATE ${CMAKE_BINARY_DIR}/kinematics-reference.f reference/pythia_probe_stubs.f90)
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
 target_sources(jewel-vacuum-reference PRIVATE ${CMAKE_BINARY_DIR}/collision-reference.f)
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
 foreach(target jewel-medium-reference jewel-vacuum-reference jewel-coherence-reference jewel-coherent-start-reference)
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
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
   add_test(NAME pythia8-isr-input-cli COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/isr_input_cli.py $<TARGET_FILE:jewel-pythia8-input>)
  endif()
 endif()
endif()
if(JEWEL_WITH_PYTHIA8)
 target_sources(jewel_pythia8 PRIVATE adapters/pythia8_hadronizer.cpp adapters/pythia8_isr.cpp adapters/pythia8_isr_frames.cpp adapters/pythia8_primordial.cpp adapters/pythia8_isr_timing.cpp)
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

target_sources(jewel_core PRIVATE src/strings.cpp src/strings_minimum.cpp)
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
  ${CMAKE_BINARY_DIR}/compression-reference.f reference/strings_probe.f90 reference/minimum_strings_support.f90 reference/record_listing_stub.f90)
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

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME recoil-fraction-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/recoil_fraction_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/recoil-fraction-validation.json)
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_test(NAME runner-angular-contracts COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/runner_angular.py $<TARGET_FILE:jewel-run-events>
  $<TARGET_FILE:jewel-medium-probe>)
endif()

# Thermal subtraction is independent of Pythia and the Fortran production ABI.
target_sources(jewel_core PRIVATE src/subtraction.cpp)
add_executable(jewel-subtraction-probe app/subtraction_probe.cpp)
target_link_libraries(jewel-subtraction-probe PRIVATE jewel_core)
if(JEWEL_REFERENCE_SOURCE)
 file(SHA256 "${JEWEL_REFERENCE_SOURCE}/subtraction.C" subtraction_hash)
 if(NOT subtraction_hash STREQUAL "5b84552df996799d9e1061d85fdddf9ee3add56aacc2e4a4655f8e7943d82d17")
  message(FATAL_ERROR "Unrecognized JEWEL subtraction.C")
 endif()
 configure_file("${JEWEL_REFERENCE_SOURCE}/subtraction.C" "${CMAKE_BINARY_DIR}/subtraction.C" COPYONLY)
 add_executable(jewel-subtraction-reference reference/subtraction_probe.cpp)
 target_include_directories(jewel-subtraction-reference PRIVATE "${CMAKE_BINARY_DIR}")
 add_test(NAME subtraction-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/subtraction_differential.py $<TARGET_FILE:jewel-subtraction-probe>
  $<TARGET_FILE:jewel-subtraction-reference> ${CMAKE_BINARY_DIR}/subtraction-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(subtraction-test tests/subtraction.cpp)
 target_link_libraries(subtraction-test PRIVATE jewel_core)
 add_test(NAME subtraction-contracts COMMAND subtraction-test)
endif()

if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_test(NAME runner-subtraction-contracts COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/runner_subtraction.py $<TARGET_FILE:jewel-run-events>
  $<TARGET_FILE:jewel-run-partons>)
endif()

target_sources(jewel_core PRIVATE src/projection.cpp)
add_executable(jewel-projection-probe app/projection_probe.cpp)
target_link_libraries(jewel-projection-probe PRIVATE jewel_core)
if(JEWEL_REFERENCE_SOURCE)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_projection.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/projection-reference.f RESULT_VARIABLE projection_extracted)
 if(NOT projection_extracted EQUAL 0)
  message(FATAL_ERROR "Projection reference verification failed")
 endif()
 add_executable(jewel-projection-reference ${CMAKE_BINARY_DIR}/projection-reference.f
  ${CMAKE_BINARY_DIR}/kinematics-reference.f reference/projection_probe.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-projection-reference PRIVATE -std=legacy -ffixed-line-length-none -ffree-line-length-none)
 endif()
 add_test(NAME projection-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/projection_differential.py $<TARGET_FILE:jewel-projection-probe>
  $<TARGET_FILE:jewel-projection-reference> ${CMAKE_BINARY_DIR}/projection-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(projection-test tests/projection.cpp)
 target_link_libraries(projection-test PRIVATE jewel_core)
 add_test(NAME projection-contracts COMMAND projection-test)
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_test(NAME runner-projection-contracts COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/runner_projection.py $<TARGET_FILE:jewel-run-events>
  $<TARGET_FILE:jewel-run-partons>)
endif()

# Backward ISR timing: production core is C++ only; frozen block is test-only.
target_sources(jewel_core PRIVATE src/isr_times.cpp)
add_executable(jewel-isr-times-probe app/isr_times_probe.cpp)
target_link_libraries(jewel-isr-times-probe PRIVATE jewel_core)
if(JEWEL_REFERENCE_SOURCE)
 execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/reference/extract_isr_times.py
  ${JEWEL_REFERENCE_SOURCE} ${CMAKE_BINARY_DIR}/isr-times-reference.f RESULT_VARIABLE isr_times_extracted)
 if(NOT isr_times_extracted EQUAL 0)
  message(FATAL_ERROR "ISR timing reference verification failed")
 endif()
 add_executable(jewel-isr-times-reference ${CMAKE_BINARY_DIR}/isr-times-reference.f reference/isr_times_probe.f90)
 if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  target_compile_options(jewel-isr-times-reference PRIVATE -std=legacy -ffixed-line-length-none -ffree-line-length-none)
 endif()
 add_test(NAME isr-times-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/isr_times_differential.py $<TARGET_FILE:jewel-isr-times-probe>
  $<TARGET_FILE:jewel-isr-times-reference> ${CMAKE_BINARY_DIR}/isr-times-validation.json)
endif()

if(BUILD_TESTING)
 add_executable(isr-times-test tests/isr_times.cpp)
 target_link_libraries(isr-times-test PRIVATE jewel_core)
 add_test(NAME isr-times-contracts COMMAND isr-times-test)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME mpi-initialization-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/hard_initialization_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/mpi-initialization-validation.json --mpi)
endif()

if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING AND Python3_Interpreter_FOUND)
 add_test(NAME hard-range-cli COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/hard_range_cli.py $<TARGET_FILE:jewel-pythia8-input>
  $<TARGET_FILE:jewel-run-partons> ${CMAKE_BINARY_DIR}/hard-range-validation.json)
endif()

if(BUILD_TESTING)
 add_executable(geometry-test tests/geometry.cpp)
 target_link_libraries(geometry-test PRIVATE jewel_core)
 add_test(NAME geometry-contracts COMMAND geometry-test)
endif()
if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME geometry-lengths-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/geometry_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/geometry-lengths-validation.json)
 if(JEWEL_WITH_PYTHIA8)
  add_test(NAME geometry-native-output COMMAND ${Python3_EXECUTABLE}
   ${CMAKE_SOURCE_DIR}/tests/geometry_native.py $<TARGET_FILE:jewel-run-partons>
   $<TARGET_FILE:jewel-medium-reference> ${CMAKE_BINARY_DIR}/geometry-native-validation.json)
 endif()
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME minimum-strings-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/minimum_strings_differential.py $<TARGET_FILE:jewel-strings-probe>
  $<TARGET_FILE:jewel-strings-reference> ${CMAKE_BINARY_DIR}/minimum-strings-validation.json)
endif()

if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_executable(pythia8-minimum-output-test tests/pythia8_minimum_output.cpp)
 target_link_libraries(pythia8-minimum-output-test PRIVATE jewel_pythia8)
 add_test(NAME pythia8-minimum-output-contracts COMMAND pythia8-minimum-output-test)
endif()

if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING AND Python3_Interpreter_FOUND)
 add_test(NAME runner-hadrotype-contracts COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/runner_hadrotype.py $<TARGET_FILE:jewel-run-events>)
endif()

if(BUILD_TESTING AND Python3_Interpreter_FOUND)
 add_test(NAME native-hepmc3-contracts COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/native_hepmc3_contracts.py)
endif()

if(JEWEL_REFERENCE_SOURCE AND BUILD_TESTING)
 add_test(NAME isr-emission-times-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/isr_emission_times_differential.py $<TARGET_FILE:jewel-isr-times-probe>
  $<TARGET_FILE:jewel-isr-times-reference> ${CMAKE_BINARY_DIR}/isr-emission-times-validation.json)
endif()

if(JEWEL_REFERENCE_SOURCE AND BUILD_TESTING)
 add_test(NAME massless-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/massless_differential.py $<TARGET_FILE:jewel-kinematics-probe>
  $<TARGET_FILE:jewel-kinematics-reference> ${CMAKE_BINARY_DIR}/massless-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(massless-test tests/massless.cpp)
 target_link_libraries(massless-test PRIVATE jewel_core)
 add_test(NAME massless-contracts COMMAND massless-test)
endif()

if(JEWEL_REFERENCE_SOURCE AND BUILD_TESTING)
 add_test(NAME preparation-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/preparation_differential.py $<TARGET_FILE:jewel-kinematics-probe>
  $<TARGET_FILE:jewel-kinematics-reference> ${CMAKE_BINARY_DIR}/preparation-validation.json)
endif()

if(JEWEL_WITH_PYTHIA8)
 add_executable(jewel-isr-timing app/isr_timing_probe.cpp)
 target_link_libraries(jewel-isr-timing PRIVATE jewel_pythia8)
endif()

if(JEWEL_WITH_PYTHIA8)
 add_executable(jewel-native-preparation app/native_preparation_probe.cpp)
 target_link_libraries(jewel-native-preparation PRIVATE jewel_pythia8)
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING)
 add_executable(subsystem-initialization-test tests/subsystem_initialization.cpp)
 target_link_libraries(subsystem-initialization-test PRIVATE jewel_pythia8)
 add_test(NAME subsystem-initialization-contracts COMMAND subsystem-initialization-test)
endif()
if(JEWEL_WITH_PYTHIA8 AND BUILD_TESTING AND Python3_Interpreter_FOUND)
 add_test(NAME experimental-isr-mpi-contracts COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/experimental_isr_mpi.py $<TARGET_FILE:jewel-run-partons>
  $<TARGET_FILE:jewel-medium-probe> $<TARGET_FILE:jewel-run-events>)
endif()

if(JEWEL_REFERENCE_SOURCE)
 add_test(NAME vacuum-backend-differential COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/tests/vacuum_backend_differential.py $<TARGET_FILE:jewel-medium-probe>
  $<TARGET_FILE:jewel-vacuum-reference> ${CMAKE_BINARY_DIR}/vacuum-backend-validation.json)
endif()
if(BUILD_TESTING)
 add_executable(vacuum-backend-test tests/vacuum_backend.cpp)
 target_link_libraries(vacuum-backend-test PRIVATE jewel_core)
 add_test(NAME vacuum-backend-contracts COMMAND vacuum-backend-test)
endif()
