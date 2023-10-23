#include "cbase.h"
#include "baseentity.h"

#define WIN32_LEAN_AND_MEAN
#define PSAPI_VERSION 2
#include <Windows.h>
#include <Psapi.h>
#include <eiface.h>
#include <iserver.h>

struct factory_t
{
	IEntityFactory* factory;
	unsigned short index;
};

CUtlDict<factory_t, unsigned short> factoryBackup;

void ReplaceFactory(const char* classnameToReplace, const char* classnameToReplaceWith, bool replaceDontRevert)
{
	CUtlDict< IEntityFactory*, unsigned short>* dict = &((CEntityFactoryDictionary*)EntityFactoryDictionary())->m_Factories;
	if (replaceDontRevert)
	{
		unsigned short indexToReplace = dict->Find(classnameToReplace);
		unsigned short indexToReplaceWith = dict->Find(classnameToReplaceWith);

		IEntityFactory*& factoryOld = dict->Element(indexToReplace);
		factoryBackup.Insert(classnameToReplace, factory_t{ factoryOld ,indexToReplace });
		factoryOld = dict->Element(indexToReplaceWith);
	}
	else
	{
		unsigned short position = factoryBackup.Find(classnameToReplace);
		factory_t factoryBackedUp = factoryBackup.Element(position);
		dict->Element(factoryBackedUp.index) = factoryBackedUp.factory;
		factoryBackup.RemoveAt(position);

	}
}

CON_COMMAND(precachesound,"Precache a sound")
{
	HSOUNDSCRIPTHANDLE handle = CBaseEntity::PrecacheScriptSound(args.ArgV()[1]);
	Msg("%s returns %x\n", args.ArgV()[1], handle);

}


void* RandomCorrupt(void* startOfMem, void* endOfMem)
{
	HANDLE hProc = GetCurrentProcess();
	void* byteChange = (void*)RandomInt((unsigned int)startOfMem, (unsigned int)endOfMem);
	char prevChar;
	HRESULT result;
	ReadProcessMemory(hProc, byteChange, &prevChar, 1, NULL);
	result = GetLastError();
	if (result)
	{
		Warning("ReadProcessMemory failed with %i\n", result);
		return NULL;
	}
	char newChar = RandomInt(0x00, 0xFF);

	WriteProcessMemory(hProc, byteChange, &newChar, 1, NULL);
	result = GetLastError();
	if (result)
	{
		Warning("WriteProcessMemory failed with %i\n", result);
		return NULL;
	}
	Msg("At 0x%X, changed %1X to %1X\n", byteChange, (unsigned)prevChar, (unsigned)newChar);
	return byteChange;
}
CON_COMMAND(corrupt,"Randomly corrupt n times")
{
	static HMODULE hMods[512];
	static HANDLE hProc = GetCurrentProcess();
	static DWORD totalBytes;

	if (totalBytes == 0)
	{
		EnumProcessModules(hProc, hMods, sizeof(hMods), &totalBytes);
	}

	int numRuns = atoi(args.ArgV()[1]);
	for (int i = 0; i < numRuns; i++)
	{
		int moduleChoice = RandomInt(0, totalBytes / sizeof(HMODULE));
		HMODULE& module = hMods[moduleChoice];
		if (!module)
		{
			continue;
		}
		MODULEINFO moduleInfo;
		GetModuleInformation(hProc, module, &moduleInfo, sizeof(moduleInfo));

	}
};

CON_COMMAND(set,"")
{
	if (FStrEq(args[1], "gpGlobals"))
	{
		if (FStrEq(args[2], "curtime"))
		{
			if (*args[3])
			{
				gpGlobals->curtime = V_atof(args[3]);
			}
			else
			{
				Msg("%f\n", gpGlobals->curtime);
			}
		}
	}
}