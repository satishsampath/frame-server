//========================================================================================
//
// PremiereFS.r
//
//========================================================================================

#include "PrSDKPiPLver.h"

/* Put the plug-in name, as it will appear in Premiere's menu, here. Needs to be localized! */
#define plugInName		"PluginPac FrameServer"
#define plugInMatchName	"PluginPac FrameServer"	// This name should not be changed

resource 'PiPL' (16000) {
	{	
		/* The plug-in type - PrImporter signifies a Premiere import module */
		Kind { PrCompile },
		
		/* This is the filter name as it will appear in a menu  */
		Name { plugInName },
	
		/* Internal name - never modify */
		AE_Effect_Match_Name { plugInMatchName },

		/* The version of the PiPL resource definition */
		AE_PiPL_Version { PiPLVerMajor, PiPLVerMinor },

		/* The version of the Playback record, for Premiere 5.0 it is 1 */
		AE_Effect_Spec_Version { 1, 0 },

		/* This is the version of the plug-in itself */
		AE_Effect_Version
			{ (PiPLMajorVersion << 19) | (PiPLMinorVersion << 15) | (PiPLStage << 9) | PiPLBuildNum },
	}
};

