# Grove: each module (modules/<module>/, see modules/README.md) adds its sources to the upstream targets it extends,
# without a line in their CMakeLists: modules/<module>/<library>/ goes into that library, modules/<module>/tests/<test>/
# into that test, modules/<module>/android/jni/ into Android's JNI library (organicmaps). cmake/OmimHelpers.cmake and
# OmimTesting.cmake call this.
function(grove_module_sources target out)
  set(dir ${target})
  if (target STREQUAL "organicmaps")
    set(dir android/jni)
  endif()
  file(GLOB sources CONFIGURE_DEPENDS "${OMIM_ROOT}/modules/*/${dir}/*.cpp" "${OMIM_ROOT}/modules/*/${dir}/*.hpp")
  set(${out} ${sources} PARENT_SCOPE)
endfunction()
