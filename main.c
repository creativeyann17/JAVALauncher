#ifdef WINDOWS
#include <windows.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define bool    char
#define true    1
#define false   0

#define SETTINGS_EXT           ".ini"

#ifdef WINDOWS
#define JAVA_VERSION_FILE       "%TEMP%/version.txt"
#elif LINUX
#define JAVA_VERSION_FILE       "/tmp/version.txt"
#endif

#define STRING_SIZE         128
#define MESSAGE_SIZE        1024
#define COMMAND_SIZE        8192

static const char * NO_JAVA_AVAILABLE_MSG = "A Java Runtime Environment (JRE) or Java Development Kit (JDK)\nmust be available in order to run %s. No Java virtual machine\nwas found after searching the following locations:\n%s";
static const char * NO_EXEC_AVAILABLE_MSG = "No executor system is available on your operating system. A batch file\nhas been generated please use it instead of this binary file.";
static const char * NO_EXEC_BATCH_AVAILABLE_MSG = "No executor system is available on your operating system. Failed to\ngenerate an alternative batch file.";
static const char * NO_SETTINGS_FILE_MSG  = "The program has failed to parse the settings file %s\nError message: %s";
static const char * NO_SETTINGS_VALUE_MSG = "Missing information in settings file %s\nMissing value for group: %s key named: %s";
static const char * PRG_EXEC_ERROR_MSG    = "An error occurred during execution of program %s\nPlease refer to console output messages.";
static const char * NO_JAVA_VERSION_MSG   = "The current installed Java version is not compatible:\nCurrent installed: %s\nMinimum required: %s";

#define CONF_PRG_KEY            "[PROGRAM]"
#define CONF_PRG_BIN_KEY        "BIN"
#define CONF_PRG_ARGS_KEY       "ARGS"
#define CONF_JAVA_KEY           "[JAVA]"
#define CONF_JAVA_PATH_KEY      "PATH"
#define CONF_JAVA_VERSION_KEY   "VERSION"
#define CONF_JAVA_ARGS_KEY      "ARGS"

#define RETURN_NO_ERROR     0
#define RETURN_ERROR        1

typedef struct SConf{
    char confFile[STRING_SIZE];
    char prgName[STRING_SIZE];
    char prgBin[STRING_SIZE];
    char prgArgs[STRING_SIZE];
    char prgConsole[STRING_SIZE];
    char javaPath[STRING_SIZE];
    char javaVersion[STRING_SIZE];
    char javaArgs[STRING_SIZE];
} TConf;

static TConf m_Conf;

static int execute(char * _cmd,bool _Hide){
    if(_Hide){
#ifdef WINDOWS
    	strcat(_cmd," > NUL");
#elif LINUX
		strcat(_cmd," > /dev/null");
#endif
	}
    return system(_cmd);
}

static void show(const char * _msg){
#ifdef WINDOWS
    MessageBox(NULL,_msg,m_Conf.prgName,MB_OK);
#elif LINUX
	char l_Msg[MESSAGE_SIZE]={0};
	strcat(l_Msg,"zenity --info --no-wrap");
	strcat(l_Msg," --title=\"");
	strcat(l_Msg,m_Conf.prgName);
	strcat(l_Msg,"\"");
	strcat(l_Msg," --text=\"");
	strcat(l_Msg,_msg);
	strcat(l_Msg,"\"");
	execute(l_Msg,false);
#endif
}

static void extractProgramName(const char * _name){
    const char * l_Index=NULL;
    const char * l_ExtIndex=NULL;
    l_Index=strrchr(_name,'\\');
    if(l_Index==NULL)
        l_Index=strrchr(_name,'/');
    if(l_Index==NULL)
        l_Index=_name;
    l_ExtIndex=strstr(_name,".exe");
    if(l_ExtIndex){
        strncpy(m_Conf.prgName,l_Index+1,l_ExtIndex-l_Index-1);
    }else{
        strcpy(m_Conf.prgName,l_Index+1);
    }
    #ifdef LINUX
    strcat(m_Conf.confFile,"./");
    #endif
    strcat(m_Conf.confFile,m_Conf.prgName);
    strcat(m_Conf.confFile,".ini");
}

static void removeCommentary(char * _Line){
    const char * l_Index=strchr(_Line,';');
    if(l_Index){
        memset(_Line+(l_Index-_Line),0,1);
    }
}

static int extractSettingsFile(){
    FILE * l_File = fopen (m_Conf.confFile,"r");
    if (l_File==NULL){
        return RETURN_ERROR;
    }

    bool isProg=false;
    bool isJava=false;

    char l_Key[STRING_SIZE];
    char l_Value[STRING_SIZE];
    char l_Line[STRING_SIZE];

    while(fgets(l_Line,STRING_SIZE,l_File)){
        removeCommentary(l_Line);
        const char * l_Index=strchr(l_Line,'=');
        if(l_Index){
            strncpy(l_Key,l_Line,l_Index-l_Line);
            strcpy(l_Value,l_Index+1);

            size_t l_ValueLength=strlen(l_Value);
            if(strlen(l_Value)==0){
                continue;
            }else{
                if(l_Value[l_ValueLength-1]=='\n'){
                    l_Value[l_ValueLength-1]=0;
                }
				if(l_Value[l_ValueLength-2]=='\r'){
                    l_Value[l_ValueLength-2]=0;
                }
            }

            if(isProg){
                if(strstr(l_Key,CONF_PRG_BIN_KEY)){
                    strcpy(m_Conf.prgBin,l_Value);
                }else if(strstr(l_Key,CONF_PRG_ARGS_KEY)){
                    strcpy(m_Conf.prgArgs,l_Value);
                }
            }else if(isJava){
                if(strstr(l_Key,CONF_JAVA_PATH_KEY)){
                    strcpy(m_Conf.javaPath,l_Value);
                }else if(strstr(l_Key,CONF_JAVA_VERSION_KEY)){
                    strcpy(m_Conf.javaVersion,l_Value);
                }else if(strstr(l_Key,CONF_JAVA_ARGS_KEY)){
                    strcpy(m_Conf.javaArgs,l_Value);
                }
            }
        }else{
            if(strstr(l_Line,CONF_PRG_KEY)){
                isProg=true;
            }else if(strstr(l_Line,CONF_JAVA_KEY)){
                isJava=true;
            }else{
                isProg=false;
                isJava=false;
            }
        }
    }

    fclose(l_File);
    return RETURN_NO_ERROR;
}

static int checkSettings(char * _Msg){
    const char * l_Grp=NULL;
    const char * l_Key=NULL;

    if(strlen(m_Conf.prgBin)==0){
        l_Grp=CONF_PRG_KEY;
        l_Key=CONF_PRG_BIN_KEY;
    }else if(strlen(m_Conf.javaPath)==0){
        l_Grp=CONF_JAVA_KEY;
        l_Key=CONF_JAVA_PATH_KEY;
    }else if(strlen(m_Conf.javaVersion)==0){
        l_Grp=CONF_JAVA_KEY;
        l_Key=CONF_JAVA_VERSION_KEY;
    }

    if(l_Key && l_Grp){
        sprintf(_Msg,NO_SETTINGS_VALUE_MSG,m_Conf.confFile,l_Grp,l_Key);
        return RETURN_ERROR;
    }

    return RETURN_NO_ERROR;
}

static int generateBatchFile(const char * _Cmd){
    char l_FileName[STRING_SIZE];
#ifdef WINDOWS
    sprintf(l_FileName,"%s.%s",m_Conf.prgName,".bat");
#elif LINUX
	sprintf(l_FileName,"%s.%s",m_Conf.prgName,".sh");
#endif
    FILE * l_File = fopen (l_FileName,"w");
    if (l_File==NULL){
        return RETURN_ERROR;
    }
    fputs(_Cmd,l_File);
    fclose(l_File);
    return RETURN_NO_ERROR;
}

static int checkJavaAndVersion(){
    char l_JavaCommand[COMMAND_SIZE]={0};
    strcat(l_JavaCommand,m_Conf.javaPath);
    strcat(l_JavaCommand," -version");
	strcat(l_JavaCommand," > ");
  	strcat(l_JavaCommand,JAVA_VERSION_FILE);
#ifdef WINDOWS
    strcat(l_JavaCommand," 2<&1");
#elif LINUX
	strcat(l_JavaCommand," 2>&1");
#endif
    char l_Msg[MESSAGE_SIZE]={0};
    if(execute(l_JavaCommand,false)!=RETURN_NO_ERROR){
        sprintf(l_Msg,NO_JAVA_AVAILABLE_MSG,m_Conf.prgBin,m_Conf.javaPath);
        show(l_Msg);
        return RETURN_ERROR;
    }

    FILE * l_JAVAVersionFile = fopen(JAVA_VERSION_FILE,"r");
    if(l_JAVAVersionFile){
        bool isError=false;
        char l_Line[STRING_SIZE]={0};
        if(fgets(l_Line,STRING_SIZE,l_JAVAVersionFile)){
            char l_Version[STRING_SIZE]={0};
            strncpy(l_Version,&l_Line[14],5);
            if(strcmp(l_Version,m_Conf.javaVersion)<0){
                sprintf(l_Msg,NO_JAVA_VERSION_MSG,l_Version,m_Conf.javaVersion);
                show(l_Msg);
                isError=true;
            }
        }
        fclose(l_JAVAVersionFile);
        unlink(JAVA_VERSION_FILE);
        if(isError)
            return RETURN_ERROR;
    }

    return RETURN_NO_ERROR;
}

int main(int argc, char * argv[]){

    char l_Msg[MESSAGE_SIZE]={0};

    extractProgramName(argv[0]);
    if(extractSettingsFile()==RETURN_ERROR){
        sprintf(l_Msg,NO_SETTINGS_FILE_MSG,m_Conf.confFile,strerror(errno));
        show(l_Msg);
        return RETURN_ERROR;
    }

    if(checkSettings(l_Msg)==RETURN_ERROR){
        show(l_Msg);
        return RETURN_ERROR;
    }

    char l_Command[COMMAND_SIZE]={0};
    strcat(l_Command,m_Conf.javaPath);
    strcat(l_Command," ");
    strcat(l_Command,m_Conf.javaArgs);
    strcat(l_Command," -jar ");
    strcat(l_Command,m_Conf.prgBin);
    strcat(l_Command," ");
    strcat(l_Command,m_Conf.prgArgs);

    if(execute(NULL,false)==0){ // 0 is OK
        if(generateBatchFile(l_Command)==RETURN_ERROR){
            show(NO_EXEC_BATCH_AVAILABLE_MSG);
        }else{
            show(NO_EXEC_AVAILABLE_MSG);
        }
        return RETURN_ERROR;
    }

    if(checkJavaAndVersion()!=RETURN_NO_ERROR){
        return RETURN_ERROR;
    }

    if(execute(l_Command,false)!=RETURN_NO_ERROR){
        sprintf(l_Msg,PRG_EXEC_ERROR_MSG,m_Conf.prgBin);
        show(l_Msg);
        return RETURN_ERROR;
    }

    return 0;
}
