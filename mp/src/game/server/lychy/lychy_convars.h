#pragma once

void ReplaceFactory(const char* classnameToReplace, const char* classnameToReplaceWith, bool replaceDontRevert);
#define FACTORYCMD( cmdName, classnameToReplace, classnameToReplaceWith )    void cmdName##_CC( IConVar *var, const char *pOldValue, float flOldValue );  ConVar cmdName( #cmdName, "0",0,"",cmdName##_CC);  void cmdName##_CC( IConVar *var, const char *pOldValue, float flOldValue ) { ReplaceFactory(classnameToReplace, classnameToReplaceWith, cmdName.GetBool());  };

