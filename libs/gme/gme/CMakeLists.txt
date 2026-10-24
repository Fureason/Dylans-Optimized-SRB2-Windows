


set(libgme_SRCS Blip_Buffer.cpp
                Classic_Emu.cpp
                Data_Reader.cpp
                Dual_Resampler.cpp
                Effects_Buffer.cpp
                Fir_Resampler.cpp
                gme.cpp
                Gme_File.cpp
                M3u_Playlist.cpp
                Multi_Buffer.cpp
                Music_Emu.cpp
                )


if (USE_GME_AY OR USE_GME_KSS)
    set(libgme_SRCS ${libgme_SRCS}
                Ay_Apu.cpp
        )
endif()


if (USE_GME_VGM OR USE_GME_GYM)
    set(libgme_SRCS ${libgme_SRCS}
                Ym2612_Emu.cpp
        )
endif()


if (USE_GME_VGM OR USE_GME_GYM OR USE_GME_KSS)
    set(libgme_SRCS ${libgme_SRCS}
                Sms_Apu.cpp
        )
endif()

if (USE_GME_AY)
    set(libgme_SRCS ${libgme_SRCS}

                Ay_Cpu.cpp
                Ay_Emu.cpp
        )
endif()

if (USE_GME_GBS)
    set(libgme_SRCS ${libgme_SRCS}
                Gb_Apu.cpp
                Gb_Cpu.cpp
                Gb_Oscs.cpp
                Gbs_Emu.cpp
        )
endif()

if (USE_GME_GYM)
    set(libgme_SRCS ${libgme_SRCS}


                Gym_Emu.cpp
        )
endif()

if (USE_GME_HES)
    set(libgme_SRCS ${libgme_SRCS}
                Hes_Apu.cpp
                Hes_Cpu.cpp
                Hes_Emu.cpp
        )
endif()

if (USE_GME_KSS)
    set(libgme_SRCS ${libgme_SRCS}


                Kss_Cpu.cpp
                Kss_Emu.cpp
                Kss_Scc_Apu.cpp
        )
endif()

if (USE_GME_NSF OR USE_GME_NSFE)
    set(libgme_SRCS ${libgme_SRCS}
                Nes_Apu.cpp
                Nes_Cpu.cpp
                Nes_Fme7_Apu.cpp
                Nes_Namco_Apu.cpp
                Nes_Oscs.cpp
                Nes_Vrc6_Apu.cpp
                Nsf_Emu.cpp
        )
endif()

if (USE_GME_NSFE)
    set(libgme_SRCS ${libgme_SRCS}
                Nsfe_Emu.cpp
        )
endif()

if (USE_GME_SAP)
    set(libgme_SRCS ${libgme_SRCS}
                Sap_Apu.cpp
                Sap_Cpu.cpp
                Sap_Emu.cpp
        )
endif()

if (USE_GME_SPC)
    set(libgme_SRCS ${libgme_SRCS}
                Snes_Spc.cpp
                Spc_Cpu.cpp
                Spc_Dsp.cpp
                Spc_Emu.cpp
                Spc_Filter.cpp
        )
endif()

if (USE_GME_VGM)
    set(libgme_SRCS ${libgme_SRCS}


                Vgm_Emu.cpp
                Vgm_Emu_Impl.cpp
                Ym2413_Emu.cpp
        )
endif()


set (EXPORTED_HEADERS gme.h)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gme_types.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/gme_types.h)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libgme.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libgme.pc @ONLY)




add_definitions(-DBLARGG_BUILD_DLL)


include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_library(gme ${libgme_SRCS})






set_target_properties(gme
    PROPERTIES VERSION ${GME_VERSION}
               SOVERSION 0)

install(TARGETS gme LIBRARY DESTINATION lib${LIB_SUFFIX}
                    RUNTIME DESTINATION bin   
                    ARCHIVE DESTINATION lib)  

install(FILES ${EXPORTED_HEADERS} DESTINATION include/gme)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libgme.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
