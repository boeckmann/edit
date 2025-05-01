/* ------------- config.c ------------- */

#include <stdlib.h>
#include <io.h>
#include "dflatp.h"
#include "config.h"


/* ------ default configuration values ------- */
CONFIG cfg = {
    "0.9.1.0",
    0,          /* Color			*/
    FALSE,	/* Snowy CGA			*/
    TRUE,       /* Editor Insert Mode		*/
    8,          /* Editor tab stop size		*/ /* was 4 before 0.7b */
    FALSE,      /* Editor word wrap		*/
    FALSE,      /* Textured application window	*/
    25,         /* Number of screen lines	*/
    "Lpt1",	/* Printer Port			*/
    66,         /* Lines per printer page	*/
    80,		/* characters per printer line	*/
    6,		/* Left printer margin		*/
    70,		/* Right printer margin		*/
    3,		/* Top printer margin		*/
    55,		/* Bottom printer margin	*/
    FALSE	/* Read only mode               */
};

extern BOOL ConfigLoaded;


FILE *OpenConfig(char *mode)
{
	char path[64];
	BuildFileName(path, DFlatApplication, ".cfg");
	return fopen(path, mode);
}

/* ------ load a configuration file from disk ------- */
BOOL LoadConfig(void)
{
	strcpy (cfg.version, ProgramVersion);
	
	if (ConfigLoaded == FALSE)	{
	    FILE *fp = OpenConfig("rb");
    	if (fp != NULL)    {
        	fread(cfg.version, sizeof cfg.version, 1, fp);
        	if (strcmp(cfg.version, ProgramVersion) == 0)    {

            	fseek(fp, 0L, SEEK_SET);
            	fread(&cfg, sizeof(CONFIG), 1, fp);
 		       	fclose(fp);
        	}
        	else	{
				char path[64];
				BuildFileName(path, DFlatApplication, ".cfg");
	        	fclose(fp);
				unlink(path);
           	strcpy(cfg.version, ProgramVersion);
			}
			ConfigLoaded = TRUE;
    	}
	}
    return ConfigLoaded;
}

/* ------ save a configuration file to disk ------- */
void SaveConfig(void)
{
    FILE *fp = OpenConfig("wb");

    memcpy(&cfg.clr, &SysConfig.VideoCurrentColorScheme, sizeof (ColorScheme));
		cfg.snowy = GetSnowyFlag();
		
		cfg.ScreenLines = SysConfig.VideoCurrentResolution.VRes;

    cfg.ReadOnlyMode = FALSE;	/* always save as FALSE for now (0.7b).  */
    /* There is no toggle / menu item: Nobody should override /R option! */
    /* however, you can manually modify the flag in the edit.cfg file... */
    if (fp != NULL)    {
        fwrite(&cfg, sizeof(CONFIG), 1, fp);
        fclose(fp);
    }
}
