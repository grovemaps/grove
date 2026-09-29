# Grove: each module (modules/<module>/, see modules/README.md) adds its sources to the upstream targets it extends,
# without a line in their CMakeLists: modules/<module>/<library>/ goes into that library, modules/<module>/tests/<test>/
# into that test. cmake/OmimHelpers.cmake and OmimTesting.cmake call this.
function(grove_module_sources target out)
  file(GLOB sources CONFIGURE_DEPENDS "${OMIM_ROOT}/modules/*/${target}/*.cpp" "${OMIM_ROOT}/modules/*/${target}/*.hpp")
  set(${out} ${sources} PARENT_SCOPE)
endfunction()
