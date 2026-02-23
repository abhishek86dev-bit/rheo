install(
    TARGETS rheo_exe
    RUNTIME COMPONENT rheo_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
