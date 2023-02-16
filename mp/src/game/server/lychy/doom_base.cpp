#include "cbase.h"
#include "ai_basenpc.h"

//SPRITES DOOMS AS DOS DAM DAS D

class CBaseDoom : public CAI_BaseNPC
{
	DECLARE_CLASS(CBaseDoom, CAI_BaseNPC);
	DECLARE_SERVERCLASS();
	int UpdateTransmitState() { return SetTransmitState(FL_EDICT_ALWAYS); }
	void Spawn();
	void Precache();

};

IMPLEMENT_SERVERCLASS_ST(CBaseDoom, DT_BaseDoom)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(doom, CBaseDoom);

void CBaseDoom::Precache()
{
	PrecacheModel("materials/doom/rabblerouser/rabb.vmt");
}
void CBaseDoom::Spawn()
{
	Precache();
	SetModel("materials/doom/rabblerouser/rabb.vmt");
	//SetSize(Vector(-25,-25,0), Vector(25,25,100));

	SetHullType(HULL_LARGE);
	SetHullSizeNormal();
	//SetSolid(SOLID_BBOX);
	BaseClass::Spawn();

}