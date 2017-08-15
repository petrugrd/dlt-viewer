# Copyright (C) 2016, Jack S. Smith
#
# This file is part of GENIVI DLT-Viewer project.
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License (MPL), v. 2.0.
# If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# For further information see http://www.genivi.org/.
#
# List of changes:
# 01.Oct.2016, Jack Smith <jack.smith@elektrobit.com>, Original Author
#
add_library (qdlt dlt_common.c
                  qdltipconnection.cpp
                  qdlttcpconnection.cpp
                  qdltudpconnection.cpp
                  qdltserialconnection.cpp
                  qdltmsg.cpp
                  qdltfilter.cpp
                  qdltfile.cpp
                  qdltcontrol.cpp
                  qdltconnection.cpp
                  qdltbase.cpp
                  qdltargument.cpp
                  qdltfilterlist.cpp
                  qdltfilterindex.cpp
                  qdltdefaultfilter.cpp
                  qdltpluginmanager.cpp
                  qdltplugin.cpp
                  qdltsegmentedmsg.cpp)

target_include_directories (qdlt  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                          ${CMAKE_CURRENT_SOURCE_DIR}/../qextserialport/src
                                          ${CMAKE_CURRENT_SOURCE_DIR}/../src)

target_link_libraries(qdlt Qt5::Core Qt5::Network)

install(TARGETS qdlt DESTINATION deploy)