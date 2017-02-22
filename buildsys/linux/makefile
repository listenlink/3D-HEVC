### enforce 32-bit build : 1=yes, 0=no
M32?= 0
HEVC_EXT?= ''
 
export M32

ifneq ($(HEVC_EXT), '')
ADDDEFS = -DHEVC_EXT=$(HEVC_EXT)
endif

all:
	$(MAKE) -C lib/TLibVideoIO 	MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibCommon 	MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoder 	MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibEncoder 	MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibRenderer     MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibExtractor    MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TAppCommon       MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoder      MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppEncoder      MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppRenderer     MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppExtractor    MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C utils/annexBbytecount       MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C utils/convert_NtoMbit_YCbCr MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoderAnalyser 	MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoderAnalyser      MM32=$(M32) ADDDEFS=$(ADDDEFS)

debug:
	$(MAKE) -C lib/TLibVideoIO 	debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibCommon 	debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoder 	debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibEncoder 	debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibRenderer     debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibExtractor    debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TAppCommon       debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoder      debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppEncoder      debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppRenderer     debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppExtractor    debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C utils/annexBbytecount       debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C utils/convert_NtoMbit_YCbCr debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoderAnalyser 	debug MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoderAnalyser      debug MM32=$(M32) ADDDEFS=$(ADDDEFS)

release:
	$(MAKE) -C lib/TLibVideoIO 	release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibCommon 	release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoder 	release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibEncoder 	release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibRenderer     release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibExtractor    release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TAppCommon       release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoder      release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppEncoder      release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppRenderer     release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppExtractor    release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C utils/annexBbytecount release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C utils/convert_NtoMbit_YCbCr release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoderAnalyser 	release MM32=$(M32) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoderAnalyser      release MM32=$(M32) ADDDEFS=$(ADDDEFS)

clean: clean_highbitdepth
	$(MAKE) -C lib/TLibVideoIO 	clean MM32=$(M32)
	$(MAKE) -C lib/TLibCommon 	clean MM32=$(M32)
	$(MAKE) -C lib/TLibDecoder 	clean MM32=$(M32)
	$(MAKE) -C lib/TLibEncoder 	clean MM32=$(M32)
	$(MAKE) -C lib/TLibRenderer     clean MM32=$(M32)
	$(MAKE) -C lib/TLibExtractor    clean MM32=$(M32)

	$(MAKE) -C lib/TAppCommon       clean MM32=$(M32)
	$(MAKE) -C app/TAppDecoder      clean MM32=$(M32)
	$(MAKE) -C app/TAppEncoder      clean MM32=$(M32)
	$(MAKE) -C app/TAppRenderer    clean MM32=$(M32)	
	$(MAKE) -C app/TAppExtractor    clean MM32=$(M32)	
	$(MAKE) -C utils/annexBbytecount       clean MM32=$(M32)
	$(MAKE) -C utils/convert_NtoMbit_YCbCr clean MM32=$(M32)
	$(MAKE) -C lib/TLibDecoderAnalyser 	clean MM32=$(M32)
	$(MAKE) -C app/TAppDecoderAnalyser      clean MM32=$(M32)

all_highbitdepth:
	$(MAKE) -C lib/TLibVideoIO 	MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibCommon 	MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoder 	MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibEncoder 	MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibRenderer     MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibExtractor    MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TAppCommon       MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoder      MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppEncoder      MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppRenderer     MM32=$(M32) ADDDEFS=$(ADDDEFS) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppExtractor    MM32=$(M32) ADDDEFS=$(ADDDEFS) ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoderAnalyser 	MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoderAnalyser      MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)

debug_highbitdepth:
	$(MAKE) -C lib/TLibVideoIO 	debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibCommon 	debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoder 	debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibEncoder 	debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibRenderer     debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibExtractor    debug M32=$(M32)  HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TAppCommon       debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoder      debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppEncoder      debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppRenderer     debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppExtractor    debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoderAnalyser debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoderAnalyser debug MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)

release_highbitdepth:
	$(MAKE) -C lib/TLibVideoIO 	release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibCommon 	release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoder 	release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibEncoder 	release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibRenderer     release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibExtractor    release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TAppCommon       release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoder      release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppEncoder      release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppRenderer     release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppExtractor    release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C lib/TLibDecoderAnalyser release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)
	$(MAKE) -C app/TAppDecoderAnalyser release MM32=$(M32) HIGHBITDEPTH=1 ADDDEFS=$(ADDDEFS)

clean_highbitdepth:
	$(MAKE) -C lib/TLibVideoIO 	clean MM32=$(M32) HIGHBITDEPTH=1
	$(MAKE) -C lib/TLibCommon 	clean MM32=$(M32) HIGHBITDEPTH=1
	$(MAKE) -C lib/TLibDecoder 	clean MM32=$(M32) HIGHBITDEPTH=1
	$(MAKE) -C lib/TLibEncoder 	clean MM32=$(M32) HIGHBITDEPTH=1
	$(MAKE) -C lib/TLibRenderer     clean MM32=$(M32) HIGHBITDEPTH=1 
	$(MAKE) -C lib/TLibExtractor    clean MM32=$(M32) HIGHBITDEPTH=1  
	$(MAKE) -C lib/TAppCommon       clean MM32=$(M32) HIGHBITDEPTH=1
	$(MAKE) -C app/TAppDecoder      clean MM32=$(M32) HIGHBITDEPTH=1
	$(MAKE) -C app/TAppEncoder      clean MM32=$(M32) HIGHBITDEPTH=1
	$(MAKE) -C app/TAppRenderer     clean MM32=$(M32) HIGHBITDEPTH=1 	
	$(MAKE) -C app/TAppExtractor    clean MM32=$(M32)HIGHBITDEPTH=1 	
	$(MAKE) -C lib/TLibDecoderAnalyser 	clean MM32=$(M32) HIGHBITDEPTH=1
	$(MAKE) -C app/TAppDecoderAnalyser      clean MM32=$(M32) HIGHBITDEPTH=1

everything: all all_highbitdepth
