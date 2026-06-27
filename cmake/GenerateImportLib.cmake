function(target_link_import_library CONSUMER_TARGET LIB_TARGET DEF_FILE)
  if (WIN32)
    get_filename_component(DEF_PATH "${DEF_FILE}" ABSOLUTE)
    get_filename_component(DEF_NAME "${DEF_PATH}" NAME_WE)

    set(LIB_PATH "${CMAKE_CURRENT_BINARY_DIR}/${DEF_NAME}.lib")

    if (MSVC)
      set(MSVC_LIB_EXE "")

      if (CMAKE_AR)
        set(MSVC_LIB_EXE "${CMAKE_AR}")
      endif()

      if (NOT MSVC_LIB_EXE AND CMAKE_LINKER)
        get_filename_component(linker_dir "${CMAKE_LINKER}" DIRECTORY)
        find_program(MSVC_LIB_EXE NAMES lib.exe PATHS "${linker_dir}" NO_DEFAULT_PATH)
      endif()

      if (NOT MSVC_LIB_EXE AND CMAKE_CXX_COMPILER)
        get_filename_component(compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        find_program(MSVC_LIB_EXE NAMES lib.exe PATHS "${compiler_dir}" NO_DEFAULT_PATH)
      endif()

      if (NOT MSVC_LIB_EXE)
        find_program(MSVC_LIB_EXE NAMES lib.exe)
      endif()

      if (NOT MSVC_LIB_EXE)
        message(FATAL_ERROR "Could not find MSVC's lib.exe to generate import library")
      endif()

      set(GEN_TOOL "${MSVC_LIB_EXE}")

      if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_FLAG "/machine:x64")
      else()
        set(ARCH_FLAG "/machine:x86")
      endif()

      set(GEN_ARGS ${ARCH_FLAG} "/def:${DEF_PATH}" "/out:${LIB_PATH}")
    else()
      find_program(DLLTOOL_EXE NAMES llvm-dlltool dlltool)
      if (NOT DLLTOOL_EXE)
        message(FATAL_ERROR "Could not find llvm-dlltool or dlltool to generate import library")
      endif()

      set(GEN_TOOL "${DLLTOOL_EXE}")

      if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_FLAG "-m" "i386:x86-64")
      else()
        set(ARCH_FLAG "-m" "i386")
      endif()

      set(GEN_ARGS ${ARCH_FLAG} "-d" "${DEF_PATH}" "-l" "${LIB_PATH}")
    endif()

    add_custom_command(
      OUTPUT "${LIB_PATH}"
      COMMAND "${GEN_TOOL}" ${GEN_ARGS}
      DEPENDS "${DEF_PATH}"
      COMMENT "Generating ${DEF_NAME}.lib from ${DEF_NAME}.def"
      VERBATIM
    )

    string(REPLACE "-" "_" SANITIZED_NAME "${LIB_TARGET}")
    string(REPLACE "::" "_" SANITIZED_NAME "${SANITIZED_NAME}")
    set(HELPER_TARGET "generate_${SANITIZED_NAME}_import")

    add_custom_target(${HELPER_TARGET} DEPENDS "${LIB_PATH}")

    add_library(${LIB_TARGET} SHARED IMPORTED GLOBAL)
    set_target_properties(${LIB_TARGET} PROPERTIES
      IMPORTED_IMPLIB "${LIB_PATH}"
    )

    if (TARGET ${CONSUMER_TARGET})
      target_link_libraries(${CONSUMER_TARGET} PRIVATE ${LIB_TARGET})
      add_dependencies(${CONSUMER_TARGET} ${HELPER_TARGET})
    else()
      message(FATAL_ERROR "target_link_import_library: The target '${CONSUMER_TARGET}' must be defined before calling this function.")
    endif()
  endif()
endfunction()