#include "cbase.h"

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
