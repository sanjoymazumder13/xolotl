list(APPEND XOLOTL_PERF_HEADERS
    ${XOLOTL_PERF_HEADER_DIR}/dummy/DummyEventCounter.h
    ${XOLOTL_PERF_HEADER_DIR}/dummy/DummyHandlerRegistry.h
    ${XOLOTL_PERF_HEADER_DIR}/dummy/DummyHardwareCounter.h
    ${XOLOTL_PERF_HEADER_DIR}/dummy/DummyTimer.h
)

list(APPEND XOLOTL_PERF_SOURCES
    ${XOLOTL_PERF_SOURCE_DIR}/dummy/DummyHandlerRegistry.cpp
    ${XOLOTL_PERF_SOURCE_DIR}/dummy/DummyHardwareCounter.cpp
    ${XOLOTL_PERF_SOURCE_DIR}/dummy/DummyTimer.cpp
)
