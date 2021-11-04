cmake_minimum_required(VERSION 3.14)
project(LinuxServerHost C)

set(CMAKE_C_STANDARD 99)

set(PROJECT_NAME LinuxServerHost)
set(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Initialize Xenomai
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/include/xenomai/cobalt -I/usr/include/xenomai -D_GNU_SOURCE -D_REENTRANT -fasynchronous-unwind-tables -D__COBALT__ -D__COBALT_WRAP__ -I/usr/include/xenomai/alchemy -g -O2 -w")

# Advantech related path variables
set (ADVANTECH_LIBS "/opt/advantech/daqnavi_driver_source_code/linux_driver_source_4.0.2.6_64bit/libs/libbiodaq.so")
set (ADVANTECH_HDRS "/opt/advantech/inc/")
set (ADVANTECH_HDRS_ "/opt/advantech/examples/ANSI_C_Example/inc/")

include_directories(${INCDIR})

set (SRC_FILES
        ${SRCDIR}/control.c
        ${SRCDIR}/daq.c
        ${SRCDIR}/main.c
        ${SRCDIR}/encoder.c
        ${SRCDIR}/server.c

        ${INCDIR}/control.h
        ${INCDIR}/daq.h
        ${INCDIR}/main.h
        ${INCDIR}/encoder.h
        ${INCDIR}/server.h

        ${ADVANTECH_HDRS}/bdaqctrl.h
        ${ADVANTECH_HDRS_}/compatibility.h
        )

add_executable(${PROJECT_NAME}
        ${SRCDIR}/control.c
        ${SRCDIR}/daq.c
        ${SRCDIR}/main.c
        ${SRCDIR}/encoder.c
        ${SRCDIR}/server.c

        ${INCDIR}/control.h
        ${INCDIR}/daq.h
        ${INCDIR}/main.h
        ${INCDIR}/encoder.h
        ${INCDIR}/server.h

        ${ADVANTECH_HDRS}/bdaqctrl.h
        ${ADVANTECH_HDRS_}/compatibility.h
        )

# Add Advantech Library for compilation
add_library(libbiodaq
        SHARED
        IMPORTED)
set_property(TARGET libbiodaq PROPERTY IMPORTED_LOCATION ${ADVANTECH_LIBS})

# add pthread at the end of target_link_libraries if you use multithreading
target_link_libraries(${PROJECT_NAME} PUBLIC m libbiodaq -lm -Wl,--no-as-needed -Wl,@/usr/lib/cobalt.wrappers -Wl,@/usr/lib/modechk.wrappers -lalchemy -lcopperplate /usr/lib/xenomai/bootstrap.o -Wl,--wrap=main -Wl,--dynamic-list=/usr/lib/dynlist.ld -L/usr/lib -lcobalt -lmodechk -lpthread -lrt)

install (TARGETS LinuxServerHost DESTINATION ${CMAKE_INSTALL_BINDIR} PERMISSIONS OWNER_EXECUTE OWNER_READ)
