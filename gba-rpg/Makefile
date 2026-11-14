#---------------------------------------------------------------------------------------------------------------------
# Butano RPG framework - project makefile (based on butano/template/Makefile).
#
# LIBBUTANO must point to the "butano/butano" folder of a Butano checkout.
# Override it from the command line or the environment if yours lives elsewhere:
#     make -j8 LIBBUTANO=/path/to/butano/butano
#---------------------------------------------------------------------------------------------------------------------
TARGET      	:=  butano_rpg
BUILD       	:=  build
LIBBUTANO   	?=  ../../butano/butano
PYTHON      	?=  python3
SOURCES     	:=  src src/data
INCLUDES    	:=  include
DATA        	:=
GRAPHICS    	:=  graphics
AUDIO       	:=  audio
# Music (.mod/.xm/.it/.s3m) and sound effects (.wav) of audio/ are played with Maxmod (mmutil comes with devkitPro).
AUDIOBACKEND	:=  maxmod
AUDIOTOOL		:=
DMGAUDIO    	:=
DMGAUDIOBACKEND	:=  null
ROMTITLE    	:=  BUTANO RPG
ROMCODE     	:=  BRPG
USERFLAGS   	:=
USERCXXFLAGS	:=
USERASFLAGS 	:=
USERLDFLAGS 	:=
USERLIBDIRS 	:=
USERLIBS    	:=
DEFAULTLIBS 	:=
STACKTRACE		:=
USERBUILD   	:=
EXTTOOL     	:=

#---------------------------------------------------------------------------------------------------------------------
# Export absolute butano path:
#---------------------------------------------------------------------------------------------------------------------
ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

ifeq ($(strip $(LIBBUTANOABS)),)
$(error Butano not found at "$(LIBBUTANO)". Set LIBBUTANO to the butano/butano folder, see README.md)
endif

#---------------------------------------------------------------------------------------------------------------------
# Include main makefile:
#---------------------------------------------------------------------------------------------------------------------
include $(LIBBUTANOABS)/butano.mak
