##########################################
# miniaudio
##########################################

add_library(miniaudio miniaudio.c)

target_include_directories(miniaudio PUBLIC ./)