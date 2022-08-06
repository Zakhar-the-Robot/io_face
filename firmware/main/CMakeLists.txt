# *************************************************************************
#
# Copyright (c) 2022 Andrei Gramakov. All rights reserved.
#
# This file is licensed under the terms of the MIT license.  
# For a copy, see: https://opensource.org/licenses/MIT
#
# site:    https://agramakov.me
# e-mail:  mail@agramakov.me
#
# *************************************************************************

include("../components/lib_zakhar_faces/faces.cmake")

set(srcs "main.cpp" 
         "communication/can.cpp"
         "display/decode_image.c"
         "display/lcd.cpp"
         "display/spi.cpp"
         )
         
set(includes "."
             ${FACES_INCLUDES})

idf_component_register( SRCS ${srcs}
                        INCLUDE_DIRS ${includes}
                        EMBED_FILES ${FACES_FILES} )
