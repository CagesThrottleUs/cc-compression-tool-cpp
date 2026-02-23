# Round-trip test: encode sample file, decode, then diff.
# Expects: EXE, INPUT, COMPRESSED, DECOMPRESSED (set via -D).
if(NOT EXE OR NOT INPUT OR NOT COMPRESSED OR NOT DECOMPRESSED)
  message(FATAL_ERROR "roundtrip_encode_decode.cmake requires EXE, INPUT, COMPRESSED, DECOMPRESSED")
endif()

if(NOT EXISTS "${INPUT}")
  message(FATAL_ERROR "Input file does not exist: ${INPUT}")
endif()

# Remove outputs from a previous run so "output file already exists" does not fail
if(EXISTS "${COMPRESSED}")
  file(REMOVE "${COMPRESSED}")
endif()
if(EXISTS "${DECOMPRESSED}")
  file(REMOVE "${DECOMPRESSED}")
endif()

execute_process(
  COMMAND "${EXE}" "${INPUT}" "${COMPRESSED}"
  RESULT_VARIABLE encode_result
)
if(encode_result)
  message(FATAL_ERROR "Encode failed with result ${encode_result}")
endif()

execute_process(
  COMMAND "${EXE}" --decompress "${COMPRESSED}" "${DECOMPRESSED}"
  RESULT_VARIABLE decode_result
)
if(decode_result)
  message(FATAL_ERROR "Decode failed with result ${decode_result}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E compare_files "${INPUT}" "${DECOMPRESSED}"
  RESULT_VARIABLE diff_result
)
if(diff_result)
  message(FATAL_ERROR "Round-trip diff failed: original and decoded files differ")
endif()

message(STATUS "Round-trip encode/decode passed: ${INPUT}")
