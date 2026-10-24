

if(${CMAKE_SYSTEM} MATCHES Linux)

	return()
endif()

if("${SRB2_CONFIG_ASSET_DIRECTORY}" STREQUAL "")
	message(WARNING "SRB2_CONFIG_ASSET_DIRECTORY is not set, so installation will not contain data files.")
	return()
endif()

get_filename_component(SRB2_ASSET_DIRECTORY_ABSOLUTE "${SRB2_CONFIG_ASSET_DIRECTORY}" ABSOLUTE)

set(SRB2_ASSETS_DOCS
	"README.txt"
	"README-SDL.txt"
	"LICENSE.txt"
	"LICENSE-3RD-PARTY.txt"
)
list(TRANSFORM SRB2_ASSETS_DOCS PREPEND "/")
list(TRANSFORM SRB2_ASSETS_DOCS PREPEND "${SRB2_ASSET_DIRECTORY_ABSOLUTE}")





set(SRB2_ASSETS_GAME
	"srb2.pk3"
	"characters.pk3"
	"zones.pk3"
	"patch.pk3"
	"music.pk3"
	"models.dat"
)
list(TRANSFORM SRB2_ASSETS_GAME PREPEND "/")
list(TRANSFORM SRB2_ASSETS_GAME PREPEND "${SRB2_ASSET_DIRECTORY_ABSOLUTE}")

set(SRB2_ASSETS ${SRB2_ASSET_DOCS} ${SRB2_ASSETS_GAME})



if(${CMAKE_SYSTEM} MATCHES Darwin)
	get_target_property(outname SRB2SDL2 OUTPUT_NAME)
	install(FILES ${SRB2_ASSETS} DESTINATION "${outname}.app/Contents/Resources")
	install(DIRECTORY "${SRB2_ASSET_DIRECTORY_ABSOLUTE}/models" DESTINATION "${outname}.app/Contents/Resources")
	install(FILES ${SRB2_ASSETS_DOCS} DESTINATION .)
else()
	install(FILES ${SRB2_ASSETS} DESTINATION .)
	install(DIRECTORY "${SRB2_ASSET_DIRECTORY_ABSOLUTE}/models" DESTINATION .)
endif()
